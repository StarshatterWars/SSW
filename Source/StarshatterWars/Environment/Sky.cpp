/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         Sky.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Celestial sphere, stars, planets, space dust...
*/

#include "Sky.h"
#include "StarSystem.h"

#include "Game.h"
#include "DataLoader.h"
#include "SimLight.h"
#include "Random.h"
#include "SimModel.h"
#include "Segment.h"

#include "CoreMinimal.h"
#include "Math/Vector.h"

#include "Math/UnrealMathUtility.h"

// +====================================================================+

Stars::Stars(int nstars)
{
	infinite = true;
	luminous = true;
	shadow = false;

	vset = new VertexSet(nstars);
	colors = new FColor[nstars];

	for (int32 StarIndex = 0; StarIndex < nstars; ++StarIndex) {
		vset->loc[StarIndex] = RandomVector(1000);

		// Three independent uniform draws in [0.7, 0.8], converted to 0–255
		const uint8 GrayR = static_cast<uint8>(FMath::FRandRange(0.7f, 0.8f) * 255.0f);
		const uint8 GrayG = static_cast<uint8>(FMath::FRandRange(0.7f, 0.8f) * 255.0f);
		const uint8 GrayB = static_cast<uint8>(FMath::FRandRange(0.7f, 0.8f) * 255.0f);

		const FColor StarColor(GrayR, GrayG, GrayB, 255);

		colors[StarIndex] = StarColor;
		vset->diffuse[StarIndex] = StarColor;        // diffuse is FColor*
		vset->specular[StarIndex] = FColor(0, 0, 0, 255);// specular is FColor* (or FColor::Black if you prefer)
	}

	// If name is a fixed char[] buffer, use the safe form:
	strcpy_s(name, sizeof(name), "Stars");
}

Stars::~Stars()
{
	delete[] colors;
	delete   vset;
}

// +--------------------------------------------------------------------+

void
Stars::Illuminate(double Scale)
{
	if (!vset || !colors)
		return;

	const float ScaleF = static_cast<float>(Scale);

	for (int32 VertexIndex = 0; VertexIndex < vset->nverts; ++VertexIndex) {
		const FColor BaseColor = colors[VertexIndex];

		const uint8 R = static_cast<uint8>(
			FMath::Clamp(static_cast<float>(BaseColor.R) * ScaleF, 0.0f, 255.0f)
			);

		const uint8 G = static_cast<uint8>(
			FMath::Clamp(static_cast<float>(BaseColor.G) * ScaleF, 0.0f, 255.0f)
			);

		const uint8 B = static_cast<uint8>(
			FMath::Clamp(static_cast<float>(BaseColor.B) * ScaleF, 0.0f, 255.0f)
			);

		// Store directly as FColor (VertexSet now uses FColor*)
		vset->diffuse[VertexIndex] = FColor(R, G, B, BaseColor.A);
	}
}


// +--------------------------------------------------------------------+

void
Stars::Render(Video* video, DWORD flags)
{
	if (!vset || !video || (flags & Graphic::RENDER_ADDITIVE) == 0)
		return;

	video->SetBlendType(Video::BLEND_ADDITIVE);
	video->DrawPoints(vset);
}

// +====================================================================+

static const double BOUNDARY = 3000;
static const double BOUNDARYx2 = BOUNDARY * 2;

Dust::Dust(int ndust, bool b)
	: really_hidden(false), bright(b)
{
	radius = (float)BOUNDARYx2;
	luminous = true;
	vset = new VertexSet(ndust);

	Reset(FVector(0, 0, 0));
	strcpy_s(name, "Dust");
}

// +--------------------------------------------------------------------+

Dust::~Dust()
{
	delete vset;
}

// +--------------------------------------------------------------------+

