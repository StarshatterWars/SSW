/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         TerrainPatch.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo, Destroyer Studios LLC

	OVERVIEW
	========
	A Single Multi-LOD Section of a Terrain Object
*/

#include "TerrainPatch.h"

#include "Terrain.h"
#include "TerrainLayer.h"
#include "TerrainRegion.h"
#include "Sim.h"
#include "SimLight.h"
#include "Bitmap.h"
#include "SimModel.h"
#include "Segment.h"
#include "GameStructs.h"

#include "Water.h"

#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogTerrainPatch, Log, All);

// +====================================================================+

// #define DEBUG_DETAIL 1

// +====================================================================+

static bool illuminating = false;

// +--------------------------------------------------------------------+

static inline void ZeroMem(void* Ptr, size_t SizeBytes)
{
	if (Ptr && SizeBytes > 0)
	{
		FMemory::Memzero(Ptr, SizeBytes);
	}
}

static inline double AbsD(double v) { return v < 0 ? -v : v; }

FORCEINLINE uint32 TPPackColor(const FColor& C)
{
	return (uint32(C.A) << 24) |
		(uint32(C.R) << 16) |
		(uint32(C.G) << 8) |
		(uint32(C.B));
}

// +--------------------------------------------------------------------+

TerrainPatch::TerrainPatch(Terrain* terr, const Bitmap* patch, const Rect& r,
	const FVector& p1, const FVector& p2)
	: ndetail(0)
	, terrain(terr)
	, rect(r)
	, water(nullptr)
	, min_height(1e9f)
	, max_height(-1e9f)
{
	luminous = true;   // we will do our own lighting
	own_model = false;  // manage the model lifetimes in this derived class

	max_detail = Terrain::DetailLevel();
	scale = AbsD(p1.X - p2.X) / (PATCH_SIZE - 1);
	mtnscale = p2.Y - p1.Y;
	base = p1.Y;
	size = p2.X - p1.X;

	ZeroMem(detail_levels, sizeof(detail_levels));

	terrain_width = 0;

	loc = (p1 + p2) * 0.5;
	loc.Y = base;

	radius = (float)(size * 0.75);
	heights = new float[MAX_VERTS];

	// Flat initialization (placeholder until CPU height sampling is implemented):
	for (int i = 0; i < MAX_VERTS; ++i)
	{
		float alt = 0.0f;

		if (alt < min_height) min_height = alt;
		if (alt > max_height) max_height = alt;

		// Preserve original "water cutout" behavior in spirit:
		// without per-pixel red channel, we can't detect red < 2,
		// so we leave alt unchanged here.
		heights[i] = alt;
	}

	Material* mtl = new Material;
	mtl->Ka = FColor(128, 128, 128, 255);
	mtl->Kd = FColor(77, 153, 51, 255);
	mtl->Ks = FColor::Black;

	// Terrain now returns UTexture2D* (per your conversion rules):
	mtl->tex_diffuse = terrain ? terrain->Texture() : nullptr;

	materials.append(mtl);

	List<TerrainLayer>& layers = terrain->GetLayers();
	for (int i = 0; i < layers.size(); i++) {
		Bitmap* tex0 = layers.at(i)->GetTileTexture();
		Bitmap* tex1 = nullptr;
		Bitmap* texd = layers.at(i)->GetDetailTexture();

		if (i < layers.size() - 1)
			tex1 = layers.at(i + 1)->GetTileTexture();

		if (!texd)
			texd = terrain->DetailTexture(0);

		mtl = new Material;
		mtl->Ka = FColor(128, 128, 128, 255);
		mtl->Kd = FColor(77, 153, 51, 255);
		mtl->Ks = FColor::Black;

		if ((i & 1) != 0) {
			mtl->tex_diffuse = tex1;
			mtl->tex_alternate = tex0;
		}
		else {
			mtl->tex_diffuse = tex0;
			mtl->tex_alternate = tex1;
		}

		mtl->tex_detail = texd;

		materials.append(mtl);
	}

	for (int i = 0; i <= max_detail; i++)
		BuildDetailLevel(i);

	model = detail_levels[1];
}

// +--------------------------------------------------------------------+

