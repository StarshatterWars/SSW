/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         Sky.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Celestial sphere, stars, planets, space dust...
*/

#pragma once

#include "Types.h"
#include "Solid.h"
#include "Geometry.h"
#include "Bitmap.h"

// Forward declarations to keep headers light:
class UTexture2D;

class StarSystem;

class Video;
struct Material;
struct VertexSet;

// +--------------------------------------------------------------------+

class Stars : public Graphic
{
public:
	Stars(int nstars);
	virtual ~Stars();

	virtual void Illuminate(double scale);
	virtual void Render(Video* video, DWORD flags);

protected:
	VertexSet* vset;
	FColor* colors;
};

// +--------------------------------------------------------------------+

class Dust : public Graphic
{
public:
	Dust(int ndust, bool bright = false);
	virtual ~Dust();

	virtual void Render(Video* video, DWORD flags);
	virtual void Reset(const FVector& ref);
	virtual void ExecFrame(double factor, const FVector& ref);

	virtual void Hide();
	virtual void Show();

protected:
	bool      really_hidden;
	bool      bright;
	VertexSet* vset;
};

// +--------------------------------------------------------------------+

class PlanetRep : public Solid
{
public:
	PlanetRep(const char* img_west,
		const char* img_glow,
		double rad,
		const FVector& pos,
		double tscale = 1,
		const char* rngname = 0,
		double minrad = 0,
		double maxrad = 0,
		FColor atmos = FColor::Black,
		const char* img_gloss = 0);

	virtual ~PlanetRep();

	virtual FColor Atmosphere() const { return atmosphere; }
	virtual void  SetAtmosphere(FColor a) { atmosphere = a; }
	virtual void  SetDaytime(bool d);
	virtual void  SetStarSystem(StarSystem* system);

	virtual void Render(Video* video, DWORD flags);

	virtual int CheckRayIntersection(FVector pt,
		FVector vpn,
		double len,
		FVector& ipt,
		bool treat_translucent_polys_as_solid = true);

protected:
	void CreateSphere(double radius,
		int nrings,
		int nsections,
		double minrad,
		double maxrad,
		int rsections,
		double tscale);

	Material* mtl_surf;
	Material* mtl_limb;
	Material* mtl_ring;

	int    has_ring;
	int    ring_verts;
	int    ring_polys;
	double ring_rad;
	double body_rad;

	FColor atmosphere;
	bool  daytime;

	StarSystem* star_system;
};