void
Dust::Reset(const FVector& RefLocation)
{
	(void)RefLocation;

	for (int32 VertexIndex = 0; VertexIndex < vset->nverts; ++VertexIndex) {
		vset->loc[VertexIndex] = FVector(
			FMath::FRandRange(-BOUNDARY, BOUNDARY),
			FMath::FRandRange(-BOUNDARY, BOUNDARY),
			FMath::FRandRange(-BOUNDARY, BOUNDARY)
		);

		uint8 GrayValue = 0;

		if (bright) {
			GrayValue = static_cast<uint8>(FMath::RandRange(96, 200));
		}
		else {
			GrayValue = static_cast<uint8>(FMath::RandRange(64, 156));
		}

		// VertexSet stores FColor directly now:
		vset->diffuse[VertexIndex] = FColor(GrayValue, GrayValue, GrayValue, 255);

		// If specular is also FColor* in your port:
		vset->specular[VertexIndex] = FColor(0, 0, 0, 255);
		// If specular is still DWORD* in your port, keep:
		// vset->specular[VertexIndex] = 0;
	}
}

// +--------------------------------------------------------------------+

void
Dust::ExecFrame(double factor, const FVector& ref)
{
	(void)factor;

	if (Game::TimeCompression() > 4) {
		Hide();
		return;
	}

	Show();

	FVector delta = ref - loc;
	double  dlen = delta.Length();

	if (dlen < 0.0001)
		return;

	if (dlen > BOUNDARY) {
		Reset(ref);
	}
	else {
		// wrap around if necessary to keep in view
		for (int i = 0; i < vset->nverts; i++) {
			FVector v = vset->loc[i];

			v -= delta;

			if (v.X > BOUNDARY) v.X -= (double)BOUNDARYx2;
			if (v.X < -BOUNDARY) v.X += (double)BOUNDARYx2;
			if (v.Y > BOUNDARY) v.Y -= (double)BOUNDARYx2;
			if (v.Y < -BOUNDARY) v.Y += (double)BOUNDARYx2;
			if (v.Z > BOUNDARY) v.Z -= (double)BOUNDARYx2;
			if (v.Z < -BOUNDARY) v.Z += (double)BOUNDARYx2;

			vset->loc[i] = v;
		}
	}

	MoveTo(ref);
}

// +--------------------------------------------------------------------+

void
Dust::Render(Video* video, DWORD flags)
{
	if (hidden || really_hidden)
		return;

	if (!vset || !video || (flags & Graphic::RENDER_SOLID) == 0 || (flags & Graphic::RENDER_ADD_LIGHT) != 0)
		return;

	video->SetBlendType(Video::BLEND_SOLID);
	video->SetRenderState(Video::Z_ENABLE, false);
	video->SetRenderState(Video::Z_WRITE_ENABLE, false);

	video->DrawPoints(vset);

	video->SetRenderState(Video::Z_ENABLE, true);
	video->SetRenderState(Video::Z_WRITE_ENABLE, true);
}

// +--------------------------------------------------------------------+

void
Dust::Hide()
{
	hidden = true;
	really_hidden = true;
}

void
Dust::Show()
{
	hidden = false;
	really_hidden = false;
}

// +====================================================================+