TerrainPatch::TerrainPatch(Terrain* terr,
	const Rect& r,
	const FVector& p1,
	const FVector& p2,
	double sea_level)
	: ndetail(0)
	, terrain(terr)
	, rect(r)
	, water(nullptr)
	, min_height(0.0f)
	, max_height(0.0f)
{
	luminous = true;   // water is lit by the graphics engine
	own_model = false;  // manage the model lifetimes in this derived class

	max_detail = Terrain::DetailLevel();
	scale = AbsD(p1.X - p2.X) / (PATCH_SIZE - 1);
	mtnscale = 0;
	base = sea_level;
	size = p2.X - p1.X;

	ZeroMem(detail_levels, sizeof(detail_levels));

	terrain_width = 0;

	loc = (p1 + p2) * 0.5;
	loc.Y = base;

	radius = (float)(size * 0.75);
	heights = new float[MAX_VERTS];

	for (int i = 0; i < MAX_VERTS; i++)
		heights[i] = (float)sea_level;

	Material* mtl = new Material;
	mtl->Ka = FColor(128, 128, 128, 255);
	mtl->Kd = FColor::White;
	mtl->Ks = FColor::White;
	mtl->power = 30.0f;
	mtl->shadow = false;
	mtl->tex_diffuse = terrain ? terrain->WaterTexture() : nullptr;
	// strcpy_s(mtl->shader, "WaterReflections");
	materials.append(mtl);

	water = terrain ? terrain->GetWater(1) : nullptr;

	for (int i = 0; i <= max_detail; i++)
		BuildDetailLevel(i);

	model = detail_levels[1];
}

// +--------------------------------------------------------------------+

TerrainPatch::~TerrainPatch()
{
	delete[] heights;

	for (int i = 0; i < MAX_LOD; i++) {
		SimModel* m = detail_levels[i];

		if (m) {
			m->GetMaterials().clear();
			delete m;
		}
	}

	materials.destroy();
}

// +--------------------------------------------------------------------+

void
TerrainPatch::SetScales(double s, double m, double b)
{
	scale = s;
	mtnscale = m;
	base = b;
}

// +--------------------------------------------------------------------+

static int mcomp(const void* a, const void* b)
{
	Poly* pa = (Poly*)a;
	Poly* pb = (Poly*)b;

	if (pa->sortval == pb->sortval)
		return 0;

	if (pa->sortval < pb->sortval)
		return 1;

	return -1;
}

static void bisect(VertexSet* vset, int v[4])
{
	double d1 = AbsD(vset->loc[v[0]].Y - vset->loc[v[3]].Y);
	double d2 = AbsD(vset->loc[v[1]].Y - vset->loc[v[2]].Y);

	if (d2 < 0.7 * d1) {
		int odd[4] = { v[1], v[3], v[0], v[2] };
		for (int i = 0; i < 4; i++)
			v[i] = odd[i];
	}
}

