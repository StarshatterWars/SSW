/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         SimShot.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Laser and Missile class
*/

#pragma once

#include "Types.h"
#include "SimObject.h"
#include "Color.h"

// Minimal Unreal include for FVector (replaces Point/Vec3):
#include "Math/Vector.h"
#include "Math/Color.h"    // FColor

// +--------------------------------------------------------------------+

class Camera;
class Ship;
class Trail;
class SimSystem;      // renamed from System to avoid Unreal conflicts
class WeaponDesign;
class Sprite;
class USound;
class SimScene;
class Graphic;

// +--------------------------------------------------------------------+

class SimShot : public SimObject, public SimObserver
{
public:
	static const char* TYPENAME() { return "SimShot"; }

	SimShot(const FVector& pos, const Camera& cam, WeaponDesign* design, const Ship* ship = 0);
	virtual ~SimShot();

	virtual void       SeekTarget(SimObject* target, SimSystem* sub = 0);
	virtual void       ExecFrame(double factor);
	static void        Initialize();
	static void        Close();

	virtual void       Activate(SimScene& scene);
	virtual void       Deactivate(SimScene& scene);

	const Ship* Owner()        const { return owner; }
	double             Damage()       const;
	int                ShotType()     const { return type; }
	virtual SimObject* GetTarget()    const;

	virtual bool       IsPrimary()    const { return primary; }
	virtual bool       IsDrone()      const { return false; }
	virtual bool       IsDecoy()      const { return false; }
	virtual bool       IsProbe()      const { return false; }
	virtual bool       IsMissile()    const { return !primary; }
	virtual bool       IsArmed()      const { return armed; }
	virtual bool       IsBeam()       const { return beam; }
	virtual bool       IsFlak()       const;
	virtual bool       IsHostileTo(const SimObject* o) const;

	bool               HitTarget()    const { return hit_target; }
	void               SetHitTarget(bool h) { hit_target = h; }

	virtual bool       IsTracking(Ship* tgt) const;
	virtual double     PCS()          const { return 0; }
	virtual double     ACS()          const { return 0; }
	virtual int        GetIFF()       const;
	virtual FColor	   MarkerColor()  const;

	const FVector& Origin()       const { return origin; }
	float              Charge()       const { return charge; }
	void               SetCharge(float c);
	double             Length()       const;
	Graphic* GetTrail()     const { return (Graphic*)trail; }
	void               SetFuse(double seconds);

	void               SetBeamPoints(const FVector& from, const FVector& to);
	virtual void       Disarm();
	virtual void       Destroy();

	void              SetLife(int seconds) { life = seconds; }

	const WeaponDesign* Design()      const { return design; }
	const char* DesignName()   const;
	int                GetEta()       const { return eta; }
	void               SetEta(int t) { eta = (short)t; }

	double             AltitudeMSL()  const;
	double             AltitudeAGL()  const;

	// SimObserver interface:
	virtual bool         Update(SimObject* obj);
	virtual const char* GetObserverName() const;

	int operator == (const SimShot& s) const { return id == s.id; }

protected:
	const Ship* owner;

	int                type;
	float              base_damage;
	float              charge;
	float              offset;
	float              altitude_agl;
	short              eta;
	BYTE               iff_code;
	bool               first_frame;
	bool               primary;
	bool               beam;
	bool               armed;
	bool               hit_target;

	Sprite* flash;   // muzzle flash
	Sprite* flare;   // drive flare
	Trail* trail;   // exhaust trail

	USound* sound;
	WeaponDesign* design;

	// for beam weapons:
	FVector            origin;
};
