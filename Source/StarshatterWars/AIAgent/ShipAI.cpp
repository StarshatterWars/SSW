/*  Project STARSHATTER WARS
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR: John DiCamillo
	ORIGINAL STUDIO: Destroyer Studios

	SUBSYSTEM:    Stars.exe
	FILE:         ShipAI.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Starship (low-level) Artificial Intelligence class
*/

#include "ShipAI.h"

// Unreal (minimal, for FVector and UE_LOG)
#include "Math/Vector.h"
#include "Logging/LogMacros.h"

// Starshatter core / sim:
#include "TacticalAI.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "SimShot.h"
#include "SimElement.h"
#include "NavLight.h"
#include "Instruction.h"
#include "RadioMessage.h"
#include "RadioTraffic.h"
#include "SimContact.h"
#include "WeaponGroup.h"
#include "Drive.h"
#include "Shield.h"
#include "Sim.h"
#include "PlayerCharacter.h"
#include "StarSystem.h"
#include "FlightComputer.h"
#include "Farcaster.h"
#include "QuantumDrive.h"
#include "Debris.h"
#include "Asteroid.h"

#include "Game.h"
#include "Random.h"

// +----------------------------------------------------------------------+

// If your project already defines a central log category, replace this with it.
DEFINE_LOG_CATEGORY_STATIC(LogTempAI, Log, All);

// +--------------------------------------------------------------------+

ShipAI::ShipAI(SimObject* s)
	: SteerAI(s),
	support(0),
	rumor(0),
	threat(0),
	threat_missile(0),
	drop_time(0),
	too_close(0),
	navpt(0),
	patrol(0),
	engaged_ship_id(0),
	bracket(false),
	identify(false),
	hold(false),
	takeoff(false),
	throttle(0),
	old_throttle(0),
	element_index(1),
	splash_count(0),
	tactical(0),
	farcaster(0),
	ai_level(2),
	last_avoid_time(0),
	last_call_time(0)
{
	ship = (Ship*)self;

	Sim* sim = Sim::GetSim();
	Ship* pship = sim->GetPlayerShip();
	int   player_team = 1;

	if (pship)
		player_team = pship->GetIFF();

	PlayerCharacter* player = PlayerCharacter::GetCurrentPlayer();
	if (player) {
		if (ship && ship->GetIFF() && ship->GetIFF() != player_team) {
			ai_level = player->AILevel();
		}
		else if (player->AILevel() == 0) {
			ai_level = 1;
		}
	}

	// evil alien ships are *always* smart:
	if (ship && ship->GetIFF() > 1 && ship->Design()->auto_roll > 1) {
		ai_level = 2;
	}
}

// +--------------------------------------------------------------------+

ShipAI::~ShipAI()
{
	delete tactical;
}

void
ShipAI::ClearTactical()
{
	delete tactical;
	tactical = 0;
}

// +--------------------------------------------------------------------+

Ship*
ShipAI::GetWard() const
{
	return ship->GetWard();
}

void
ShipAI::SetWard(Ship* s)
{
	if (ship == nullptr)
		return;
	if (s == ship->GetWard())
		return;

	if (ship)
		ship->SetWard(s);

	FVector form = RandomDirection();
	form.Z = form.Y;
	form.Y = form.Z; // NOTE: preserved intent of SwapYZ() from legacy Point

	if (FMath::Abs((double)form.X) < 0.5) {
		if (form.X < 0)
			form.X = -0.5f;
		else
			form.X = 0.5f;
	}

	if (ship && ship->IsStarship()) {
		form *= 30e3f;
	}
	else {
		form *= 15e3f;
		form.Y = 500.0f;
	}

	SetFormationDelta(form);
}

void
ShipAI::SetSupport(Ship* s)
{
	if (support == s)
		return;

	support = s;

	if (support)
		Observe(support);
}

void
ShipAI::SetRumor(Ship* s)
{
	if (!s || rumor == s)
		return;

	rumor = s;

	if (rumor)
		Observe(rumor);
}

void
ShipAI::ClearRumor()
{
	rumor = 0;
}

void
ShipAI::SetThreat(Ship* s)
{
	if (threat == s)
		return;

	threat = s;

	if (threat)
		Observe(threat);
}

void
ShipAI::SetThreatMissile(SimShot* s)
{
	if (threat_missile == s)
		return;

	threat_missile = s;

	if (threat_missile)
		Observe(threat_missile);
}

bool
ShipAI::Update(SimObject* obj)
{
	if (obj == support)
		support = 0;

	if (obj == threat)
		threat = 0;

	if (obj == threat_missile)
		threat_missile = 0;

	if (obj == rumor)
		rumor = 0;

	return SteerAI::Update(obj);
}