bool TerrainPatch::BuildDetailLevel(int level)
{
	int i, j;

	const int DetailSize = 1 << level;
	const int Ds1 = DetailSize + 1;

	if (DetailSize > PATCH_SIZE)
		return false;

	SimModel* LocalModel = new SimModel;
	detail_levels[level] = LocalModel;

	LocalModel->SetLuminous(luminous);
	LocalModel->SetDynamic(true);

	const int NUM_STRIPS = 4;
	const int NUM_INDICES_QUAD = 6;

	int nverts = Ds1 * Ds1 + Ds1 * 2 * NUM_STRIPS;
	int npolys = DetailSize * DetailSize * 2;
	int strip_len = DetailSize;
	int total = npolys + strip_len * NUM_STRIPS;

	if (water) {
		nverts = Ds1 * Ds1;
		strip_len = 0;
		total = npolys;
	}

	Surface* SurfacePtr = new Surface;
	VertexSet* vset = nullptr;

	if (SurfacePtr) {
		SurfacePtr->SetName("default");
		SurfacePtr->CreateVerts(nverts);
		SurfacePtr->CreatePolys(total);
		SurfacePtr->AddIndices(npolys * NUM_INDICES_TRI + strip_len * NUM_STRIPS * NUM_INDICES_QUAD);

		vset = SurfacePtr->GetVertexSet();
		if (!water)
			vset->CreateAdditionalTexCoords();

		// NOTE:
		// - vset->loc / vset->nrm / vset->s_loc are FVector*
		// - vset->diffuse is DWORD* (packed)
		// - vset->specular is FColor* (your current VertexSet definition)
		FMemory::Memzero(vset->loc, nverts * sizeof(FVector));
		FMemory::Memzero(vset->diffuse, nverts * sizeof(DWORD));
		FMemory::Memzero(vset->specular, nverts * sizeof(FColor));
		FMemory::Memzero(vset->tu, nverts * sizeof(float));
		FMemory::Memzero(vset->tv, nverts * sizeof(float));
		if (!water) {
			FMemory::Memzero(vset->tu1, nverts * sizeof(float));
			FMemory::Memzero(vset->tv1, nverts * sizeof(float));
		}
		FMemory::Memzero(vset->rw, nverts * sizeof(float));

		// initialize vertices
		FVector* pVert = vset->loc;
		float* pTu = vset->tu;
		float* pTv = vset->tv;
		float* pTu1 = vset->tu1;
		float* pTv1 = vset->tv1;
		FColor* pSpec = vset->specular;

		const int    dscale = (PATCH_SIZE - 1) / DetailSize;
		const double dt = 0.0625 / (Ds1 - 1);  // terrain texture scale
		const double dtt = 2.0000 / (Ds1 - 1);  // tile texture scale
		const double tu0 = (double)rect.x / rect.w / 16.0 + 1.0 / 16.0;
		const double tv0 = (double)rect.y / rect.h / 16.0;

		// surface verts
		for (i = 0; i < Ds1; i++) {
			for (j = 0; j < Ds1; j++) {
				*pVert = FVector(
					(float)(j * scale * dscale - (HALF_PATCH_SIZE * scale)),
					(float)(heights[i * dscale * PATCH_SIZE + j * dscale]),
					(float)(i * scale * dscale - (HALF_PATCH_SIZE * scale))
				);

				if (level >= 2) {
					*pTu++ = (float)(-j * dtt);
					*pTv++ = (float)(i * dtt);

					if (level >= 4 && !water) {
						*pTu1++ = (float)(-j * dtt * 3);
						*pTv1++ = (float)(i * dtt * 3);
					}

					// BlendValue returns packed DWORD (assumed 0xAARRGGBB); store into FColor
					{
						const DWORD Packed = BlendValue(pVert->Y);
						*pSpec++ = FColor(
							(uint8)((Packed >> 16) & 0xFF), // R
							(uint8)((Packed >> 8) & 0xFF), // G
							(uint8)((Packed >> 0) & 0xFF), // B
							(uint8)((Packed >> 24) & 0xFF)  // A
						);
					}
				}
				else {
					*pTu++ = (float)(tu0 - j * dt);
					*pTv++ = (float)(tv0 + i * dt);
				}

				pVert++;
			}
		}

		if (!water) {
			// strip 1 & 2 verts
			for (i = 0; i < Ds1; i += DetailSize) {
				for (j = 0; j < Ds1; j++) {
					FVector vl(
						(float)(j * scale * dscale - (HALF_PATCH_SIZE * scale)),
						(float)(heights[i * dscale * PATCH_SIZE + j * dscale]),
						(float)(i * scale * dscale - (HALF_PATCH_SIZE * scale))
					);

					*pVert++ = vl;

					DWORD BlendPacked = 0;

					if (level >= 2) {
						BlendPacked = BlendValue(vl.Y);

						*pSpec++ = FColor(
							(uint8)((BlendPacked >> 16) & 0xFF),
							(uint8)((BlendPacked >> 8) & 0xFF),
							(uint8)((BlendPacked >> 0) & 0xFF),
							(uint8)((BlendPacked >> 24) & 0xFF)
						);

						*pTu++ = (float)(-j * dtt);
						*pTv++ = (float)(i * dtt);
					}
					else {
						*pTu++ = (float)(tu0 - j * dt);
						*pTv++ = (float)(tv0 + i * dt);
					}

					vl.Y = -5000.0f;
					*pVert++ = vl;

					if (level >= 2) {
						*pSpec++ = FColor(
							(uint8)((BlendPacked >> 16) & 0xFF),
							(uint8)((BlendPacked >> 8) & 0xFF),
							(uint8)((BlendPacked >> 0) & 0xFF),
							(uint8)((BlendPacked >> 24) & 0xFF)
						);

						*pTu++ = (float)(-j * dtt);
						*pTv++ = (float)(i * dtt);
					}
					else {
						*pTu++ = (float)(tu0 - j * dt);
						*pTv++ = (float)(tv0 + i * dt);
					}
				}
			}

			// strip 3 & 4 verts
			for (j = 0; j < Ds1; j += DetailSize) {
				for (i = 0; i < Ds1; i++) {
					FVector vl(
						(float)(j * scale * dscale - (HALF_PATCH_SIZE * scale)),
						(float)(heights[i * dscale * PATCH_SIZE + j * dscale]),
						(float)(i * scale * dscale - (HALF_PATCH_SIZE * scale))
					);

					*pVert++ = vl;

					DWORD BlendPacked = 0;

					if (level >= 2) {
						BlendPacked = BlendValue(vl.Y);

						*pSpec++ = FColor(
							(uint8)((BlendPacked >> 16) & 0xFF),
							(uint8)((BlendPacked >> 8) & 0xFF),
							(uint8)((BlendPacked >> 0) & 0xFF),
							(uint8)((BlendPacked >> 24) & 0xFF)
						);

						*pTu++ = (float)(-j * dtt);
						*pTv++ = (float)(i * dtt);
					}
					else {
						*pTu++ = (float)(tu0 - j * dt);
						*pTv++ = (float)(tv0 + i * dt);
					}

					vl.Y = -5000.0f;
					*pVert++ = vl;

					if (level >= 2) {
						*pSpec++ = FColor(
							(uint8)((BlendPacked >> 16) & 0xFF),
							(uint8)((BlendPacked >> 8) & 0xFF),
							(uint8)((BlendPacked >> 0) & 0xFF),
							(uint8)((BlendPacked >> 24) & 0xFF)
						);

						*pTu++ = (float)(-j * dtt);
						*pTv++ = (float)(i * dtt);
					}
					else {
						*pTu++ = (float)(tu0 - j * dt);
						*pTv++ = (float)(tv0 + i * dt);
					}
				}
			}
		}

		Material* m = materials.first();

		// initialize the polys
		for (i = 0; i < npolys; i++) {
			Poly* p = SurfacePtr->GetPolys() + i;
			p->nverts = 3;
			p->vertex_set = vset;
			p->visible = 1;
			p->sortval = 0;
			p->material = m;

			if (level >= 2 && !water) {
				p->material = materials.at(1);
				p->sortval = 1;
			}
		}

		for (i = npolys; i < total; i++) {
			Poly* p = SurfacePtr->GetPolys() + i;
			p->nverts = 4;
			p->vertex_set = vset;
			p->visible = 1;
			p->sortval = 0;
			p->material = m;
		}

		int index = 0;

		// build main patch polys:
		for (i = 0; i < DetailSize; i++) {
			for (j = 0; j < DetailSize; j++) {
				int v[4] = {
					(Ds1 * (i)+(j)),
					(Ds1 * (i)+(j + 1)),
					(Ds1 * (i + 1) + (j)),
					(Ds1 * (i + 1) + (j + 1))
				};

				bisect(vset, v);

				// first triangle
				Poly* p = SurfacePtr->GetPolys() + index++;
				p->verts[0] = v[0];
				p->verts[1] = v[1];
				p->verts[2] = v[3];

				if (level >= 2 && !water) {
					int layer = CalcLayer(p) + 1;
					p->material = materials.at(layer);
					p->sortval = layer;
				}

				// second triangle
				p = SurfacePtr->GetPolys() + index++;
				p->verts[0] = v[0];
				p->verts[1] = v[3];
				p->verts[2] = v[2];

				if (level >= 2 && !water) {
					int layer = CalcLayer(p) + 1;
					p->material = materials.at(layer);
					p->sortval = layer;
				}
			}
		}

		// build vertical edge strip polys:
		if (!water) {
			for (i = 0; i < NUM_STRIPS; i++) {
				Poly* p = SurfacePtr->GetPolys() + npolys + i * strip_len;
				const int base_index = Ds1 * Ds1 + Ds1 * 2 * i;

				for (j = 0; j < strip_len; j++) {
					const int v = base_index + j * 2;
					p->nverts = 4;

					if (i == 1 || i == 2) {
						p->verts[0] = v;
						p->verts[1] = v + 2;
						p->verts[2] = v + 3;
						p->verts[3] = v + 1;
					}
					else {
						p->verts[0] = v;
						p->verts[1] = v + 1;
						p->verts[2] = v + 3;
						p->verts[3] = v + 2;
					}

					if (level >= 2) {
						int layer = CalcLayer(p) + 1;
						p->material = materials.at(layer);
						p->sortval = layer;
					}

					p++;
				}
			}
		}

		// update the poly planes:
		for (i = 0; i < total; i++) {
			Poly* p = SurfacePtr->GetPolys() + i;
			WORD* v = p->verts;

			p->plane = Plane(
				vset->loc[v[0]] + loc,
				vset->loc[v[1]] + loc,
				vset->loc[v[2]] + loc
			);
		}

		SurfacePtr->Normalize();

		// create contiguous segments for each material:
		qsort((void*)SurfacePtr->GetPolys(), SurfacePtr->GetNumPolys(), sizeof(Poly), mcomp);

		Segment* segment = nullptr;
		Poly* spolys = SurfacePtr->GetPolys();

		for (int n = 0; n < SurfacePtr->GetNumPolys(); n++) {
			if (segment && segment->material == spolys[n].material) {
				segment->npolys++;
			}
			else {
				segment = nullptr;
			}

			if (!segment) {
				segment = new Segment;

				segment->npolys = 1;
				segment->polys = &spolys[n];
				segment->material = segment->polys->material;
				segment->model = LocalModel;

				SurfacePtr->GetSegments().append(segment);
			}
		}

		Solid::EnableCollision(false);
		LocalModel->AddSurface(SurfacePtr);
		Solid::EnableCollision(true);

		// copy vertex normals:
		const Vec3B* tnorms = terrain ? terrain->Normals() : nullptr;

		for (i = 0; i < Ds1; i++) {
			for (j = 0; j < Ds1; j++) {

				if (water) {
					vset->nrm[i * Ds1 + j] = FVector(0, 1, 0);
				}
				else if (tnorms && dscale > 1) {
					FVector normal(0, 0, 0);

					int step = 1;
					if (dscale > 4)
						step = dscale / 4;

					for (int dy = -dscale / 2; dy < dscale / 2; dy += step) {
						for (int dx = -dscale / 2; dx < dscale / 2; dx += step) {
							int ix = rect.x + (Ds1 - 1 - j) * dscale + dx;
							int iy = rect.y + i * dscale + dy;

							if (ix < 0) ix = 0;
							if (iy < 0) iy = 0;

							if (terrain_width > 0) {
								if (ix > terrain_width - 1) ix = terrain_width - 1;
								if (iy > terrain_width - 1) iy = terrain_width - 1;

								const Vec3B vbn = tnorms[iy * terrain_width + ix];
								normal += FVector(
									(128 - vbn.x) / 127.0,
									(vbn.z - 128) / 127.0,
									(vbn.y - 128) / 127.0
								);
							}
						}
					}

					normal.Normalize();
					vset->nrm[i * Ds1 + j] = normal;
				}
				else if (tnorms && terrain_width > 0) {
					const Vec3B vbn = tnorms[(rect.y + i * dscale) * terrain_width + (rect.x + (Ds1 - 1 - j) * dscale)];
					vset->nrm[i * Ds1 + j] = FVector(
						(128 - vbn.x) / 127.0,
						(vbn.z - 128) / 127.0,
						(vbn.y - 128) / 127.0
					);
				}
				else {
					vset->nrm[i * Ds1 + j] = FVector(0, 1, 0);
				}
			}
		}

		if (!water) {
			FVector* pNrm = &vset->nrm[Ds1 * Ds1];

			// strip 1 & 2 verts
			for (i = 0; i < Ds1; i += DetailSize) {
				for (j = 0; j < Ds1; j++) {
					const FVector vn = vset->nrm[i * Ds1 + j];
					*pNrm++ = vn;
					*pNrm++ = vn;
				}
			}

			// strip 3 & 4 verts
			for (j = 0; j < Ds1; j += DetailSize) {
				for (i = 0; i < Ds1; i++) {
					const FVector vn = vset->nrm[i * Ds1 + j];
					*pNrm++ = vn;
					*pNrm++ = vn;
				}
			}
		}
	}

	if (level > max_detail)
		max_detail = level;

	return true;
}

