/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         ShieldRep.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	ShieldRep Solid class
/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         ShieldRep.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	ShieldRep Solid class
*/

#include "ShieldRep.h"

// UE logging (minimal include):
#include "Logging/LogMacros.h"

// Minimal Unreal include required by Vec3/Point -> FVector conversion:
#include "Math/Vector.h"

// Starshatter core / project headers:
#include "Random.h"
#include "Game.h"
#include "SimLight.h"
#include "Solid.h"
#include "SimModel.h"
#include "Bitmap.h"
#include "Color.h"
#include "DataLoader.h"

// If you don't already have a log category, define a local one for this TU:
DEFINE_LOG_CATEGORY_STATIC(LogShieldRep, Log, All);

// +--------------------------------------------------------------------+

static const int MAX_SHIELD_HITS = 16;

// +--------------------------------------------------------------------+

struct ShieldHit
{
	FVector  hitloc = FVector::ZeroVector;
	double   damage = 0.0;
	double   age = 0.0;
	SimShot* shot = nullptr;
};

// +--------------------------------------------------------------------+

ShieldRep::ShieldRep()
{
	bubble = false;
	luminous = true;
	trans = true;
	nhits = 0;

	hits = new ShieldHit[MAX_SHIELD_HITS];
}

ShieldRep::~ShieldRep()
{
	delete[] hits;
	hits = nullptr;
}

// +--------------------------------------------------------------------+

void ShieldRep::Hit(FVector Impact, SimShot* Shot, double Damage)
{
	if (!model || model->GetSurfaces().size() < 1)
		return;

	// --------------------------------------------------
	// Transform impact into object (local) space
	// --------------------------------------------------

	// World -> local offset
	const FVector LocalDelta = Impact - Location();

	// Orientation() assumed to be a Starshatter-style orientation matrix
	// converted to an Unreal-compatible FMatrix wrapper.
	const FMatrix XForm = Orientation();

	// Extract basis vectors explicitly (ROW-MAJOR)
	const FVector XAxis(XForm.M[0][0], XForm.M[0][1], XForm.M[0][2]);
	const FVector YAxis(XForm.M[1][0], XForm.M[1][1], XForm.M[1][2]);
	const FVector ZAxis(XForm.M[2][0], XForm.M[2][1], XForm.M[2][2]);

	Impact.X = FVector::DotProduct(LocalDelta, XAxis);
	Impact.Y = FVector::DotProduct(LocalDelta, YAxis);
	Impact.Z = FVector::DotProduct(LocalDelta, ZAxis);

	// --------------------------------------------------
	// Find slot to store the hit
	// --------------------------------------------------

	int Slot = -1;
	double OldestAge = -1.0;

	// Reuse slot if this shot already exists
	for (int i = 0; i < MAX_SHIELD_HITS; i++)
	{
		if (hits[i].shot == Shot)
		{
			Slot = i;
			break;
		}
	}

	// Otherwise find empty or oldest slot
	if (Slot < 0)
	{
		for (int i = 0; i < MAX_SHIELD_HITS; i++)
		{
			if (hits[i].damage <= 0.0)
			{
				Slot = i;
				break;
			}

			if (hits[i].age > OldestAge)
			{
				Slot = i;
				OldestAge = hits[i].age;
			}
		}
	}

	// --------------------------------------------------
	// Record hit
	// --------------------------------------------------

	if (Slot >= 0 && Slot < MAX_SHIELD_HITS)
	{
		hits[Slot].hitloc = Impact;
		hits[Slot].damage = Damage;
		hits[Slot].age = 1.0;
		hits[Slot].shot = Shot;

		if (nhits < MAX_SHIELD_HITS)
			nhits++;
	}
}

// +--------------------------------------------------------------------+

void
ShieldRep::Energize(double seconds, bool b)
{
	bubble = b;

	if (nhits < 1)
		return;

	nhits = 0;

	for (int i = 0; i < MAX_SHIELD_HITS; i++) {
		if (hits[i].damage > 0) {
			// age the hit:
			hits[i].age += seconds;
			hits[i].damage -= (hits[i].damage * 4.0 * seconds);

			// collect garbage:
			if (hits[i].damage < 10) {
				hits[i].age = 0;
				hits[i].damage = 0;
				hits[i].shot = nullptr;
			}
			else {
				nhits++;
			}
		}
	}
}

// +--------------------------------------------------------------------+

void
ShieldRep::TranslateBy(const FVector& ref)
{
	true_eye_point = ref;
	Solid::TranslateBy(ref);
}

// +--------------------------------------------------------------------+