PlanetRep::PlanetRep(const char* surface_name,
	const char* glow_name,
	double rad,
	const FVector& pos,
	double tscale,
	const char* rngname,
	double minrad,
	double maxrad,
	FColor atmos,
	const char* gloss_name)
	: mtl_surf(0),
	mtl_limb(0),
	mtl_ring(0),
	star_system(0)
{
	loc = pos;

	radius = (float)rad;
	has_ring = 0;
	ring_verts = -1;
	ring_polys = -1;
	ring_rad = 0;
	body_rad = rad;
	daytime = false;
	atmosphere = atmos;
	star_system = 0;

	if (!surface_name || !*surface_name) {
		UE_LOG(LogTemp, Warning, TEXT("PlanetRep: invalid planet patch - no surface texture specified"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("PlanetRep: constructing planet patch '%s'"), ANSI_TO_TCHAR(surface_name));
	strncpy(name, surface_name, 31);
	name[31] = 0;

	Bitmap* bmp_surf = 0;
	Bitmap* bmp_spec = 0;
	Bitmap* bmp_glow = 0;
	Bitmap* bmp_ring = 0;
	Bitmap* bmp_limb = 0;

	DataLoader* loader = DataLoader::GetLoader();
	loader->LoadTexture(surface_name, bmp_surf, Bitmap::BMP_SOLID, true);

	if (glow_name && *glow_name) {
		UE_LOG(LogTemp, Log, TEXT("PlanetRep: loading glow texture '%s'"), ANSI_TO_TCHAR(glow_name));
		loader->LoadTexture(glow_name, bmp_glow, Bitmap::BMP_SOLID, true);
	}

	if (gloss_name && *gloss_name) {
		UE_LOG(LogTemp, Log, TEXT("PlanetRep: loading gloss texture '%s'"), ANSI_TO_TCHAR(gloss_name));
		loader->LoadTexture(gloss_name, bmp_spec, Bitmap::BMP_SOLID, true);
	}

	mtl_surf = new Material;

	mtl_surf->Ka = FColor(192, 192, 192, 255);   // LightGray
	mtl_surf->Kd = FColor::White;
	mtl_surf->Ke = bmp_glow ? FColor::White : FColor::Black;
	mtl_surf->Ks = bmp_spec ? FColor(192, 192, 192, 255) : FColor::Black;
	mtl_surf->power = 25.0f;
	mtl_surf->tex_diffuse = bmp_surf;
	mtl_surf->tex_specular = bmp_spec;
	mtl_surf->tex_emissive = bmp_glow;
	mtl_surf->blend = Material::MTL_SOLID;

	if (bmp_spec && Video::GetInstance()->IsSpecMapEnabled()) {
		if (glow_name && strstr(glow_name, "light"))
			strcpy_s(mtl_surf->shader, "SimplePix/PlanetSurfNightLight");
		else if (glow_name)
			strcpy_s(mtl_surf->shader, "SimplePix/PlanetSurf");
	}

	if (atmosphere != FColor::Black) {
		mtl_limb = new Material;

		mtl_limb->Ka = atmosphere;

		strcpy_s(mtl_limb->shader, "PlanetLimb");

		UE_LOG(LogTemp, Log, TEXT("PlanetRep: loading atmospheric limb texture 'PlanetLimb.pcx'"));
		loader->LoadTexture("PlanetLimb.pcx", bmp_limb, Bitmap::BMP_TRANSLUCENT, true);

		mtl_limb->tex_diffuse = bmp_limb;
		mtl_limb->blend = Material::MTL_TRANSLUCENT;
	}

	if (maxrad > 0 && minrad > 0) {
		has_ring = 1;
		radius = (float)maxrad;
		ring_rad = (maxrad + minrad) / 2;

		loader->LoadTexture(rngname, bmp_ring, Bitmap::BMP_SOLID, true);

		mtl_ring = new Material;

		mtl_ring->Ka = FColor(192, 192, 192, 255);   // LightGray
		mtl_ring->Kd = FColor::White;
		mtl_ring->Ks = FColor(128, 128, 128, 255);   // Gray
		mtl_ring->Ke = FColor::Black;
		mtl_ring->power = 30.0f;
		mtl_ring->tex_diffuse = bmp_ring;
		mtl_ring->blend = Material::MTL_TRANSLUCENT;
	}

	if (rad > 2e6 && rad < 1e8)
		CreateSphere(rad, 24, 32, minrad, maxrad, 48, tscale);
	else
		CreateSphere(rad, 16, 24, minrad, maxrad, 48, tscale);
}

// +--------------------------------------------------------------------+

PlanetRep::~PlanetRep()
{
	// Ownership of materials/model is consistent with original engine conventions.
	// If your Unreal port assigns ownership elsewhere, clean up accordingly.
}

// +--------------------------------------------------------------------+

void
PlanetRep::CreateSphere(double InRadius,
	int nrings,
	int nsections,
	double minrad,
	double maxrad,
	int rsections,
	double tscale)
{
	const int sect_verts = nsections + 1;

	model = new SimModel;
	own_model = 1;

	Surface* surface = new Surface;

	int i, j, m, n;

	int npolys = (nrings + 2) * nsections;
	int nverts = (nrings + 3) * sect_verts;

	int ppolys = npolys;
	int pverts = nverts;

	int apolys = 0;
	int averts = 0;

	if (atmosphere != FColor::Black) {
		apolys = npolys;
		averts = nverts;

		npolys *= 2;
		nverts *= 2;
	}

	if (has_ring) {
		ring_verts = nverts;
		ring_polys = npolys;

		npolys += rsections * 3;   // top, bottom, edge
		nverts += rsections * 6;
	}

	surface->SetName(name);
	surface->CreateVerts(nverts);
	surface->CreatePolys(npolys);

	VertexSet* vset = surface->GetVertexSet();

	if (!vset || vset->nverts < nverts) {
		UE_LOG(LogTemp, Warning, TEXT("PlanetRep: insufficient memory for planet '%s'"), ANSI_TO_TCHAR(name));
		return;
	}

	Poly* polys = surface->GetPolys();

	if (!polys) {
		UE_LOG(LogTemp, Warning, TEXT("PlanetRep: insufficient memory for planet '%s'"), ANSI_TO_TCHAR(name));
		return;
	}

	ZeroMemory(polys, sizeof(Poly) * npolys);

	// Generate vertex points for planetary rings:
	double dtheta = PI / (nrings + 2);
	double dphi = 2 * PI / nsections;
	double theta = 0;
	n = 0; // vertex being generated

	for (i = 0; i < nrings + 3; i++) {
		double y = InRadius * cos(theta);  // y is the same for entire ring
		double v = theta / PI;             // v is the same for entire ring
		double rsintheta = InRadius * sin(theta);
		double phi = 0;

		for (j = 0; j < sect_verts; j++) {
			double x = rsintheta * sin(phi);
			double z = rsintheta * cos(phi);

			vset->loc[n] = FVector(x, y, z);
			vset->nrm[n] = FVector(x, y, z);
			vset->tu[n] = (float)(tscale * (1 - (phi / (2.0 * PI))));
			vset->tv[n] = (float)(tscale * v);

			vset->nrm[n].Normalize();

			phi += dphi;
			n++;
		}

		theta += dtheta;
	}

	// Generate vertex points for rings:
	if (has_ring) {
		n = ring_verts;

		double dphi_ring = 2.0 * PI / rsections;
		double y = 0;  // y is the same for entire ring

		// top of ring:
		double phi = 0;
		for (j = 0; j < rsections; j++) {
			double x = minrad * sin(phi);
			double z = minrad * cos(phi);

			vset->loc[n] = FVector(x, y, z);
			vset->nrm[n] = FVector(0, 1, 0);
			vset->tu[n] = (j & 1) ? 1.0f : 0.0f;
			vset->tv[n] = 0.0f;
			n++;

			x = maxrad * sin(phi);
			z = maxrad * cos(phi);

			vset->loc[n] = FVector(x, y, z);
			vset->nrm[n] = FVector(0, 1, 0);
			vset->tu[n] = (j & 1) ? 1.0f : 0.0f;
			vset->tv[n] = 1.0f;
			n++;

			phi += dphi_ring;
		}

		// bottom of ring:
		phi = 0;
		for (j = 0; j < rsections; j++) {
			double x = minrad * sin(phi);
			double z = minrad * cos(phi);

			vset->loc[n] = FVector(x, y, z);
			vset->nrm[n] = FVector(0, -1, 0);
			vset->tu[n] = (j & 1) ? 1.0f : 0.0f;
			vset->tv[n] = 0.0f;
			n++;

			x = maxrad * sin(phi);
			z = maxrad * cos(phi);

			vset->loc[n] = FVector(x, y, z);
			vset->nrm[n] = FVector(0, -1, 0);
			vset->tu[n] = (j & 1) ? 1.0f : 0.0f;
			vset->tv[n] = 1.0f;
			n++;

			phi += dphi_ring;
		}

		// edge of ring:
		phi = 0;
		for (j = 0; j < rsections; j++) {
			double x = maxrad * sin(phi);
			double z = maxrad * cos(phi);

			FVector normal(x, 0, z);
			normal.Normalize();

			double thickness = maxrad / 333;

			vset->loc[n] = FVector(x, y + thickness, z);
			vset->nrm[n] = normal;
			vset->tu[n] = (j & 1) ? 1.0f : 0.0f;
			vset->tv[n] = 1.0f;
			n++;

			vset->loc[n] = FVector(x, y - thickness, z);
			vset->nrm[n] = normal;
			vset->tu[n] = (j & 1) ? 1.0f : 0.0f;
			vset->tv[n] = 1.0f;
			n++;

			phi += dphi_ring;
		}
	}

	for (i = 0; i < npolys; i++) {
		polys[i].nverts = 3;
		polys[i].vertex_set = vset;
		polys[i].material = mtl_surf;
	}

	// Generate triangles for top and bottom caps.
	for (i = 0; i < nsections; i++) {
		Poly& p0 = polys[i];
		p0.verts[2] = i;
		p0.verts[1] = sect_verts + i;
		p0.verts[0] = sect_verts + ((i + 1) % sect_verts);

		Poly& p1 = polys[ppolys - nsections + i];
		p1.verts[2] = pverts - 1 - i;
		p1.verts[1] = pverts - 1 - sect_verts - i;
		p1.verts[0] = pverts - 2 - sect_verts - i;

		surface->AddIndices(6);
	}

	// Generate triangles for the planetary rings
	m = sect_verts;  // first vertex in current ring
	n = nsections;   // triangle being generated, skip the top cap

	for (i = 0; i < nrings; i++) {
		for (j = 0; j < nsections; j++) {
			Poly& p0 = polys[n];
			p0.nverts = 4;
			p0.verts[3] = m + j;
			p0.verts[2] = m + (sect_verts)+j;
			p0.verts[1] = m + (sect_verts)+((j + 1) % (sect_verts));
			p0.verts[0] = m + ((j + 1) % (sect_verts));
			n++;

			surface->AddIndices(6);
		}

		m += sect_verts;
	}

	if (averts && apolys && mtl_limb) {
		for (i = 0; i < pverts; i++) {
			vset->loc[averts + i] = vset->loc[i];
			vset->nrm[averts + i] = vset->nrm[i];
		}

		for (i = 0; i < ppolys; i++) {
			Poly& p0 = polys[i];
			Poly& p1 = polys[apolys + i];

			p1.vertex_set = vset;
			p1.material = mtl_limb;

			p1.nverts = p0.nverts;
			p1.verts[0] = p0.verts[0];
			p1.verts[1] = p0.verts[1];
			p1.verts[2] = p0.verts[2];
			p1.verts[3] = p0.verts[3];

			surface->AddIndices(p1.nverts == 3 ? 3 : 6);
		}
	}

	if (has_ring) {
		// Generate quads for the rings
		m = ring_verts; // first vertex in top of ring, after planet verts
		n = ring_polys; // quad being generated, after planet polys

		// top of ring:
		for (j = 0; j < rsections; j++) {
			Poly& p0 = polys[n];
			p0.nverts = 4;
			p0.material = mtl_ring;

			p0.verts[3] = m + 2 * j;
			p0.verts[2] = m + 2 * j + 1;
			p0.verts[1] = m + ((2 * j + 3) % (rsections * 2));
			p0.verts[0] = m + ((2 * j + 2) % (rsections * 2));

			surface->AddIndices(6);
			n++;
		}

		// bottom of ring:
		m = ring_verts + 2 * rsections; // first vertex in bottom of ring, after top ring verts

		for (j = 0; j < rsections; j++) {
			Poly& p0 = polys[n];
			p0.nverts = 4;
			p0.material = mtl_ring;

			p0.verts[0] = m + 2 * j;
			p0.verts[1] = m + 2 * j + 1;
			p0.verts[2] = m + ((2 * j + 3) % (rsections * 2));
			p0.verts[3] = m + ((2 * j + 2) % (rsections * 2));

			surface->AddIndices(6);
			n++;
		}

		// edge of ring:
		m = ring_verts + 4 * rsections; // first vertex in edge of ring, after bottom ring verts

		for (j = 0; j < rsections; j++) {
			Poly& p0 = polys[n];
			p0.nverts = 4;
			p0.material = mtl_ring;

			p0.verts[3] = m + 2 * j;
			p0.verts[2] = m + 2 * j + 1;
			p0.verts[1] = m + ((2 * j + 3) % (rsections * 2));
			p0.verts[0] = m + ((2 * j + 2) % (rsections * 2));

			surface->AddIndices(6);
			n++;
		}
	}

	// then assign them to cohesive segments:
	Segment* segment = 0;

	for (n = 0; n < npolys; n++) {
		Poly& poly = polys[n];

		poly.plane = Plane(vset->loc[poly.verts[0]],
			vset->loc[poly.verts[2]],
			vset->loc[poly.verts[1]]);

		if (segment && segment->material == polys[n].material) {
			segment->npolys++;
		}
		else {
			segment = new Segment;

			segment->npolys = 1;
			segment->polys = &polys[n];
			segment->material = segment->polys->material;

			surface->GetSegments().append(segment);
		}
	}

	model->AddSurface(surface);
}

int
PlanetRep::CheckRayIntersection(FVector Q, FVector w, double len, FVector& ipt,
	bool treat_translucent_polys_as_solid)
{
	(void)ipt;
	(void)treat_translucent_polys_as_solid;

	// compute leading edge of ray:
	FVector dst = Q + w * len;

	// check right angle spherical distance:
	FVector d0 = loc - Q;
	FVector d1 = FVector::CrossProduct(d0, w);
	double  dlen = d1.Length();          // distance of point from line

	if (dlen > body_rad)                 // clean miss
		return 0;                      // (no impact)

	// possible collision course...
	FVector d2 = Q + w * (FVector::DotProduct(d0, w));

	// so check the leading edge:
	FVector delta0 = dst - loc;

	if (delta0.Length() > radius) {
		// and the endpoints:
		FVector delta1 = d2 - Q;
		FVector delta2 = dst - Q;

		// if d2 is not between Q and dst, we missed:
		if (FVector::DotProduct(delta1, delta2) < 0 ||
			delta1.Length() > delta2.Length()) {
			return 0;
		}
	}

	return 1;
}

void
PlanetRep::SetDaytime(bool d)
{
	daytime = d;

	if (daytime) {
		if (mtl_surf) mtl_surf->blend = Material::MTL_ADDITIVE;
		if (mtl_ring) mtl_ring->blend = Material::MTL_ADDITIVE;
	}
	else {
		if (mtl_surf) mtl_surf->blend = Material::MTL_SOLID;
		if (mtl_ring) mtl_ring->blend = Material::MTL_TRANSLUCENT;
	}
}

void
PlanetRep::SetStarSystem(StarSystem* system)
{
	star_system = system;
}

// +--------------------------------------------------------------------+

void
PlanetRep::Render(Video* video, DWORD flags)
{
	Solid::Render(video, flags);

	/***
	*** DEBUG
	***

	Matrix orient  = Orientation();
	orient.Transpose();

	video->SetObjTransform(orient, Location());

	Surface* surf  = model->GetSurfaces().first();
	Poly*    polys = surf->GetPolys();

	for (int i = 0; i < 5; i++)
		video->DrawPolyOutline(polys + i);
	***/
}