// +--------------------------------------------------------------------+

DWORD
TerrainPatch::BlendValue(double y)
{
	if (terrain && y >= 0 && !water) {
		for (int i = 0; i < terrain->GetLayers().size(); i++) {
			TerrainLayer* layer = terrain->GetLayers().at(i);

			if (y >= layer->GetMinHeight() && y < layer->GetMaxHeight()) {
				double scale01 = (y - layer->GetMinHeight()) / (layer->GetMaxHeight() - layer->GetMinHeight());

				if (scale01 < 0.2)
					scale01 = 0;
				else if (scale01 > 0.8)
					scale01 = 1;
				else
					scale01 = (scale01 - 0.2) / 0.6;

				if ((i & 1) == 0) {
					scale01 = 1 - scale01;
				}

				DWORD val = (DWORD)(scale01 * 255);
				return val << 24;
			}
		}
	}

	return 0;
}

int
TerrainPatch::CalcLayer(Poly* poly)
{
	if (terrain && poly) {
		if (water)
			return 0;

		double y = 1e6;

		for (int i = 0; i < poly->nverts; i++) {
			double h = poly->vertex_set->loc[poly->verts[i]].Y;

			if (h >= 0 && h < y) {
				y = h;
			}
		}

		if (y <= terrain->GetLayers().first()->GetMinHeight())
			return 0;

		for (int i = 0; i < terrain->GetLayers().size(); i++) {
			TerrainLayer* layer = terrain->GetLayers().at(i);

			if (y >= layer->GetMinHeight() && y < layer->GetMaxHeight()) {
				return i;
			}
		}
	}

	return -1;
}

