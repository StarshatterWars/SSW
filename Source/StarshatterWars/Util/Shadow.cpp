/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         Shadow.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo, Destroyer Studios LLC

	OVERVIEW
	========
	Dynamic Stencil Shadow Volumes
*/

#include "Shadow.h"

#include "Types.h"
#include "Geometry.h"
#include "Color.h"

// Minimal Unreal includes:
#include "Math/Vector.h"
#include "Logging/LogMacros.h"

// Concrete type includes (forward-declared in header):
#include "Solid.h"
#include "SimModel.h"
#include "Video.h"
#include "SimLight.h"
#include "SimScene.h"
#include "GameStructs.h"

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterShadow, Log, All);

static bool visible_shadow_volumes = false;

// +--------------------------------------------------------------------+

Shadow::Shadow(Solid* s)
	: solid(s)
	, verts(nullptr)
	, nverts(0)
	, max_verts(0)
	, enabled(true)
	, edges(nullptr)
	, num_edges(0)
{
	if (solid && solid->GetModel()) {
		SimModel* model = solid->GetModel();
		const int npolys = model->GetNumPolys();

		max_verts = model->GetNumVerts() * 4;

		verts = new FVector[max_verts];
		edges = new WORD[npolys * 6];
	}
}

// +--------------------------------------------------------------------+

Shadow::~Shadow()
{
	delete[] verts;
	verts = nullptr;

	delete[] edges;
	edges = nullptr;
}

// +--------------------------------------------------------------------+

void Shadow::Reset()
{
	num_edges = 0;
	nverts = 0;
}

// +--------------------------------------------------------------------+

void Shadow::Render(Video* video)
{
	if (!video || !solid)
		return;

	if (enabled) {
		video->DrawShadow(solid, nverts, verts, visible_shadow_volumes);
	}
}

// +--------------------------------------------------------------------+

static inline FVector TransformVectorByBasisRows(const FVector& v, const Matrix& m)
{
	// Starshatter Matrix stores basis rows accessed via m(r,c).
	// Original code computed:
	//   out.x = v * Vec3(m00,m01,m02)  (dot with row 0)
	//   out.y = v * Vec3(m10,m11,m12)  (dot with row 1)
	//   out.z = v * Vec3(m20,m21,m22)  (dot with row 2)
	return FVector(
		(float)(v.X * m(0, 0) + v.Y * m(0, 1) + v.Z * m(0, 2)),
		(float)(v.X * m(1, 0) + v.Y * m(1, 1) + v.Z * m(1, 2)),
		(float)(v.X * m(2, 0) + v.Y * m(2, 1) + v.Z * m(2, 2))
	);
}

void Shadow::Update(SimLight* light)
{
	Reset();

	if (!light || !solid || !solid->GetModel() || !edges)
		return;

	const bool bDirectional = (light->Type() == LIGHTTYPE::DIRECTIONAL);
	SimModel* model = solid->GetModel();

	// Solid transform (UE port: matrix -> quat + vector):
	const FMatrix SolidMatrix = solid->Orientation();
	FQuat SolidRot(SolidMatrix);
	SolidRot.Normalize();

	const FVector SolidLoc = solid->Location();

	ListIter<Surface> surfIter = model->GetSurfaces();
	while (++surfIter)
	{
		Surface* surf = surfIter.value();
		if (!surf)
			continue;

		// -------------------------------------------------
		// Transform light vector into *solid local space*
		// -------------------------------------------------

		FVector LocalLight = FVector::ZeroVector;

		if (bDirectional)
		{
			// Directional: use direction from SimLight orientation.
			// Convention: forward vector points "where the light points".
			// For silhouette tests/extrusion we want a vector from surface toward light,
			// so we can use +/- as long as we're consistent. Keep it stable:
			const FVector LightDirWS = light->Direction();      // NEW API (from SimLight.h)
			LocalLight = SolidRot.UnrotateVector(LightDirWS);   // world -> solid local
		}
		else
		{
			// Point/Spot: use position relative to the surface
			FVector WorldLight = light->Location();
			WorldLight -= (SolidLoc + surf->GetOffset());       // bring into solid space w/ surf offset
			LocalLight = SolidRot.UnrotateVector(WorldLight);   // world -> solid local
		}

		// -------------------------------------------------
		// Build silhouette edges
		// -------------------------------------------------

		for (int32 i = 0; i < surf->GetNumPolys(); ++i)
		{
			Poly* p = surf->GetPolys() + i;
			if (!p)
				continue;

			// Skip polys with non-shadowing materials:
			if (p->material && !p->material->shadow)
				continue;

			// If this poly faces the light:
			// NOTE: assumes p->plane.normal is in the same local space as LocalLight.
			if (FVector::DotProduct(p->plane.normal, LocalLight) > 0.0f)
			{
				for (int32 n = 0; n < p->nverts; ++n)
				{
					const int32 n0 = n;
					const int32 n1 = (n + 1) % p->nverts;
					AddEdge(p->vlocs[n0], p->vlocs[n1]);
				}
			}
		}

		// -------------------------------------------------
		// Extrude shadow volume
		// -------------------------------------------------

		// Extrude away from the light vector in local space:
		FVector ExtrudeDir = -LocalLight;
		if (!ExtrudeDir.Normalize())
		{
			// Degenerate light vector; nothing to extrude safely
			continue;
		}

		ExtrudeDir *= 50.0e3f; // legacy scale preserved (50 km)

		for (int32 i = 0; i < (int32)num_edges; ++i)
		{
			if (nverts + 6 > max_verts)
			{
				UE_LOG(LogStarshatterShadow, Warning,
					TEXT("Shadow volume vertex buffer overflow (nverts=%d max=%d)"),
					nverts, max_verts);
				break;
			}

			// NOTE: Assumes Surface::GetVLoc() returns FVector* (Vec3->FVector conversion done).
			const int32 i0 = edges[2 * i + 0];
			const int32 i1 = edges[2 * i + 1];

			const FVector v1 = surf->GetVLoc()[i0];
			const FVector v2 = surf->GetVLoc()[i1];
			const FVector v3 = v1 + ExtrudeDir;
			const FVector v4 = v2 + ExtrudeDir;

			verts[nverts++] = v1;
			verts[nverts++] = v2;
			verts[nverts++] = v3;

			verts[nverts++] = v2;
			verts[nverts++] = v4;
			verts[nverts++] = v3;
		}
	}
}

// +--------------------------------------------------------------------+

void Shadow::AddEdge(WORD v1, WORD v2)
{
	// Remove interior edges (which appear in the list twice)
	for (DWORD i = 0; i < num_edges; i++) {
		if ((edges[2 * i + 0] == v1 && edges[2 * i + 1] == v2) ||
			(edges[2 * i + 0] == v2 && edges[2 * i + 1] == v1))
		{
			if (num_edges > 1) {
				edges[2 * i + 0] = edges[2 * (num_edges - 1) + 0];
				edges[2 * i + 1] = edges[2 * (num_edges - 1) + 1];
			}

			num_edges--;
			return;
		}
	}

	edges[2 * num_edges + 0] = v1;
	edges[2 * num_edges + 1] = v2;

	num_edges++;
}

// +--------------------------------------------------------------------+

bool Shadow::GetVisibleShadowVolumes()
{
	return visible_shadow_volumes;
}

void Shadow::SetVisibleShadowVolumes(bool vis)
{
	visible_shadow_volumes = vis;
}