const char*
ShipAI::GetObserverName() const
{
	static char name[64];
	sprintf_s(name, "ShipAI(%s)", self->GetName());
	return name;
}

// +--------------------------------------------------------------------+

FVector
ShipAI::GetPatrol() const
{
	return patrol_loc;
}

void
ShipAI::SetPatrol(const FVector& p)
{
	patrol = 1;
	patrol_loc = p;
}

void
ShipAI::ClearPatrol()
{
	patrol = 0;
}

// +--------------------------------------------------------------------+

void
ShipAI::ExecFrame(double secs)
{
	seconds = secs;

	if (drop_time > 0) drop_time -= seconds;
	if (!ship) return;

	ship->SetDirectorInfo(" ");

	// check to make sure current navpt is still valid:
	if (navpt)
		navpt = ship->GetNextNavPoint();

	if (ship->GetFlightPhase() == Ship::TAKEOFF || ship->GetFlightPhase() == Ship::LAUNCH)
		takeoff = true;

	if (takeoff) {
		FindObjective();
		Navigator();

		if (ship->GetMissionClock() > 10000)
			takeoff = false;

		return;
	}

	// initial assessment:
	if (ship->GetMissionClock() < 5000)
		return;

	element_index = ship->GetElementIndex();

	NavlightControl();
	CheckTarget();

	if (tactical)
		tactical->ExecFrame(seconds);

	if (target && target != ship->GetTarget()) {
		ship->LockTarget(target);

		// if able to lock target, and target is a ship (not a shot)...
		if (target == ship->GetTarget() && target->GetType() == SimObject::SIM_SHIP) {

			// if this isn't the same ship we last called out:
			if (target->Identity() != engaged_ship_id && Game::GameTime() - last_call_time > 10000) {
				// call engaging:
				RadioMessage* msg = new RadioMessage(ship->GetElement(), ship, RadioMessageAction::CALL_ENGAGING);
				msg->AddTarget(target);
				RadioTraffic::Transmit(msg);
				last_call_time = Game::GameTime();

				engaged_ship_id = target->Identity();
			}
		}
	}

	else if (!target) {
		target = ship->GetTarget();

		if (engaged_ship_id && !target) {
			engaged_ship_id = 0;
		}
	}

	FindObjective();
	Navigator();
}

// +--------------------------------------------------------------------+

FVector
ShipAI::ClosingVelocity()
{
	if (ship && target) {
		if (ship->GetPrimaryDesign()) {
			WeaponDesign* guns = ship->GetPrimaryDesign();
			FVector       delta = (FVector)(target->GetLocation() - ship->GetLocation());

			// fighters need to aim the ship so that the guns will hit the target
			if (guns->firing_cone < 10 * DEGREES && guns->max_range <= delta.Size()) {
				FVector aim_vec = ship->GetHeading();
				aim_vec.Normalize();

				FVector shot_vel = ship->GetVelocity() + aim_vec * (float)guns->speed;
				return shot_vel - target->GetVelocity();
			}

			// ships with turreted weapons just need to worry about actual closing speed
			else {
				return ship->GetVelocity() - target->GetVelocity();
			}
		}
		else {
			return ship->GetVelocity();
		}
	}

	return FVector(1, 0, 0);
}

// +--------------------------------------------------------------------+