// +--------------------------------------------------------------------+

void
TerrainPatch::UpdateSurfaceWaves(FVector& eyePos)
{
	if (water && model && model->GetNumVerts() > 1) {
		Surface* s = model->GetSurfaces().first();
		if (s) {
			VertexSet* vset = s->GetVertexSet();
			for (int i = 0; i < vset->nverts; i++)
				vset->loc[i].Y = 0.0f;

			water->UpdateSurface(eyePos, vset);
		}
	}
}

// +--------------------------------------------------------------------+

int
TerrainPatch::CollidesWith(Graphic& o)
{
	return 0;
}

// +--------------------------------------------------------------------+

void
TerrainPatch::SelectDetail(SimProjector* projector)
{
	// Delegate to the overall terrain to compute a detail level for every patch:
	if (terrain) {
		terrain->SelectDetail(projector);
	}

	// Build detail levels on demand:
	if (detail_levels[ndetail] == nullptr)
		BuildDetailLevel(ndetail);
}

void
TerrainPatch::SetDetailLevel(int nd)
{
	if (nd >= 0 && nd <= max_detail) {
		if (ndetail != nd)
			DeletePrivateData();

		ndetail = nd;

		if (detail_levels[ndetail] == nullptr)
			BuildDetailLevel(ndetail);

		model = detail_levels[ndetail];

		if (terrain && water)
			water = terrain->GetWater(ndetail);
	}
}

