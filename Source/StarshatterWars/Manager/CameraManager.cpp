/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         CameraManager.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Camera Manager singleton manages the main view camera based on the current ship
	(Renamed from CameraDirector to avoid Unreal Engine filename collisions)
*/

#include "CameraManager.h"

#include "Ship.h"
#include "FlightDeck.h"
#include "SimContact.h"
#include "Sim.h"
#include "StarSystem.h"
#include "Terrain.h"
#include "HUDView.h"
#include "DetailSet.h"
#include "Starshatter.h"
#include "Game.h"

// Minimal Unreal types used by this implementation:
#include "Math/UnrealMathUtility.h"   // FMath::Sin/Cos/Abs, PI
#include "Math/Vector.h"             // FVector

// +----------------------------------------------------------------------+

CameraManager* CameraManager::instance = 0;

static double range_max_limit = 300e3;

// +----------------------------------------------------------------------+

CameraManager*
CameraManager::GetInstance()
{
	if (!instance)
		instance = new CameraManager;

	return instance;
}

// +----------------------------------------------------------------------+

CameraManager::CameraManager()
	: mode(MODE_COCKPIT),
	requested_mode(MODE_NONE),
	old_mode(MODE_NONE),
	camera(),
	ship(0),
	region(0),
	external_body(0),
	external_ship(0),
	external_point(FVector::ZeroVector),
	base_loc(FVector::ZeroVector),
	virt_az(0),
	virt_el(0),
	virt_x(0),
	virt_y(0),
	virt_z(0),
	azimuth(PI / 4),
	elevation(PI / 4),
	az_rate(0),
	el_rate(0),
	range_rate(0),
	range(0),
	range_min(100),
	range_max(range_max_limit),
	base_range(0),
	transition(0),
	sim(0),
	hud(0)
{
	instance = this;
}

// +--------------------------------------------------------------------+

CameraManager::~CameraManager()
{
	if (instance == this)
		instance = 0;
}

// +--------------------------------------------------------------------+

void
CameraManager::Reset()
{
	mode = MODE_COCKPIT;
	requested_mode = MODE_NONE;
	old_mode = MODE_NONE;

	sim = 0;
	ship = 0;
	region = 0;
	external_ship = 0;
	external_body = 0;
	transition = 0;
	hud = 0;
}

// +--------------------------------------------------------------------+

void
CameraManager::SetShip(Ship* s)
{
	sim = Sim::GetSim();
	hud = HUDView::GetInstance();

	// can't take control of a dead ship:
	if (s && (s->Life() == 0 || s->IsDying() || s->IsDead()))
		return;

	// leaving the old ship, so make sure it is visible:
	if (ship && ship != s) {
		ship->ShowRep();
		ship->HideCockpit();
	}

	// taking control of the new ship:
	ship = s;

	if (ship) {
		Observe(ship);
		region = ship->GetRegion();

		if (sim && ship->GetRegion() != sim->GetActiveRegion())
			sim->ActivateRegion(ship->GetRegion());

		range = ship->Radius() * 4;

		if (mode == MODE_COCKPIT)
			mode = MODE_CHASE;

		SetMode(MODE_COCKPIT);
		ExecFrame(0);
	}
}

void
CameraManager::SetMode(int m, double t)
{
	if (requested_mode == m)
		return;

	external_point = FVector::ZeroVector;

	// save current mode for after transition:
	if (m == MODE_DROP && mode != MODE_DROP)
		old_mode = mode;

	// if manually leaving drop mode, forget about restoring the previous mode:
	else if (m != MODE_DROP && mode == MODE_DROP)
		old_mode = MODE_NONE;

	if (m == MODE_VIRTUAL && ship && !ship->Cockpit())
		return;

	if (mode == m) {
		if (mode == MODE_TARGET || mode == MODE_ORBIT)
			CycleViewObject();
		return;
	}

	if (m > MODE_NONE && m < MODE_LAST) {
		if (m <= MODE_VIRTUAL) {
			requested_mode = m;
			transition = t;
			external_ship = nullptr;
			ClearGroup();

			// no easy way to do a smooth transition between certain modes, so go immediately:
			if ((mode == MODE_TARGET && m == MODE_CHASE) ||
				(mode == MODE_COCKPIT && m == MODE_VIRTUAL) ||
				(mode == MODE_VIRTUAL && m == MODE_COCKPIT))
			{
				mode = m;
				requested_mode = 0;
				transition = 0;
			}
		}
		else if (m == MODE_TRANSLATE || m == MODE_ZOOM) {
			requested_mode = m;
			transition = t;
			base_range = range;
		}
		else if (m >= MODE_DOCKING) {
			mode = m;
			transition = 0;
			external_ship = nullptr;
			ClearGroup();
		}

		virt_az = 0;
		virt_el = 0;
		virt_x = 0;
		virt_y = 0;
		virt_z = 0;
	}
}