void
ShipAI::FindObjective()
{
	distance = 0;

	RadioMessageAction order = ship->GetRadioOrders()->GetRadioAction();

	if (order == RadioMessageAction::QUANTUM_TO ||
		order == RadioMessageAction::FARCAST_TO) {

		FindObjectiveQuantum();
		objective = Transform(obj_w);
		return;
	}

	bool form =
		(order == RadioMessageAction::WEP_HOLD) ||
		(order == RadioMessageAction::FORM_UP) ||
		(order == RadioMessageAction::MOVE_PATROL) ||
		(order == RadioMessageAction::RTB) ||
		(order == RadioMessageAction::DOCK_WITH) ||
		((order == RadioMessageAction::NONE) && !target) ||   
		(farcaster);

	Ship* ward = ship->GetWard();

	// if not the element leader, stay in formation:
	if (form && element_index > 1) {
		ship->SetDirectorInfo(Game::GetText("ai.formation"));

		if (navpt && navpt->GetAction() == INSTRUCTION_ACTION::LAUNCH) {
			FindObjectiveNavPoint();
		}
		else {
			navpt = 0;
			FindObjectiveFormation();
		}

		// transform into camera coords:
		objective = Transform(obj_w);
		return;
	}

	// under orders?
	bool directed = false;
	if (tactical)
		directed = (tactical->RulesOfEngagement() == TacticalAI::DIRECTED);

	// threat processing:
	if (threat && !directed) {
		double d_threat = ((FVector)(threat->GetLocation() - ship->GetLocation())).Size();

		// seek support:
		if (support) {
			double d_support = ((FVector)(support->GetLocation() - ship->GetLocation())).Size();
			if (d_support > 35e3) {
				ship->SetDirectorInfo("Regroup");
				FindObjectiveTarget(support);
				objective = Transform(obj_w);
				return;
			}
		}

		// run away:
		else if (threat != target) {
			ship->SetDirectorInfo("Retreat");
			obj_w = ship->GetLocation() + (FVector)(ship->GetLocation() - threat->GetLocation()) * 100.0f;
			objective = Transform(obj_w);
			return;
		}
	}

	// normal processing:
	if (target) {
		ship->SetDirectorInfo("Seek Target");
		FindObjectiveTarget(target);
		objective = AimTransform(obj_w);
	}

	else if (patrol) {
		ship->SetDirectorInfo("Patrol");
		FindObjectivePatrol();
		objective = Transform(obj_w);
	}

	else if (ward) {
		ship->SetDirectorInfo("Seek Wward");
		FindObjectiveFormation();
		objective = Transform(obj_w);
	}

	else if (navpt && form) {
		ship->SetDirectorInfo("Seek Navpoint");
		FindObjectiveNavPoint();
		objective = Transform(obj_w);
	}

	else if (rumor) {
		ship->SetDirectorInfo("Search");
		FindObjectiveTarget(rumor);
		objective = Transform(obj_w);
	}

	else {
		obj_w = FVector::ZeroVector;
		objective = FVector::ZeroVector;
	}
}

// +--------------------------------------------------------------------+

void
ShipAI::FindObjectiveTarget(SimObject* tgt)
{
	if (!tgt) {
		obj_w = FVector::ZeroVector;
		return;
	}

	navpt = 0; // this tells fire control that we are chasing a target

	FVector cv = ClosingVelocity();
	double  cvl = cv.Size();
	double  time = 0;

	if (cvl > 50) {
		// distance from self to target:
		distance = ((FVector)(tgt->GetLocation() - self->GetLocation())).Size();

		// time to reach target:
		time = distance / cvl;

		// where the target will be when we reach it:
		if (time < 15) {
			FVector run_vec = tgt->GetVelocity();
			obj_w = tgt->GetLocation() + run_vec * (float)time;

			if (time < 10)
				obj_w += tgt->GetAcceleration() * (float)(0.33 * time * time);
		}
		else {
			obj_w = tgt->GetLocation();
		}
	}
	else {
		obj_w = tgt->GetLocation();
	}

	distance = ((FVector)(obj_w - self->GetLocation())).Size();

	if (cvl > 50) {
		time = distance / cvl;

		// where we will be when the target gets there:
		if (time < 15) {
			FVector self_dest = self->GetLocation() + cv * (float)time;
			FVector err = obj_w - self_dest;

			obj_w += err;
		}
	}

	FVector approach = obj_w - self->GetLocation();
	distance = approach.Size();

	if (bracket && distance > 25e3) {
		FVector offset = FVector::CrossProduct(approach, FVector(0, 1, 0));
		offset.Normalize();
		offset *= 15e3f;

		Ship* s = (Ship*)self;
		if (s->GetElementIndex() & 1)
			obj_w -= offset;
		else
			obj_w += offset;
	}
}

// +--------------------------------------------------------------------+

void
ShipAI::FindObjectivePatrol()
{
	navpt = 0;

	FVector npt = patrol_loc;
	obj_w = npt;

	// distance from self to navpt:
	distance = ((FVector)(obj_w - self->GetLocation())).Size();

	if (distance < 1000) {
		ship->ClearRadioOrders();
		ClearPatrol();
	}
}

// +--------------------------------------------------------------------+