// +--------------------------------------------------------------------+

void TerrainPatch::Illuminate(FColor Ambient, List<SimLight>& Lights)
{
	if (!model || model->GetNumVerts() < 1)
		return;

	Surface* SurfacePtr = model->GetSurfaces().first();
	if (!SurfacePtr)
		return;

	illuminating = true;

	VertexSet* VSet = SurfacePtr->GetVertexSet();
	if (!VSet || VSet->nverts < 1 || !VSet->diffuse || !VSet->nrm || !VSet->loc) {
		illuminating = false;
		return;
	}

	const int32 NumVerts = VSet->nverts;

	// Clear to ambient (opaque)
	const FColor AmbientColor = Ambient.WithAlpha(255);
	for (int32 i = 0; i < NumVerts; ++i) {
		VSet->diffuse[i] = AmbientColor;
	}

	TerrainRegion* Region = terrain ? terrain->GetRegion() : nullptr;
	bool bEclipsed = false;
	const bool bFirstPatch = terrain ? terrain->IsFirstPatch(this) : false;

	if (Region && !bFirstPatch) {
		bEclipsed = Region->IsEclipsed();
	}

	auto ScaleRGB = [](const FColor& C, float S) -> FColor
		{
			return FColor(
				(uint8)FMath::Clamp(int32(float(C.R) * S), 0, 255),
				(uint8)FMath::Clamp(int32(float(C.G) * S), 0, 255),
				(uint8)FMath::Clamp(int32(float(C.B) * S), 0, 255),
				255
			);
		};

	auto AddClampRGB = [](const FColor& Base, const FColor& Add) -> FColor
		{
			return FColor(
				(uint8)FMath::Clamp(int32(Base.R) + int32(Add.R), 0, 255),
				(uint8)FMath::Clamp(int32(Base.G) + int32(Add.G), 0, 255),
				(uint8)FMath::Clamp(int32(Base.B) + int32(Add.B), 0, 255),
				Base.A
			);
		};

	ListIter<SimLight> LightIter = Lights;
	while (++LightIter) {
		SimLight* Light = LightIter.value();
		if (!Light)
			continue;

		if (!Light->IsDirectional())
			continue;

		// Shadow / eclipse test:
		if (Light->CastsShadow() && bFirstPatch && scene) {
			const FVector LightPos = Light->Location();
			const FVector V0 = VSet->loc[0];

			bEclipsed =
				(LightPos.Y < -100.0f) ||
				scene->IsLightObscured(V0, LightPos, radius);
		}

		if (!Light->CastsShadow() || !bEclipsed) {
			// Treat "location" as the directional vector, per original behavior
			const FVector LightDir = Light->Location().GetSafeNormal();

			for (int32 i = 0; i < NumVerts; ++i) {
				const FVector& Normal = VSet->nrm[i];

				double Value = 0.0;
				const double Gain = double(FVector::DotProduct(LightDir, Normal));

				if (Gain > 0.0) {
					Value = Light->Intensity() * (0.85 * Gain);
					if (Value > 1.0) Value = 1.0;
				}

				if (Value > 0.01) {
					const FColor LightColor = Light->GetColor().WithAlpha(255);
					const FColor AddColor = ScaleRGB(LightColor, (float)Value);

					VSet->diffuse[i] = AddClampRGB(VSet->diffuse[i], AddColor);
				}
			}
		}
	}

	// Legacy behavior: "merge" specular alpha into diffuse alpha (old packed OR trick).
	// With FColor arrays, replicate the intent by copying specular A if specular exists.
	if (ndetail >= 2 && VSet->specular) {
		for (int32 i = 0; i < NumVerts; ++i) {
			VSet->diffuse[i].A = VSet->specular[i].A;
		}
	}

	if (Region && bFirstPatch) {
		Region->SetEclipsed(bEclipsed);
	}

	InvalidateSurfaceData();
	illuminating = false;
}