// +--------------------------------------------------------------------+

static const char* get_camera_mode_name(int m)
{
	switch (m) {
	default:
	case CameraManager::MODE_NONE:      return "";
	case CameraManager::MODE_COCKPIT:   return "";
	case CameraManager::MODE_CHASE:     return "Chase Cam";
	case CameraManager::MODE_TARGET:    return "Padlock";
	case CameraManager::MODE_THREAT:    return "Threatlock";
	case CameraManager::MODE_VIRTUAL:   return "Virtual";
	case CameraManager::MODE_ORBIT:
	case CameraManager::MODE_TRANSLATE:
	case CameraManager::MODE_ZOOM:      return "Orbit Cam";
	case CameraManager::MODE_DOCKING:   return "Dock Cam";
	case CameraManager::MODE_DROP:      return "";
	}
}

const char*
CameraManager::GetModeName()
{
	const int m = GetCameraMode();

	if (m != CameraManager::MODE_VIRTUAL) {
		return get_camera_mode_name(m);
	}
	else if (instance) {
		if (instance->virt_az > 30 * DEGREES && instance->virt_az < 100 * DEGREES)
			return "RIGHT";
		else if (instance->virt_az >= 100 * DEGREES)
			return "RIGHT-AFT";
		else if (instance->virt_az < -30 * DEGREES && instance->virt_az > -100 * DEGREES)
			return "LEFT";
		else if (instance->virt_az <= -100 * DEGREES)
			return "LEFT-AFT";
		else if (instance->virt_el > 15 * DEGREES)
			return "UP";
		else if (instance->virt_el < -15 * DEGREES)
			return "DOWN";

		return get_camera_mode_name(m);
	}

	return "";
}

int
CameraManager::GetCameraMode()
{
	if (instance) {
		int op_mode = instance->mode;
		if (instance->requested_mode > instance->mode)
			op_mode = instance->requested_mode;
		return op_mode;
	}

	return 0;
}

void
CameraManager::SetCameraMode(int m, double t)
{
	if (instance)
		instance->SetMode(m, t);
}

// +--------------------------------------------------------------------+

void
CameraManager::ClearGroup()
{
	external_group.clear();
}

// +--------------------------------------------------------------------+

void
CameraManager::CycleViewObject()
{
	if (!ship) return;

	Ship* const Current = external_ship;
	external_ship = nullptr;

	ListIter<SimContact> iter = ship->ContactList();
	while (++iter && !external_ship) {
		SimContact* c = iter.value();
		Ship* c_ship = c ? c->GetShip() : nullptr;

		if (c_ship && !Current) {
			external_ship = c_ship;
		}
		else if (Current && c_ship == Current) {
			while (++iter && !external_ship) {
				c = iter.value();
				if (c && c->ActLock())
					external_ship = c->GetShip();
			}
		}
	}

	if (external_ship != Current) {
		if (external_ship) {
			if (external_ship->Life() == 0 || external_ship->IsDying() || external_ship->IsDead()) {
				external_point = external_ship->Location(); // FVector in UE port
				external_ship = nullptr;
			}
			else {
				Observe(external_ship);
			}
		}

		if (mode == MODE_ORBIT) {
			SetMode(MODE_TRANSLATE);
			ExternalRange(1);
		}
	}
}

// +--------------------------------------------------------------------+

void
CameraManager::SetViewOrbital(Orbital* orb)
{
	external_body = orb;

	if (external_body) {
		range_min = external_body->Radius() * 2.5;
		ClearGroup();
		external_ship = 0;

		if (sim) {
			region = sim->FindNearestSpaceRegion(orb);
			if (region)
				sim->ActivateRegion(region);
		}

		if (ship && !region)
			region = ship->GetRegion();
	}
}

