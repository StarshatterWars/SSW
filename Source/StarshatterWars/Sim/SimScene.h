/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         SimScene.h
	AUTHOR:       Carlos Bott
	ORIGINAL:     John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	A 3D Scene, basically a collection of 3D graphic objects
*/
#pragma once
#include "Types.h"
#include "Color.h"
#include "Geometry.h"
#include "List.h"

// Minimal Unreal include (required by request: convert Point/Vec3 to FVector):
#include "Math/Vector.h"

// +--------------------------------------------------------------------+
// Forward declarations (keep header light)

class UTexture2D;

class Graphic;
class SimLight;

// +--------------------------------------------------------------------+

class SimScene
{
public:
	static const char* TYPENAME() { return "SimScene"; }

	SimScene();
	virtual ~SimScene();

	void              AddBackground(Graphic* g);
	void              DelBackground(Graphic* g);
	void              AddForeground(Graphic* g);
	void              DelForeground(Graphic* g);
	void              AddGraphic(Graphic* g);
	void              DelGraphic(Graphic* g);
	void              AddSprite(Graphic* g);
	void              DelSprite(Graphic* g);

	void              AddLight(SimLight* l);
	void              DelLight(SimLight* l);

	List<Graphic>& Background() { return background; }
	List<Graphic>& Foreground() { return foreground; }
	List<Graphic>& Graphics() { return graphics; }
	List<Graphic>& Sprites() { return sprites; }
	List<SimLight>& Lights() { return lights; }
	FColor          Ambient() { return ambient; }
	void            SetAmbient(FColor a) { ambient = a; }

	virtual void      Collect();

	virtual bool IsLightObscured(
		UWorld* World,
		const FVector& obj_pos,
		const FVector& light_pos,
		double obj_radius,
		FVector* impact_point = nullptr) const;

	virtual bool IsLightObscured(
		const FVector& obj_pos,
		const FVector& light_pos,
		double obj_radius,
		FVector* impact_point = nullptr) const;

	static SimScene* emergency_scene;

protected:
	List<Graphic>     background;
	List<Graphic>     foreground;
	List<Graphic>     graphics;
	List<Graphic>     sprites;
	List<SimLight>    lights;
	FColor            ambient;
};