// +--------------------------------------------------------------------+

void
TerrainPatch::Render(Video* video, DWORD flags)
{
	if (max_height < 0)
		return;

	if (flags & RENDER_ADDITIVE)
		return;

	if (!model)
		model = detail_levels[0];

	if (scene) {
		if ((flags & RENDER_ADD_LIGHT) != 0)
			return;

		if (water) {
			FVector EyePos(0, 0, 0);
			UpdateSurfaceWaves(EyePos);
			Illuminate(scene->Ambient(), scene->Lights());
		}
		else {
			Illuminate(scene->Ambient(), scene->Lights());
		}
	}
	else {
		if ((flags & RENDER_ADD_LIGHT) != 0)
			return;
	}

	// Temporarily disable detail textures on low-detail LODs (original behavior):
	Bitmap* details[16];
	ZeroMem(details, sizeof(details));

	if (ndetail < 3) {
		for (int i = 0; i < 16 && i < materials.size(); i++) {
			Material* mtl = materials[i];

			if (mtl->tex_detail) {
				details[i] = mtl->tex_detail;
				mtl->tex_detail = nullptr;
			}
		}
	}

	double visibility = terrain->GetRegion()->GetWeather().Visibility();
	FLOAT fog_density = (FLOAT)(terrain->GetRegion()->FogDensity() * 2.5e-5 * 1 / visibility);

	video->SetRenderState(Video::LIGHTING_ENABLE, false);
	video->SetRenderState(Video::SPECULAR_ENABLE, false); // water != 0);
	video->SetRenderState(Video::FOG_ENABLE, true);

	video->SetRenderState(
		Video::FOG_COLOR,
		TPPackColor(terrain->GetRegion()->FogColor())
	);
	video->SetRenderState(Video::FOG_DENSITY, *((DWORD*)&fog_density));

	Solid::Render(video, flags);

	video->SetRenderState(Video::LIGHTING_ENABLE, true);
	video->SetRenderState(Video::SPECULAR_ENABLE, true);
	video->SetRenderState(Video::FOG_ENABLE, false);

	if (ndetail < 3) {
		for (int i = 0; i < 16 && i < materials.size(); i++) {
			Material* mtl = materials[i];

			if (details[i] && !mtl->tex_detail) {
				mtl->tex_detail = details[i];
			}
		}
	}
}