// +--------------------------------------------------------------------+

void
CameraManager::SetViewObject(Ship* obj, bool quick)
{
	if (!ship) return;

	if (!obj) {
		obj = ship;
		region = ship->GetRegion();
	}

	external_body = nullptr;
	external_point = FVector::ZeroVector;

	Starshatter* stars = Starshatter::GetInstance();

	if (obj->GetIFF() != ship->GetIFF() && !stars->InCutscene()) {
		// only view solid contacts:
		SimContact* c = ship->FindContact(obj);
		if (!c || !c->ActLock())
			return;
	}

	if (mode == MODE_TARGET) {
		ClearGroup();
		external_ship = nullptr;
	}
	else if (mode >= MODE_ORBIT) {
		if (quick) {
			mode = MODE_ORBIT;
			transition = 0;
		}
		else {
			SetMode(MODE_TRANSLATE);
		}

		if (external_group.size()) {
			ClearGroup();
			external_ship = nullptr;
		}
		else {
			if ((obj == external_ship) || (obj == ship && external_ship == nullptr)) {
				if (!quick)
					SetMode(MODE_ZOOM);
			}
			else if (external_ship) {
				external_ship = nullptr;
			}
		}
	}

	if (external_ship != obj) {
		external_ship = obj;

		if (external_ship) {
			region = external_ship->GetRegion();

			if (external_ship->Life() == 0 || external_ship->IsDying() || external_ship->IsDead()) {
				external_ship = nullptr;
				range_min = 100;
			}
			else {
				Observe(external_ship);

				if (sim)
					sim->ActivateRegion(external_ship->GetRegion());

				range_min = external_ship->Radius() * 1.5;
			}
		}

		Observe(external_ship);
		ExternalRange(1);
	}
}

// +--------------------------------------------------------------------+

bool
CameraManager::Update(SimObject* obj)
{
	if (!obj)
		return SimObserver::Update(obj);

	if (obj->Type() == SimObject::SIM_SHIP) {
		Ship* s = (Ship*)obj;

		if (ship == s)
			ship = nullptr;

		if (external_ship == s) {
			external_point = s->Location(); // FVector
			external_ship = nullptr;
		}

		if (external_group.contains(s))
			external_group.remove(s);
	}

	return SimObserver::Update(obj);
}

// +--------------------------------------------------------------------+