void
ShipAI::FindObjectiveNavPoint()
{
	SimRegion* SelfRgn = ship ? ship->GetRegion() : nullptr;
	SimRegion* NavRgn = navpt ? navpt->GetRegion() : nullptr;
	QuantumDrive* QDrive = ship ? ship->GetQuantumDrive() : nullptr;

	if (!SelfRgn || !navpt)
		return;

	if (!NavRgn) {
		NavRgn = SelfRgn;
		navpt->SetRegion(NavRgn);
	}

	const bool bUseFarcaster =
		(SelfRgn != NavRgn) &&
		(navpt->GetFarcast() ||
			!QDrive ||
			!QDrive->IsPowerOn() ||
			QDrive->GetStatus() < SYSTEM_STATUS::DEGRADED);

	if (bUseFarcaster) {
		FindObjectiveFarcaster(SelfRgn, NavRgn);
		return;
	}

	// ------------------------------------------------------------
	// Non-farcaster routing:
	// ------------------------------------------------------------
	if (farcaster) {
		// If our current farcaster isn't in the same region, re-acquire via destination.
		if (farcaster->GetShip() && farcaster->GetShip()->GetRegion() != SelfRgn) {
			if (farcaster->GetDest())
				farcaster = farcaster->GetDest()->GetFarcaster();
		}

		if (farcaster) {
			obj_w = farcaster->EndPoint(); // expected FVector in UE port
		}
	}

	if (!farcaster) {
		// Transform from StarSystem space to current active region space.
		// UE port assumption:
		// - Region::Location() returns FVector (world/region origin)
		// - NavPoint::Location() returns FVector (local within region)
		FVector Npt = navpt->GetRegion()->GetLocation() + navpt->GetLocation();

		SimRegion* ActiveRegion = ship->GetRegion();
		if (ActiveRegion)
			Npt -= ActiveRegion->GetLocation();

		// If your UE port removed handedness conversions, delete this line.
		// Keep it ONLY if you still maintain a legacy "OtherHand()" helper on FVector/Point.
		// Npt = Npt.OtherHand();

		obj_w = Npt;
	}

	// Distance from self to navpt:
	distance = (obj_w - ship->GetLocation()).Size();

	if (farcaster && distance < 1000.0)
		farcaster = nullptr;

	if (distance < 1000.0 ||
		(navpt->GetAction() == INSTRUCTION_ACTION::LAUNCH && distance > 25000.0))
	{
		ship->SetNavptStatus(navpt, INSTRUCTION_STATUS::COMPLETE);
	}
}

// +--------------------------------------------------------------------+

void
ShipAI::FindObjectiveQuantum()
{
	Instruction* Orders = ship ? ship->GetRadioOrders() : nullptr;
	SimRegion* SelfRgn = ship ? ship->GetRegion() : nullptr;
	SimRegion* NavRgn = Orders ? Orders->GetRegion() : nullptr;
	QuantumDrive* QDrive = ship ? ship->GetQuantumDrive() : nullptr;

	if (!Orders || !SelfRgn || !NavRgn)
		return;

	const bool bUseFarcaster =
		(SelfRgn != NavRgn) &&
		(Orders->GetFarcast() ||
			!QDrive ||
			!QDrive->IsPowerOn() ||
			QDrive->GetStatus() < SYSTEM_STATUS::DEGRADED);

	if (bUseFarcaster) {
		FindObjectiveFarcaster(SelfRgn, NavRgn);
		return;
	}

	// ------------------------------------------------------------
	// Non-farcaster routing:
	// ------------------------------------------------------------
	if (farcaster) {
		// If farcaster ship is not in our current region, reacquire through destination.
		if (farcaster->GetShip() && farcaster->GetShip()->GetRegion() != SelfRgn) {
			if (farcaster->GetDest())
				farcaster = farcaster->GetDest()->GetFarcaster();
		}

		if (farcaster) {
			obj_w = farcaster->EndPoint(); // expected FVector in UE port
		}
	}

	if (!farcaster) {
		// Transform from StarSystem space to active region space:
		FVector Npt = Orders->GetRegion()->GetLocation() + Orders->GetLocation();

		SimRegion* ActiveRegion = ship->GetRegion();
		if (ActiveRegion)
			Npt -= ActiveRegion->GetLocation();

		// If you kept a legacy handedness helper, apply it here; otherwise omit.
		// Npt = Npt.OtherHand();

		obj_w = Npt;

		// If the QDrive is ready, set destination and engage immediately:
		if (QDrive && QDrive->ActiveState() == QuantumDrive::ACTIVE_READY) {
			QDrive->SetDestination(NavRgn, Orders->GetLocation());
			QDrive->Engage();
			return;
		}
	}

	// Distance from self to objective:
	distance = (obj_w - ship->GetLocation()).Size();

	if (farcaster) {
		if (distance < 1000.0) {
			farcaster = nullptr;
			ship->ClearRadioOrders();
		}
	}
	else if (SelfRgn == NavRgn) {
		ship->ClearRadioOrders();
	}
}