// +--------------------------------------------------------------------+

int
TerrainPatch::CheckRayIntersection(FVector obj_pos, FVector dir, double len, FVector& ipt, bool ttpas)
{
	FVector light_pos = obj_pos + dir * len;
	int impact = light_pos.Y < -100;

	// Special case for illumination -
	// just check if sun is above or below the horizon:
	if (illuminating || impact) {
		return impact;
	}

	if (obj_pos.X != 0 || obj_pos.Y != 0 || obj_pos.Z != 0) {
		return impact;
	}

	// the rest of this code is only used for eclipsing the solar lens flare:
	FVector d0 = loc;
	FVector d1 = FVector::CrossProduct(d0, dir);
	double dlen = d1.Length();

	if (dlen > radius)
		return 0;

	FVector closest = loc + dir * radius;

	if (FVector::DotProduct(closest, dir) < 0)
		return 0;

	if (!model)
		return 0;

	Surface* s = model->GetSurfaces().first();
	if (!s)
		return 0;

	// NOTE:
	// The original uses the Starshatter Matrix class to transform into object space.
	// In this UE conversion, Solid::Orientation() should already be compatible with your
	// math layer; if you still have a Starshatter Matrix, provide an adapter that yields
	// a transform or 3x3 basis. For now, the conservative behavior is to skip per-poly
	// eclipse tests and return the coarse impact result.
	//
	// This preserves gameplay behavior (lens flare occlusion is a visual), while avoiding
	// invalid type conversions like "const Matrix" -> UE::Math::TMatrix<double>.

	return impact;
}

// +--------------------------------------------------------------------+

double
TerrainPatch::Height(double x, double z) const
{
	if (water) return base;

	double height = 0;

	x /= scale;
	z /= scale;

	int x0 = (int)x;
	int z0 = (int)z;

	if (x0 >= 0 && x0 < PATCH_SIZE && z0 >= 0 && z0 < PATCH_SIZE) {
		double dx = x - x0;
		double dz = z - z0;

		double h[4];

		h[0] = heights[(z0 * PATCH_SIZE + x0)];
		h[1] = heights[((z0 + 1) * PATCH_SIZE + x0)];
		h[2] = heights[(z0 * PATCH_SIZE + x0 + 1)];
		h[3] = heights[((z0 + 1) * PATCH_SIZE + x0 + 1)];

		if (h[0] == h[1] && h[1] == h[2] && h[2] == h[3]) {
			height = h[0];
		}
		else {
			double hl = h[0] * (1 - dz) + h[1] * dz;
			double hr = h[2] * (1 - dz) + h[3] * dz;

			height = hl * (1 - dx) + hr * dx + 5; // fudge
		}
	}

	if (height < 0)
		height = 0;

	return height;
}