void
CameraManager::ExecFrame(double seconds)
{
	if (!ship)
		return;

	hud = HUDView::GetInstance();

	const int flight_phase = ship->GetFlightPhase();

	if (flight_phase < Ship::LOCKED)
		SetMode(MODE_DOCKING);

	if (ship->IsAirborne()) {
		if (flight_phase >= Ship::DOCKING)
			SetMode(MODE_DOCKING);
	}
	else {
		if (flight_phase >= Ship::RECOVERY)
			SetMode(MODE_DOCKING);
	}

	if (flight_phase >= Ship::LOCKED && flight_phase < Ship::ACTIVE) {
		const int m = GetMode();
		if (m != MODE_COCKPIT && m != MODE_VIRTUAL)
			SetMode(MODE_COCKPIT);
	}

	if (ship->InTransition()) {
		SetMode(MODE_DROP);
	}
	// automatically restore mode after transition:
	else if (old_mode != MODE_NONE) {
		mode = old_mode;
		requested_mode = old_mode;
		old_mode = MODE_NONE;
	}

	int op_mode = mode;
	if (requested_mode > mode)
		op_mode = requested_mode;

	Starshatter* stars = Starshatter::GetInstance();

	// if we are in padlock, and have not locked a ship, try to padlock the current target:
	if (op_mode == MODE_TARGET && !external_ship) {
		SimObject* tgt = ship->GetTarget();
		if (tgt && tgt->Type() == SimObject::SIM_SHIP)
			SetViewObject((Ship*)tgt);
	}
	// if in an external mode, check the external ship:
	else if (op_mode >= MODE_TARGET && op_mode <= MODE_ZOOM) {
		if (external_ship && external_ship != ship && !stars->InCutscene()) {
			SimContact* c = ship->FindContact(external_ship);
			if (!c || !c->ActLock()) {
				SetViewObject(ship);
			}
		}
	}

	if (ship->Rep()) {
		if (op_mode == MODE_COCKPIT) {
			ship->HideRep();
			ship->HideCockpit();
		}
		else if (op_mode == MODE_VIRTUAL || op_mode == MODE_TARGET) {
			if (ship->Cockpit()) {
				ship->HideRep();
				ship->ShowCockpit();
			}
			else {
				ship->ShowRep();
			}
		}
		else {
			ship->Rep()->SetForeground(op_mode == MODE_DOCKING);
			ship->ShowRep();
			ship->HideCockpit();
		}
	}

	if (hud && hud->Ambient() != FColor::Black)
		sim->GetScene()->SetAmbient(hud->Ambient());
	else
		sim->GetScene()->SetAmbient(sim->GetStarSystem()->Ambient());

	switch (op_mode) {
	default:
	case MODE_COCKPIT:      Cockpit(seconds);    break;
	case MODE_CHASE:        Chase(seconds);      break;
	case MODE_TARGET:       Target(seconds);     break;
	case MODE_THREAT:       Threat(seconds);     break;
	case MODE_VIRTUAL:      Virtual(seconds);    break;
	case MODE_ORBIT:
	case MODE_TRANSLATE:
	case MODE_ZOOM:         Orbit(seconds);      break;
	case MODE_DOCKING:      Docking(seconds);    break;
	case MODE_DROP:         Drop(seconds);       break;
	}

	if (ship->Shake() > 0 &&
		(op_mode < MODE_ORBIT || (op_mode == MODE_VIRTUAL && ship->Cockpit())))
	{
		const FVector Vib = ship->Vibration() * 0.2f;
		camera.MoveBy(Vib);
		camera.Aim(0, Vib.Y, Vib.Z);
	}

	Transition(seconds);

	{
		const FVector Cam = camera.Pos();
		DetailSet::SetReference(region, Cam);
	}
}

// +--------------------------------------------------------------------+

void
CameraManager::Cockpit(double seconds)
{
	camera.Clone(ship->Cam());

	const FVector Bridge = ship->BridgeLocation();

	const FVector Cpos =
		camera.Pos() +
		camera.vrt() * Bridge.X +
		camera.vpn() * Bridge.Y +
		camera.vup() * Bridge.Z;

	camera.MoveTo(Cpos);
}

// +--------------------------------------------------------------------+

void
CameraManager::Virtual(double seconds)
{
	camera.Clone(ship->Cam());

	const FVector Bridge = ship->BridgeLocation();

	const FVector Cpos =
		camera.Pos() +
		camera.vrt() * (float)(Bridge.X + virt_x) +
		camera.vpn() * (float)(Bridge.Y + virt_z) +
		camera.vup() * (float)(Bridge.Z + virt_y);

	camera.MoveTo(Cpos);

	camera.Yaw(virt_az);
	camera.Pitch(-virt_el);

	const double fvaz = FMath::Abs(virt_az);
	const double fvel = FMath::Abs(virt_el);

	if (fvaz > 0.01 * DEGREES && fvaz < 15 * DEGREES) {
		const double bleed = fvaz * 2;
		virt_az += (virt_az > 0 ? -1.0 : 1.0) * bleed * seconds;
	}

	if (fvel > 0.01 * DEGREES && fvel < 15 * DEGREES) {
		const double bleed = fvel;
		virt_el += (virt_el > 0 ? -1.0 : 1.0) * bleed * seconds;
	}
}

// +--------------------------------------------------------------------+