void
ShipAI::FindObjectiveFarcaster(SimRegion* src_rgn, SimRegion* dst_rgn)
{
	if (!farcaster) {
		ListIter<Ship> s = src_rgn->GetShips();
		while (++s && !farcaster) {
			if (s->GetFarcaster()) {
				const Ship* dest = s->GetFarcaster()->GetDest();
				if (dest && dest->GetRegion() == dst_rgn) {
					farcaster = s->GetFarcaster();
				}
			}
		}
	}

	if (farcaster) {
		FVector apt = farcaster->ApproachPoint(0);
		FVector npt = farcaster->StartPoint();
		double  r1 = ((FVector)(ship->GetLocation() - npt)).Size();

		if (r1 > 50e3) {
			obj_w = apt;
			distance = r1;
		}
		else {
			double r2 = ((FVector)(ship->GetLocation() - apt)).Size();
			double r3 = ((FVector)(npt - apt)).Size();

			if (r1 + r2 < 1.2 * r3) {
				obj_w = npt;
				distance = r1;
			}
			else {
				obj_w = apt;
				distance = r2;
			}
		}

		objective = Transform(obj_w);
	}
}

// +--------------------------------------------------------------------+

void
ShipAI::SetFormationDelta(const FVector& point)
{
	formation_delta = point;
}

void
ShipAI::FindObjectiveFormation()
{
	const double Prediction = 5.0;

	// find the base position:
	SimElement* Element = ship->GetElement();
	Ship* LeadShip = Element ? Element->GetShip(1) : nullptr;
	Ship* WardShip = ship->GetWard();

	if (!LeadShip || LeadShip == ship) {
		LeadShip = WardShip;

		distance = (LeadShip->GetLocation() - ship->GetLocation()).Size();
		if (distance < 30e3 && LeadShip->GetVelocity().Size() < 50) {
			obj_w = ship->GetLocation() + LeadShip->GetHeading() * 1e6f;
			distance = -1;
			return;
		}
	}

	obj_w = LeadShip->GetLocation() + LeadShip->GetVelocity() * (float)Prediction;

	// --- FIX: rotate formation delta using Unreal rotation (yaw about Z) ---
	const float YawRadians = (float)(LeadShip->GetCompassHeading() - PI);
	const float YawDegrees = FMath::RadiansToDegrees(YawRadians);

	const FRotator YawRot(0.0f, YawDegrees, 0.0f);
	const FVector FormationOffsetWorld = YawRot.RotateVector(formation_delta);

	obj_w += FormationOffsetWorld;
	// --- end fix ---

	// try to avoid smacking into the ground...
	if (ship->IsAirborne()) {
		if (ship->GetAltitudeAGL() < 3000 || LeadShip->GetAltitudeAGL() < 3000) {
			obj_w.Y += 500.0f;
		}
	}

	const FVector PredictedSelf = ship->GetLocation() + ship->GetVelocity() * (float)Prediction;
	const FVector DeltaWorld = obj_w - PredictedSelf;

	distance = DeltaWorld.Size();

	// get slot z distance:
	FVector SlotProbe = DeltaWorld + ship->GetLocation();
	slot_dist = Transform(SlotProbe).Z;

	SimDirector* LeadDirector = LeadShip->GetDirector();
	if (LeadDirector && (LeadDirector->GetType() == FIGHTER || LeadDirector->GetType() == STARSHIP)) {
		ShipAI* LeadAI = (ShipAI*)LeadDirector;
		farcaster = LeadAI->GetFarcaster();
	}
	else {
		Instruction* NavPointLocal = Element ? Element->GetNextNavPoint() : nullptr;
		if (!NavPointLocal) {
			farcaster = 0;
			return;
		}

		SimRegion* SelfRegion = ship->GetRegion();
		SimRegion* NavRegion = NavPointLocal->GetRegion();
		QuantumDrive* QDrive = ship->GetQuantumDrive();

		if (SelfRegion && !NavRegion) {
			NavRegion = SelfRegion;
			NavPointLocal->SetRegion(NavRegion);
		}

		const bool bUseFarcaster =
			SelfRegion != NavRegion &&
			(NavPointLocal->GetFarcast() ||
				!QDrive ||
				!QDrive->IsPowerOn() ||
				QDrive->GetStatus() < SYSTEM_STATUS::DEGRADED);

		if (bUseFarcaster) {
			ListIter<Ship> ShipIter = SelfRegion->GetShips();
			while (++ShipIter && !farcaster) {
				if (ShipIter->GetFarcaster()) {
					const Ship* Dest = ShipIter->GetFarcaster()->GetDest();
					if (Dest && Dest->GetRegion() == NavRegion) {
						farcaster = ShipIter->GetFarcaster();
					}
				}
			}
		}
		else if (farcaster) {
			if (farcaster->GetShip()->GetRegion() != SelfRegion)
				farcaster = farcaster->GetDest()->GetFarcaster();

			obj_w = farcaster->EndPoint();
			distance = (obj_w - ship->GetLocation()).Size();

			if (distance < 1000)
				farcaster = 0;
		}
	}
}

