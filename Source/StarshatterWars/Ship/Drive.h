/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         Drive.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Conventional Drive (system) class
*/

#pragma once

#include "Types.h"
#include "SimSystem.h"

// NOTE:
// Geometry types (Point/Vec3) are migrated to Unreal's FVector.
// Keep includes minimal and explicit:
#include "Math/Vector.h"

// +--------------------------------------------------------------------+

class Bolt;
class DriveSprite;
class Light;
class USound;
class Ship;
class Physical;

// +--------------------------------------------------------------------+

struct DrivePort
{
	static const char* TYPENAME() { return "DrivePort"; }

	DrivePort(const FVector& InLoc, float InScale);
	~DrivePort();

	FVector       loc;
	float         scale;

	DriveSprite* flare;
	Bolt* trail;
};

// +--------------------------------------------------------------------+

class Drive : public SimSystem
{
public:
	enum SUBTYPE { PLASMA, FUSION, GREEN, RED, BLUE, YELLOW, STEALTH };
	enum Constants { MAX_ENGINES = 16 };

	Drive(SUBTYPE s, float max_thrust, float max_aug, bool show_trail = true);
	Drive(const Drive& rhs);
	virtual ~Drive();

	static void       Initialize();
	static void       Close();
	static void       StartFrame();

	float             Thrust(double seconds);
	float             MaxThrust()          const { return thrust; }
	float             MaxAugmenter()       const { return augmenter; }
	int               NumEngines()         const;
	DriveSprite* GetFlare(int port)   const;
	Bolt* GetTrail(int port)   const;
	bool              IsAugmenterOn()      const;

	virtual void      AddPort(const FVector& loc, float flare_scale = 0);
	virtual void      CreatePort(const FVector& loc, float flare_scale);

	virtual void      Orient(const Physical* rep);

	void              SetThrottle(double InThrottle, bool aug = false);
	virtual double    GetRequest(double seconds) const;

protected:
	float             thrust;
	float             augmenter;
	float             scale;
	float             throttle;
	float             augmenter_throttle;
	float             intensity;

	List<DrivePort>   ports;

	USound*			  sound;
	USound*			  burner_sound;
	bool              show_trail;
};
