/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         Debris.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Debris Sprite class
*/

#pragma once

#include "Types.h"
#include "SimObject.h"

#include "Math/Vector.h"

// +--------------------------------------------------------------------+

class Solid;
class SimModel;
class SimShot;

// +--------------------------------------------------------------------+

class Debris : public SimObject
{
public:
	Debris(SimModel* model, const FVector& pos, const FVector& vel, double mass);

    void SetVisualModel(FName Model, double Radius, double Scale);
    double GetVisualScale() const { return VisualScale; }
    FName GetVisualModel() const { return VisualModel; }

private:
    FName VisualModel;
    double VisualScale = 1.0;

public:
	void              SetLife(int seconds) { life = seconds; }
	virtual int       HitBy(SimShot* shot, FVector& impact);

	virtual void      ExecFrame(double seconds);
	virtual double    AltitudeAGL() const;
};