// +--------------------------------------------------------------------+

void ShipAI::Splash(const Ship* targ)
{
	if (splash_count > 6)
		splash_count = 4;

	RadioTraffic::SendQuickMessage(
		ship,
		static_cast<RadioMessageAction>(
			static_cast<int32>(RadioMessageAction::SPLASH_1) + splash_count
			)
	);

	splash_count++;
}

// +--------------------------------------------------------------------+

void
ShipAI::SetTarget(SimObject* targ, SimSystem* sub)
{
	if (targ != target) {
		bracket = false;
	}

	SteerAI::SetTarget(targ, sub);
}

void
ShipAI::DropTarget(double dtime)
{
	SetTarget(0);
	drop_time = dtime;    // seconds until we can re-acquire

	ship->DropTarget();
}

void
ShipAI::SetBracket(bool b)
{
	bracket = b;
	identify = false;
}

void
ShipAI::SetIdentify(bool i)
{
	identify = i;
	bracket = false;
}

// +--------------------------------------------------------------------+

void
ShipAI::Navigator()
{
	accumulator.Clear();
	magnitude = 0;

	hold = false;
	if ((ship->GetElement() && ship->GetElement()->GetHoldTime() > 0) ||
		(navpt && navpt->GetStatus() == INSTRUCTION_STATUS::COMPLETE && navpt->GetHoldTime() > 0))
		hold = true;

	ship->SetFLCSMode(Ship::FLCS_HELM);

	if (target)
		ship->SetDirectorInfo("Seek Target");
	else if (rumor)
		ship->SetDirectorInfo("Seek Rumor");
	else
		ship->SetDirectorInfo("Cruise");

	Accumulate(AvoidCollision());
	Accumulate(AvoidTerrain());

	if (!hold)
		Accumulate(SeekTarget());

	HelmControl();
	ThrottleControl();
	FireControl();
	AdjustDefenses();
}

// +--------------------------------------------------------------------+

void
ShipAI::HelmControl()
{
	double trans_x = 0;
	double trans_y = 0;
	double trans_z = 0;

	ship->SetHelmHeading(accumulator.yaw);

	if (FMath::Abs(accumulator.pitch) < 5 * DEGREES || FMath::Abs(accumulator.pitch) > 45 * DEGREES) {
		trans_z = objective.Y;
		ship->SetHelmPitch(0);
	}
	else {
		ship->SetHelmPitch(accumulator.pitch);
	}

	ship->SetTransX(trans_x);
	ship->SetTransY(trans_y);
	ship->SetTransZ(trans_z);

	ship->ExecFLCSFrame();
}

/*****************************************
**
**  NOTE:
**  No one is really using this method.
**  It is overridden by both StarshipAI
**  and FighterAI.
**
*****************************************/

void
ShipAI::ThrottleControl()
{
	if (navpt && !threat && !target) {     // lead only, get speed from navpt
		double speed = navpt->GetSpeed();

		if (speed > 0)
			throttle = speed / ship->VelocityLimit() * 100;
		else
			throttle = 50;
	}

	else if (patrol && !threat && !target) { // lead only, get speed from navpt
		double speed = 200;

		if (distance > 5000)
			speed = 500;

		if (ship->GetVelocity().Size() > speed)
			throttle = 0;
		else
			throttle = 50;
	}

	else {
		if (threat || target || element_index < 2) { // element lead
			throttle = 100;

			if (!threat && !target)
				throttle = 50;

			if (accumulator.brake > 0) {
				throttle *= (1 - accumulator.brake);
			}
		}

		else {                                       // wingman
			Ship* lead = ship->GetElement()->GetShip(1);
			double lv = lead->GetVelocity().Size();
			double sv = ship->GetVelocity().Size();
			double dv = lv - sv;
			double dt = 0;

			if (dv > 0)       dt = dv * 1e-2 * seconds;
			else if (dv < 0)  dt = dv * 1e-2 * seconds;

			throttle = old_throttle + dt;
		}
	}

	old_throttle = throttle;
	ship->SetThrottle((int)throttle);
}

// +--------------------------------------------------------------------+