void
CameraManager::Chase(double seconds)
{
	double step = 1.0;

	if (requested_mode == MODE_COCKPIT)
		step = transition;
	else if (requested_mode == MODE_CHASE)
		step = 1.0 - transition;

	camera.Clone(ship->Cam());

	FVector VelocityDir = camera.vpn();

	const FVector ShipVel = ship->Velocity();
	if (ShipVel.Length() > 10.0f) {
		VelocityDir = ShipVel.GetSafeNormal();
		VelocityDir *= 0.25f;
		VelocityDir += camera.vpn() * 2.0f;
		VelocityDir = VelocityDir.GetSafeNormal();
	}

	const FVector Chase = ship->ChaseLocation();   // UE: return FVector
	const FVector Bridge = ship->BridgeLocation();  // UE: return FVector

	const FVector Cpos =
		camera.Pos() +
		camera.vrt() * (Bridge.X * (float)(1.0 - step)) +
		camera.vpn() * (Bridge.Y * (float)(1.0 - step)) +
		camera.vup() * (Bridge.Z * (float)(1.0 - step)) +
		VelocityDir * (Chase.Y * (float)step) +
		camera.vup() * (Chase.Z * (float)step);

	camera.MoveTo(Cpos);
}

// +--------------------------------------------------------------------+
void
CameraManager::Target(double seconds)
{
	// If external_point is already an FVector in your UE port, just use it.
	// If it's still a legacy Point type, ensure it can convert to FVector, or keep this explicit mapping.
	FVector TargetLoc(external_point.X, external_point.Y, external_point.Z);

	if (external_ship) {
		TargetLoc = external_ship->Location();
	}

	// If we have no meaningful external target (or it's our own ship), fall back:
	if (!external_ship || external_ship == ship) {
		if (external_point.IsZero()) {
			if (ship && ship->Cockpit())
				Virtual(seconds);
			else
				Orbit(seconds);

			return;
		}
	}

	// transition step:
	double Step = 1.0;

	if (requested_mode == MODE_COCKPIT)
		Step = transition;
	else if (requested_mode == MODE_TARGET)
		Step = 1.0 - transition;

	if (ship && ship->Cockpit()) {
		// internal padlock:
		Cockpit(seconds);

		// If camera.Padlock takes FVector and radians, this is fine.
		// If it takes legacy Point, convert TargetLoc back to that type (but prefer FVector in UE).
		camera.Padlock(TargetLoc, 3.0 * PI / 4.0, PI / 8.0, PI / 3.0);
	}
	else {
		// external padlock:
		const FVector ShipLoc = ship ? ship->Location() : FVector::ZeroVector;

		FVector Delta = TargetLoc - ShipLoc;
		Delta = Delta.GetSafeNormal(); // UE-safe normalize (handles zero length)

		const double Radius = ship ? ship->Radius() : 0.0;

		Delta *= (float)(-5.0 * Radius * Step);
		Delta.Y += (float)(Radius * Step);

		camera.MoveTo(ShipLoc + Delta);
		camera.LookAt(TargetLoc);
	}
}


// +--------------------------------------------------------------------+

void
CameraManager::Threat(double seconds)
{
	Chase(seconds);
}

// +--------------------------------------------------------------------+