void
ShieldRep::Illuminate()
{
	if (!model)
		return;

	Surface* surf = model->GetSurfaces().first();
	if (!surf)
		return;

	VertexSet* vset = surf->GetVertexSet();
	if (!vset || vset->nverts < 1)
		return;

	const int nverts = vset->nverts;

	// Clear lighting buffers (FColor*)
	for (int i = 0; i < nverts; i++)
	{
		vset->diffuse[i] = FColor(0, 0, 0, 0);
		vset->specular[i] = FColor(0, 0, 0, 0);
	}

	if (nhits < 1)
		return;

	double all_damage = 0.0;

	// --------------------------------------------------
	// Non-bubble: add hit glow contribution per-vertex
	// --------------------------------------------------
	for (int i = 0; i < MAX_SHIELD_HITS; i++)
	{
		if (hits[i].damage <= 0.0)
			continue;

		const FVector HitLoc = hits[i].hitloc;
		double HitDam = hits[i].damage * 2000.0;

		all_damage += hits[i].damage;

		if (!bubble)
		{
			const double limit = (double)radius * (double)radius;
			if (HitDam > limit)
				HitDam = limit;

			for (int v = 0; v < nverts; v++)
			{
				// vset->loc[v] may be FVector OR UE::Math::TVector<double>.
				// Convert explicitly to FVector to avoid operator mismatches:
				const FVector VLoc(
					(float)vset->loc[v].X,
					(float)vset->loc[v].Y,
					(float)vset->loc[v].Z
				);

				double dist = (VLoc - HitLoc).Length();

				if (dist < 1.0)
					dist = 1.0;
				else
					dist = pow(dist, 2.7);

				const double pert = (double)FMath::FRandRange(0.1f, 1.5f);
				const double intensity = pert * HitDam / dist;

				if (intensity > 0.003)
				{
					const float AddF = (float)FMath::Clamp(intensity, 0.0, 1.0);
					const uint8 AddU = (uint8)FMath::Clamp<int32>((int32)(AddF * 255.0f), 0, 255);

					const FColor Base = vset->diffuse[v];

					const uint8 R = (uint8)FMath::Min(255, (int32)Base.R + (int32)AddU);
					const uint8 G = (uint8)FMath::Min(255, (int32)Base.G + (int32)AddU);
					const uint8 B = (uint8)FMath::Min(255, (int32)Base.B + (int32)AddU);

					vset->diffuse[v] = FColor(R, G, B, 255);
				}
			}
		}
	}

	// --------------------------------------------------
	// Bubble: rim-light effect (view vector vs normal)
	// --------------------------------------------------
	if (bubble)
	{
		double shield_gain = 1.0;
		if (all_damage < 1000.0)
			shield_gain = all_damage / 1000.0;

		// Solid::Orientation() returns const FMatrix& (per your Solid.h)
		const FMatrix& orientation = Orientation();

		// Basis vectors from matrix rows:
		const FVector XAxis((float)orientation.M[0][0], (float)orientation.M[0][1], (float)orientation.M[0][2]);
		const FVector YAxis((float)orientation.M[1][0], (float)orientation.M[1][1], (float)orientation.M[1][2]);
		const FVector ZAxis((float)orientation.M[2][0], (float)orientation.M[2][1], (float)orientation.M[2][2]);

		const FVector WorldOrigin = Location();

		for (int i = 0; i < nverts; i++)
		{
			const FVector L(
				(float)vset->loc[i].X,
				(float)vset->loc[i].Y,
				(float)vset->loc[i].Z
			);

			const FVector N(
				(float)vset->nrm[i].X,
				(float)vset->nrm[i].Y,
				(float)vset->nrm[i].Z
			);

			// Rotate local -> world (row-dot semantics consistent with Hit)
			const FVector RotLoc(
				FVector::DotProduct(L, XAxis),
				FVector::DotProduct(L, YAxis),
				FVector::DotProduct(L, ZAxis)
			);

			const FVector RotNrm(
				FVector::DotProduct(N, XAxis),
				FVector::DotProduct(N, YAxis),
				FVector::DotProduct(N, ZAxis)
			);

			const FVector vloc = RotLoc + WorldOrigin;
			const FVector vnrm = RotNrm.GetSafeNormal();

			const FVector V = (-vloc).GetSafeNormal();

			double intensity = 1.0 - (double)FVector::DotProduct(V, vnrm);

			if (intensity > 0.0)
			{
				intensity *= intensity;
				if (intensity > 1.0)
					intensity = 1.0;

				intensity *= (shield_gain * (double)FMath::FRandRange(0.75f, 1.0f));

				const float I = (float)FMath::Clamp(intensity, 0.0, 1.0);
				const uint8 U = (uint8)FMath::Clamp<int32>((int32)(I * 255.0f), 0, 255);

				vset->diffuse[i] = FColor(U, U, U, 255);
			}
		}
	}

	InvalidateSurfaceData();
}

void
ShieldRep::Render(Video* video, DWORD flags)
{
	if ((flags & RENDER_ADDITIVE) == 0)
		return;

	if (nhits > 0) {
		Illuminate();
		Solid::Render(video, RENDER_ALPHA); // have to lie about the render flag
		// or the engine will reject the solid
	}
}