void
ShipAI::NavlightControl()
{
	Ship* leader = ship->GetLeader();

	if (leader && leader != ship) {
		bool navlight_enabled = false;

		if (leader->GetNavLights().size() > 0)
			navlight_enabled = leader->GetNavLights().at(0)->IsEnabled();

		for (int i = 0; i < ship->GetNavLights().size(); i++) {
			if (navlight_enabled)
				ship->GetNavLights().at(i)->Enable();
			else
				ship->GetNavLights().at(i)->Disable();
		}
	}
}

// +--------------------------------------------------------------------+

Steer
ShipAI::AvoidTerrain()
{
	Steer avoid;
	return avoid;
}

// +--------------------------------------------------------------------+

Steer
ShipAI::AvoidCollision()
{
	Steer avoid;

	if (!ship || !ship->GetRegion() || !ship->GetRegion()->IsActive())
		return avoid;

	if (other && (other->GetLife() == 0 || other->GetIntegrity() < 1)) {
		other = 0;
		last_avoid_time = 0; // check for a new obstacle immediately
	}

	if (!other && Game::GameTime() - last_avoid_time < 500)
		return avoid;

	brake = 0;

	// don't get closer than this:
	double avoid_dist = 5 * ship->GetRadius();

	if (avoid_dist < 1e3) avoid_dist = 1e3;
	else if (avoid_dist > 12e3) avoid_dist = 12e3;

	// find the soonest potential collision,
	// ignore any that occur after this:
	double avoid_time = 15;

	if (ship->Design()->avoid_time > 0)
		avoid_time = ship->Design()->avoid_time;
	else if (ship->IsStarship())
		avoid_time *= 1.5;

	FVector bearing = ship->GetVelocity();
	bearing.Normalize();

	bool              found = false;
	ListIter<SimContact> contact = ship->ContactList();

	// check current obstacle first:
	if (other) {
		found = AvoidTestSingleObject(other, bearing, avoid_dist, avoid_time, avoid);
	}

	if (!found) {
		// avoid ships:
		while (++contact && !found) {
			Ship* c_ship = contact->GetShip();

			if (c_ship && c_ship != ship && c_ship->IsStarship()) {
				found = AvoidTestSingleObject(c_ship, bearing, avoid_dist, avoid_time, avoid);
			}
		}

		// also avoid large pieces of debris:
		if (!found) {
			ListIter<Debris> iter = ship->GetRegion()->GetRocks();
			while (++iter && !found) {
				Debris* debris = iter.value();

				if (debris->GetMass() > ship->GetMass())
					found = AvoidTestSingleObject(debris, bearing, avoid_dist, avoid_time, avoid);
			}
		}

		// and asteroids:
		if (!found) {
			// give asteroids a wider berth -
			avoid_dist *= 8;

			ListIter<Asteroid> iter = ship->GetRegion()->GetRoids();
			while (++iter && !found) {
				Asteroid* roid = iter.value();
				found = AvoidTestSingleObject(roid, bearing, avoid_dist, avoid_time, avoid);
			}

			if (!found)
				avoid_dist /= 8;
		}

		// if found, steer to avoid:
		if (other) {
			avoid = Avoid(obstacle, (float)(ship->GetRadius() + other->GetRadius() + avoid_dist * 0.9));
			avoid.brake = brake;

			ship->SetDirectorInfo(Game::GetText("ai.avoid-collision"));
		}
	}

	last_avoid_time = Game::GameTime();
	return avoid;
}