void
CameraManager::Orbit(double seconds)
{
	// UE: clamp delta-time
	if (seconds < 0.0)        seconds = 0.0;
	else if (seconds > 0.2)   seconds = 0.2;

	// Use UE vectors for spatial math in this function:
	FVector Cpos = ship ? ship->Location() : FVector::ZeroVector;
	const int32 OpMode = GetCameraMode();
	(void)OpMode; // retained for parity / potential future use

	// auto rotate
	azimuth += az_rate * seconds;
	elevation += el_rate * seconds;
	range *= (1.0 + range_rate * seconds);

	// Determine orbit focus + min range
	if (external_body && external_body->Rep()) {
		range_min = external_body->Radius() * 2.5;
		Cpos = external_body->Rep()->Location();
	}
	else if (external_group.size()) {
		FVector Neg(1e9, 1e9, 1e9);
		FVector Pos(-1e9, -1e9, -1e9);

		ListIter<Ship> iter(external_group);
		while (++iter) {
			const FVector Loc = iter->Location();

			if (Loc.X < Neg.X) Neg.X = Loc.X;
			if (Loc.X > Pos.X) Pos.X = Loc.X;
			if (Loc.Y < Neg.Y) Neg.Y = Loc.Y;
			if (Loc.Y > Pos.Y) Pos.Y = Loc.Y;
			if (Loc.Z < Neg.Z) Neg.Z = Loc.Z;
			if (Loc.Z > Pos.Z) Pos.Z = Loc.Z;
		}

		const double dx = (double)(Pos.X - Neg.X);
		const double dy = (double)(Pos.Y - Neg.Y);
		const double dz = (double)(Pos.Z - Neg.Z);

		// UE: compute max extent cleanly
		const double maxExtent = FMath::Max(dx, FMath::Max(dy, dz));
		range_min = maxExtent * 1.2;

		// focus on median location:
		Cpos = Neg + FVector((float)(dx * 0.5), (float)(dy * 0.5), (float)(dz * 0.5));
	}
	else if (external_ship) {
		range_min = external_ship->Radius() * 1.5;
		Cpos = external_ship->Location();
	}
	else if (ship) {
		range_min = ship->Radius() * 1.5;
		Cpos = ship->Location();
	}

	if (range < range_min)
		range = range_min;

	double er = range;
	double az = azimuth;
	double el = elevation;

	// MODE_TRANSLATE: blend focus point toward base_loc
	if (requested_mode == MODE_TRANSLATE) {
		Cpos = (Cpos * (float)(1.0 - transition)) + (base_loc * (float)transition);
	}
	else if (requested_mode == MODE_ZOOM) {
		er = base_range * transition;

		if (er < range_min) {
			er = range_min;
			range = range_min;
			transition = 0;
			requested_mode = 0;
		}
	}
	// transitions:
	else if (mode < MODE_ORBIT || (requested_mode > 0 && requested_mode < MODE_ORBIT)) {
		double az0 = ship ? ship->CompassHeading() : 0.0;
		if (FMath::Abs(az - az0) > PI) az0 -= 2.0 * PI;

		double r0 = 0.0;
		double z0 = 0.0;

		if (ship &&
			(mode == MODE_CHASE || requested_mode == MODE_CHASE ||
				mode == MODE_TARGET || requested_mode == MODE_TARGET)) {
			r0 = (double)ship->ChaseLocation().Length();
			z0 = 20.0 * DEGREES;
		}

		// pull out:
		if (mode < MODE_ORBIT) {
			er *= (1.0 - transition);
			az *= (1.0 - transition);
			el *= (1.0 - transition);

			er += r0 * transition;
			az += az0 * transition;
			el += z0 * transition;
		}
		// push in:
		else {
			er *= transition;
			az *= transition;
			el *= transition;

			er += r0 * (1.0 - transition);
			az += az0 * (1.0 - transition);
			el += z0 * (1.0 - transition);
		}
	}
	else {
		// save base location for next time we re-focus
		base_loc = Cpos;
	}

	// UE: use trig with FVector output
	const double dx = er * FMath::Sin(az) * FMath::Cos(el);
	const double dy = er * FMath::Cos(az) * FMath::Cos(el);
	const double dz = er * FMath::Sin(el);

	// NOTE: Original mapping was Point(dx, dz, dy) (swapped Y/Z for engine coords)
	FVector Cloc = Cpos + FVector((float)dx, (float)dz, (float)dy);

	// Terrain AGL clamp (keep original axis usage: Height(x, z) and compare against .Y)
	Terrain* terrain = (ship && ship->GetRegion()) ? ship->GetRegion()->GetTerrain() : nullptr;
	if (terrain) {
		const double ground = terrain->Height((double)Cloc.X, (double)Cloc.Z);
		const double cam_agl = (double)Cloc.Y - ground;

		if (cam_agl < 100.0)
			Cloc.Y = (float)(ground + 100.0);
	}

	if (!external_ship && ship && ship->Rep() && er < 0.5 * ship->Radius())
		ship->Rep()->Hide();

	// camera API assumed Starshatter-style; feed UE-space values
	camera.MoveTo((double)Cloc.X, (double)Cloc.Y, (double)Cloc.Z);
	camera.LookAt(Cpos);
}

// +--------------------------------------------------------------------+

