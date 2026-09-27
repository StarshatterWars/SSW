/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         Shadow.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo, Destroyer Studios LLC

	OVERVIEW
	========
	Dynamic Stencil Shadow Volumes
*/

#pragma once

#include "Types.h"
#include "Color.h"
#include "Geometry.h"

// +--------------------------------------------------------------------+

// Prefer an inline-safe helper over macros, but keeping the original callsite
// semantics here (Destroy + null) without relying on __FILE__/__LINE__.
#define Shadow_DESTROY(x) if (x) { (x)->Destroy(); (x) = 0; }

// +--------------------------------------------------------------------+

class SimLight;
class SimScene;
class Solid;
class Video;

// +--------------------------------------------------------------------+

class Shadow
{
public:
	static const char* TYPENAME() { return "Shadow"; }

	Shadow(Solid* solid);
	virtual ~Shadow();

	int operator == (const Shadow& s) const { return this == &s; }

	// operations
	void     Render(Video* video);
	void     Update(SimLight* light);
	void     AddEdge(WORD v1, WORD v2);
	void     Reset();

	bool     IsEnabled()          const { return enabled; }
	void     SetEnabled(bool e) { enabled = e; }

	static void SetVisibleShadowVolumes(bool vis);
	static bool GetVisibleShadowVolumes();

protected:
	Solid*		solid;
	FVector*	verts;
	int			nverts;
	int			max_verts;
	bool		enabled;

	WORD*		edges;
	DWORD		num_edges;
};