bool
ShipAI::AvoidTestSingleObject(SimObject* obj,
	const FVector& bearing,
	double avoid_dist,
	double& avoid_time,
	Steer& avoid)
{
	if (too_close == obj->Identity()) {
		double dist = ((FVector)(ship->GetLocation() - obj->GetLocation())).Size();
		double closure = FVector::DotProduct((FVector)(ship->GetVelocity() - obj->GetVelocity()), bearing);

		if (closure > 1 && dist < avoid_dist) {
			avoid = AvoidCloseObject(obj);
			return true;
		}
		else {
			too_close = 0;
		}
	}

	// will we get close?
	double time = ClosestApproachTime(ship->GetLocation(), ship->GetVelocity(),
		obj->GetLocation(), obj->GetVelocity());

	// already past the obstacle:
	if (time <= 0) {
		if (other == obj) other = 0;
		return false;
	}

	// how quickly could we collide?
	FVector current_relation = ship->GetLocation() - obj->GetLocation();
	double  current_distance = current_relation.Size() - ship->GetRadius() - obj->GetRadius();

	// are we really far away?
	if (current_distance > 25e3) {
		if (other == obj) other = 0;
		return false;
	}

	// is the obstacle a farcaster?
	if (obj->GetType() == SimObject::SIM_SHIP) {
		Ship* c_ship = (Ship*)obj;

		if (c_ship->GetFarcaster()) {
			// are we on a safe vector?
			FVector dir = ship->GetVelocity();
			dir.Normalize();

			double angle_off = FMath::Abs(FMath::Acos((double)FVector::DotProduct(dir, (FVector)obj->GetCam().vpn())));

			if (angle_off > 90 * DEGREES)
				angle_off = 180 * DEGREES - angle_off;

			if (angle_off < 35 * DEGREES) {
				// will we pass through the center?
				FVector d = ship->GetLocation() + dir * (float)(current_distance + ship->GetRadius() + obj->GetRadius());
				double  err = ((FVector)(obj->GetLocation() - d)).Size();

				if (err < 0.667 * obj->GetRadius()) {
					return false;
				}
			}
		}
	}

	// rate of closure:
	double closing_velocity = FVector::DotProduct((FVector)(ship->GetVelocity() - obj->GetVelocity()), bearing);

	// are we too close already?
	if (current_distance < (avoid_dist * 0.35)) {
		if (closing_velocity > 1 || current_distance < ship->GetRadius()) {
			avoid = AvoidCloseObject(obj);
			return true;
		}
	}

	// too far away to worry about:
	double separation = (avoid_dist + obj->GetRadius());
	if ((current_distance - separation) / closing_velocity > avoid_time) {
		if (other == obj) other = 0;
		return false;
	}

	// where will we be?
	FVector selfpt = ship->GetLocation() + ship->GetVelocity() * (float)time;
	FVector testpt = obj->GetLocation() + obj->GetVelocity() * (float)time;

	// how close will we get?
	double dist = ((FVector)(selfpt - testpt)).Size()
		- ship->GetRadius()
		- obj->GetRadius();

	// that's too close:
	if (dist < avoid_dist) {
		if (dist < avoid_dist * 0.25 && time < avoid_time * 0.5) {
			avoid = AvoidCloseObject(obj);
			return true;
		}

		obstacle = Transform(testpt);

		if (obstacle.Z > 0) {
			other = obj;
			avoid_time = time;
			brake = 0.5;

			Observe(other);
		}
	}

	// hysteresis:
	else if (other == obj && dist > avoid_dist * 1.25) {
		other = 0;
	}

	return false;
}

// +--------------------------------------------------------------------+

Steer
ShipAI::AvoidCloseObject(SimObject* obj)
{
	too_close = obj->Identity();
	obstacle = Transform(obj->GetLocation());
	other = obj;

	Observe(other);

	Steer avoid = Flee(obstacle);
	avoid.brake = 0.3;

	ship->SetDirectorInfo("Avoid collision");
	return avoid;
}

// +--------------------------------------------------------------------+

Steer
ShipAI::SeekTarget()
{
	Ship* ward = ship->GetWard();

	if (!target && !ward && !navpt && !patrol) {
		if (element_index > 1) {
			// wingmen keep in formation:
			return Seek(objective);
		}

		if (farcaster) {
			return Seek(objective);
		}

		if (rumor) {
			return Seek(objective);
		}

		return Steer();
	}

	if (patrol) {
		Steer result = Seek(objective);

		if (distance < 2000) {
			result.brake = 1;
		}

		return result;
	}

	if (target && too_close == target->Identity()) {
		drop_time = 4;
		return Avoid(objective, 0.0f);
	}
	else if (drop_time > 0) {
		return Steer();
	}

	return Seek(objective);
}

// +--------------------------------------------------------------------+

Steer
ShipAI::EvadeThreat()
{
	return Steer();
}

// +--------------------------------------------------------------------+

void
ShipAI::FireControl()
{
}

// +--------------------------------------------------------------------+

void
ShipAI::AdjustDefenses()
{
	Shield* shield = ship->GetShield();

	if (shield) {
		double desire = 50;

		if (threat_missile || threat)
			desire = 100;

		shield->SetPowerLevel(desire);
	}
}

// +--------------------------------------------------------------------+

void
ShipAI::CheckTarget()
{
	if (target) {
		if (target->GetLife() == 0)
			target = 0;

		else if (target->GetType() == SimObject::SIM_SHIP) {
			Ship* tgt_ship = (Ship*)target;

			if (tgt_ship->GetIFF() == ship->GetIFF() && !tgt_ship->IsRogue())
				target = 0;
		}
	}
}