void
CameraManager::Docking(double seconds)
{
	FlightDeck* dock = ship ? ship->GetDock() : nullptr;

	if (!dock) {
		Cockpit(seconds);
		return;
	}

	// If you still have legacy sim/scene/color plumbing, keep it.
	// Otherwise, replace with your UE scene/PPV/lighting pipeline later.
	if (ship && !ship->IsAirborne() && sim && sim->GetScene()) {
		sim->GetScene()->SetAmbient(FColor(120, 130, 140));
	}

	const int32 flight_phase = ship ? ship->GetFlightPhase() : 0;

	// --- Bridge focus point (look-at) ---
	// Convert Point math to FVector math locally for UE compatibility.
	const FVector Bridge = ship ? ship->BridgeLocation() : FVector::ZeroVector;

	// NOTE: Cam().vrt/vpn/vup are assumed to return axis vectors (legacy camera basis).
	// If they return Point/Vec3, ensure implicit conversion or update those accessors to FVector.
	const FVector ShipLoc = ship ? ship->Location() : FVector::ZeroVector;

	const FVector Vrt = ship ? ship->Cam().vrt() : FVector::RightVector;
	const FVector Vpn = ship ? ship->Cam().vpn() : FVector::ForwardVector;
	const FVector Vup = ship ? ship->Cam().vup() : FVector::UpVector;

	const FVector Cloc = ShipLoc
		+ (Vrt * Bridge.X)
		+ (Vpn * Bridge.Y)
		+ (Vup * Bridge.Z);

	// --- Camera position (from flight deck) ---
	FVector Cpos = dock->CamLoc();

	// preflight:
	if (flight_phase < Ship::LOCKED) {
		base_loc = Cpos;
	}
	else if (flight_phase == Ship::LOCKED) {
		if (hud)
			hud->SetHUDMode(EHUDMode::Tactical);

		// NOTE: preserve original lerp direction:
		// cpos = base*transition + cloc*(1-transition)
		Cpos = (base_loc * (float)transition) + (Cloc * (float)(1.0 - transition));
	}
	// recovery:
	else if (flight_phase > Ship::APPROACH) {
		if (hud)
			hud->SetTacticalMode(1);
	}

	// If your camera API has FVector overloads, use them:
	camera.MoveTo(Cpos);
	camera.LookAt(Cloc);
}
// +--------------------------------------------------------------------+

void
CameraManager::Drop(double seconds)
{
	// orbital transitions use "drop cam" at transition_loc
	camera.MoveTo(ship->TransitionLocation());
	camera.LookAt(ship->Location());
}

// ---------------------------------------------------------------------
// Static range limit accessors (legacy parity)
// ---------------------------------------------------------------------

double CameraManager::GetRangeLimit()
{
	return range_max_limit;
}

void CameraManager::SetRangeLimit(double r)
{
	if (r >= 1e3)
		range_max_limit = r;
}

void CameraManager::SetRangeLimits(double min, double max)
{
	if (instance) {
		instance->range_min = min;
		instance->range_max = max;
	}
}

// ---------------------------------------------------------------------
// View centered check (legacy parity)
// ---------------------------------------------------------------------

bool CameraManager::IsViewCentered()
{
	if (instance) {
		const double fvaz = FMath::Abs(instance->virt_az);
		const double fvel = FMath::Abs(instance->virt_el);
		return fvaz < 15 * DEGREES && fvel < 15 * DEGREES;
	}

	return true;
}

// ---------------------------------------------------------------------
// Virtual head / padlock controls (legacy parity)
// ---------------------------------------------------------------------

void CameraManager::VirtualHead(double az, double el)
{
	if (mode == MODE_VIRTUAL || mode == MODE_TARGET || mode == MODE_COCKPIT) {
		const double alimit = 3 * PI / 4;
		const double e_lo = PI / 8;
		const double e_hi = PI / 3;
		const double escale = e_hi;

		virt_az = az * alimit;
		virt_el = el * escale;

		if (virt_az > alimit) virt_az = alimit;
		if (virt_az < -alimit) virt_az = -alimit;

		if (virt_el > e_hi)   virt_el = e_hi;
		if (virt_el < -e_lo)   virt_el = -e_lo;
	}
}

void CameraManager::VirtualHeadOffset(double x, double y, double z)
{
	if (mode == MODE_VIRTUAL || mode == MODE_TARGET || mode == MODE_COCKPIT) {
		virt_x = x;
		virt_y = y;
		virt_z = z;
	}
}

void CameraManager::VirtualAzimuth(double delta)
{
	if (mode == MODE_VIRTUAL || mode == MODE_TARGET || mode == MODE_COCKPIT) {
		virt_az += delta;

		const double alimit = 3 * PI / 4;
		if (virt_az > alimit) virt_az = alimit;
		if (virt_az < -alimit) virt_az = -alimit;
	}
}

void CameraManager::VirtualElevation(double delta)
{
	if (mode == MODE_VIRTUAL || mode == MODE_TARGET || mode == MODE_COCKPIT) {
		virt_el += delta;

		const double e_lo = PI / 8;
		const double e_hi = PI / 3;

		if (virt_el > e_hi) virt_el = e_hi;
		if (virt_el < -e_lo) virt_el = -e_lo;
	}
}

// ---------------------------------------------------------------------
// External orbit controls (legacy parity)
// ---------------------------------------------------------------------

void CameraManager::ExternalAzimuth(double delta)
{
	azimuth += delta;

	if (azimuth > PI)  azimuth = -2 * PI + azimuth;
	if (azimuth < -PI)  azimuth = 2 * PI + azimuth;
}

void CameraManager::ExternalElevation(double delta)
{
	elevation += delta;

	const double limit = (0.43 * PI);

	if (elevation > limit) elevation = limit;
	if (elevation < -limit) elevation = -limit;
}

void CameraManager::ExternalRange(double delta)
{
	range *= delta;

	if (ship && ship->IsAirborne())
		range_max = 30e3;
	else
		range_max = range_max_limit;

	if (range < range_min) range = range_min;
	if (range > range_max) range = range_max;
}

void CameraManager::SetOrbitPoint(double a, double e, double r)
{
	azimuth = a;
	elevation = e;
	range = r;

	if (external_body) {
		const double r2 = external_body->Radius() * 2.0;
		const double r6 = external_body->Radius() * 6.0;

		if (range < r2) range = r2;
		if (range > r6) range = r6;
	}
	else {
		if (range < range_min) range = range_min;
		if (range > range_max) range = range_max;
	}
}

void CameraManager::SetOrbitRates(double ar, double er, double rr)
{
	az_rate = ar;
	el_rate = er;
	range_rate = rr;
}

// ---------------------------------------------------------------------
// Group viewing (legacy parity)
// ---------------------------------------------------------------------

void CameraManager::SetViewObjectGroup(ListIter<Ship> group, bool quick)
{
	if (!ship)
		return;

	Starshatter* stars = Starshatter::GetInstance();

	if (!stars->InCutscene()) {
		// only view solid contacts:
		while (++group) {
			Ship* s = group.value();
			if (!s)
				return;

			if (s->GetIFF() != ship->GetIFF()) {
				SimContact* c = ship->FindContact(s);
				if (!c || !c->ActLock())
					return;
			}

			if (s->Life() == 0 || s->IsDying() || s->IsDead())
				return;
		}
	}

	group.reset();

	if (external_group.size() > 1 &&
		external_group.size() == group.size()) {

		bool same = true;

		for (int i = 0; same && i < external_group.size(); i++) {
			if (external_group[i] != group.container()[i])
				same = false;
		}

		if (same) {
			SetMode(MODE_ZOOM);
			return;
		}
	}

	ClearGroup();

	if (quick) {
		mode = MODE_ORBIT;
		transition = 0;
	}
	else {
		SetMode(MODE_TRANSLATE);
	}

	external_group.append(group.container());

	ListIter<Ship> iter = external_group;
	while (++iter) {
		Ship* s = iter.value();
		if (!s) continue;

		region = s->GetRegion();
		Observe(s);
	}
}

// ---------------------------------------------------------------------
// Transition (legacy parity)
// ---------------------------------------------------------------------

void CameraManager::Transition(double seconds)
{
	if (transition > 0)
		transition -= seconds * 1.5;

	if (transition <= 0) {
		transition = 0;

		if (requested_mode && requested_mode != mode)
			mode = requested_mode;

		requested_mode = 0;

		if (mode == MODE_TRANSLATE || mode == MODE_ZOOM) {
			if (mode == MODE_ZOOM)
				range = range_min;

			mode = MODE_ORBIT;
		}
	}
}

// ---------------------------------------------------------------------
// SimObserver name (legacy parity)
// ---------------------------------------------------------------------

const char* CameraManager::GetObserverName() const
{
	return "CameraManager";
}
