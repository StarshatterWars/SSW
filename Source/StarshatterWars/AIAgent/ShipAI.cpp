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
#include "ShipActor.h"

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
	{
		return;
	}

	if (s == ship->GetWard())
	{
		return;
	}

	ship->SetWard(s);

	FVector form = RandomDirection();

	const float OldY = form.Y;
	form.Y = form.Z;
	form.Z = OldY;

	if (FMath::Abs((double)form.X) < 0.5)
	{
		if (form.X < 0)
		{
			form.X = -0.5f;
		}
		else
		{
			form.X = 0.5f;
		}
	}

	if (ship->IsStarship())
	{
		form *= 30e3f;
	}
	else
	{
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
	UE_LOG(LogTemp, Warning,
		TEXT("[ShipAI::ExecFrame] Ship='%hs' ShipPtr=%p Seconds=%.4f FlightPhase=%d Life=%.2f Integrity=%.2f Region='%hs' RegionPtr=%p Element=%p ElementIndex=%d Ward='%hs' Target='%hs' Throttle=%.2f Vel=%s Heading=%s Compass=%.4f Helm=%.4f Loc=%s"),
		ship ? ship->GetName() : "NULL",
		ship,
		secs,
		ship ? (int)ship->GetFlightPhase() : -1,
		ship ? (double)ship->GetLife() : -1.0,
		ship ? (double)ship->GetIntegrity() : -1.0,
		ship && ship->GetRegion() ? ship->GetRegion()->GetName() : "NULL",
		ship ? ship->GetRegion() : nullptr,
		ship ? ship->GetElement() : nullptr,
		ship ? ship->GetElementIndex() : -1,
		ship && ship->GetWard() ? ship->GetWard()->GetName() : "NULL",
		target ? target->GetName() : "NULL",
		ship ? ship->GetThrottle() : -1.0,
		ship ? *ship->GetVelocity().ToString() : TEXT("NULL"),
		ship ? *ship->GetHeading().ToString() : TEXT("NULL"),
		ship ? ship->GetCompassHeading() : 0.0,
		ship ? ship->GetHelmHeading() : 0.0,
		ship ? *ship->GetLocation().ToString() : TEXT("NULL"));

	if (drop_time > 0)
		drop_time -= seconds;

	if (!ship)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[ShipAI::ExecFrame] NULL ship"));
		return;
	}

	ship->SetDirectorInfo(" ");

	// check to make sure current navpt is still valid:
	if (navpt)
	{
		navpt = ship->GetNextNavPoint();
	}

	if (ship->GetFlightPhase() == Ship::TAKEOFF ||
		ship->GetFlightPhase() == Ship::LAUNCH)
	{
		takeoff = true;
	}

	if (takeoff)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ShipAI::ExecFrame] TAKEOFF Ship='%s'"),
			ANSI_TO_TCHAR(ship->GetName()));

		FindObjective();
		Navigator();

		if (ship->GetMissionClockMS() > 10000)
		{
			takeoff = false;

			UE_LOG(LogTemp, Warning,
				TEXT("[ShipAI::ExecFrame] TAKEOFF COMPLETE Ship='%s'"),
				ANSI_TO_TCHAR(ship->GetName()));
		}

		return;
	}

	const int32 ClockMS = ship->GetMissionClockMS();
	const double ClockSec = ship->GetMissionClock();

	// initial assessment:
	if (ClockMS < 500)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ShipAI::ExecFrame] WAITING Ship='%s' ClockMS=%d"),
			ANSI_TO_TCHAR(ship->GetName()),
			ClockMS);

		return;
	}

	element_index = ship->GetElementIndex();

	NavlightControl();

	CheckTarget();

	if (tactical)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ShipAI::ExecFrame] CALL Tactical Ship='%s' Tactical=%p Contacts=%d"),
			ANSI_TO_TCHAR(ship->GetName()),
			tactical,
			ship->GetContactList().size());

		tactical->ExecFrame(seconds);
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[ShipAI::ExecFrame] NULL Tactical Ship='%s'"),
			ANSI_TO_TCHAR(ship->GetName()));
	}

	if (target && target != ship->GetTarget())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ShipAI::ExecFrame] LOCK TARGET Ship='%s' Target=%p"),
			ANSI_TO_TCHAR(ship->GetName()),
			target);

		ship->LockTarget(target);

		// if able to lock target, and target is a ship (not a shot)...
		if (target == ship->GetTarget() &&
			target->GetType() == SimObject::SIM_SHIP)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[ShipAI::ExecFrame] TARGET LOCKED Ship='%s'"),
				ANSI_TO_TCHAR(ship->GetName()));

			// if this isn't the same ship we last called out:
			if (target->GetIdentity() != engaged_ship_id &&
				Game::GetGameTime() - last_call_time > 10000)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[ShipAI::ExecFrame] RADIO ENGAGING Ship='%s'"),
					ANSI_TO_TCHAR(ship->GetName()));

				RadioMessage* msg =
					new RadioMessage(
						ship->GetElement(),
						ship,
						RadioMessageAction::CALL_ENGAGING);

				msg->AddTarget(target);

				RadioTraffic::Transmit(msg);

				last_call_time = Game::GetGameTime();

				engaged_ship_id = target->GetIdentity();
			}
		}
	}

	else if (!target)
	{
		target = ship->GetTarget();

		UE_LOG(LogTemp, Warning,
			TEXT("[ShipAI::ExecFrame] PULL TARGET FROM SHIP Ship='%s' ShipTarget=%p"),
			ANSI_TO_TCHAR(ship->GetName()),
			target);

		if (engaged_ship_id && !target)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[ShipAI::ExecFrame] CLEAR ENGAGED TARGET Ship='%s'"),
				ANSI_TO_TCHAR(ship->GetName()));

			engaged_ship_id = 0;
		}
	}

	FindObjective();

	UE_LOG(LogTemp, Warning,
		TEXT("[ShipAI::ExecFrame] FindObjective Ship='%s'"),
		ANSI_TO_TCHAR(ship->GetName()));

	Navigator();

	UE_LOG(LogTemp, Warning,
		TEXT("[ShipAI::ExecFrame] END Ship='%s' Target=%p Contacts=%d"),
		ANSI_TO_TCHAR(ship->GetName()),
		target,
		ship->GetContactList().size());
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
	distance = 0.0;
	obj_w = FVector::ZeroVector;
	objective = FVector::ZeroVector;

	if (!ship)
	{
		return;
	}

	const FVector ShipLoc = ship->GetLocation();

	RadioMessageAction order =
		ship->GetRadioOrders() ?
		ship->GetRadioOrders()->GetRadioAction() :
		RadioMessageAction::NONE;

	const bool form =
		(order == RadioMessageAction::WEP_HOLD) ||
		(order == RadioMessageAction::FORM_UP) ||
		(order == RadioMessageAction::MOVE_PATROL) ||
		(order == RadioMessageAction::RTB) ||
		(order == RadioMessageAction::DOCK_WITH) ||
		((order == RadioMessageAction::NONE) && !target) ||
		(farcaster != nullptr);

	Ship* ward = ship->GetWard();

	if (order == RadioMessageAction::QUANTUM_TO ||
		order == RadioMessageAction::FARCAST_TO)
	{
		FindObjectiveQuantum();
	}
	else if (form && (element_index > 1 || ward))
	{
		ship->SetDirectorInfo(Game::GetText("ai.formation"));

		if (navpt && navpt->GetAction() == INSTRUCTION_ACTION::LAUNCH)
		{
			FindObjectiveNavPoint();
		}
		else
		{
			navpt = nullptr;
			FindObjectiveFormation();
		}
	}
	else
	{
		bool directed = false;

		if (tactical)
		{
			directed =
				(tactical->RulesOfEngagement() == TacticalAI::DIRECTED);
		}

		bool bObjectiveHandled = false;

		if (threat && !directed)
		{
			if (support)
			{
				const double d_support =
					(support->GetLocation() - ShipLoc).Size();

				if (d_support > 35e3)
				{
					ship->SetDirectorInfo("Regroup");
					FindObjectiveTarget(support);
					bObjectiveHandled = true;
				}
			}
			else if (threat != target)
			{
				ship->SetDirectorInfo("Retreat");

				FVector AwayFromThreat =
					ShipLoc - threat->GetLocation();

				if (!AwayFromThreat.IsNearlyZero())
				{
					AwayFromThreat.Normalize();

					obj_w =
						ShipLoc +
						AwayFromThreat * 100000.0f;

					bObjectiveHandled = true;
				}
			}
		}

		if (!bObjectiveHandled)
		{
			if (target)
			{
				ship->SetDirectorInfo("Seek Target");
				FindObjectiveTarget(target);
			}
			else if (patrol)
			{
				ship->SetDirectorInfo("Patrol");
				FindObjectivePatrol();
			}
			else if (ward)
			{
				ship->SetDirectorInfo("Seek Ward");
				FindObjectiveFormation();
			}
			else if (navpt && form)
			{
				ship->SetDirectorInfo("Seek Navpoint");
				FindObjectiveNavPoint();
			}
			else if (rumor)
			{
				ship->SetDirectorInfo("Search");
				FindObjectiveTarget(rumor);
			}
			else
			{
				obj_w = FVector::ZeroVector;
				objective = FVector::ZeroVector;
				distance = 0.0;
			}
		}
	}

	objective = obj_w;

	const bool bInvalidObjW =
		obj_w.ContainsNaN() ||
		!FMath::IsFinite(obj_w.X) ||
		!FMath::IsFinite(obj_w.Y) ||
		!FMath::IsFinite(obj_w.Z);

	const bool bInvalidObjective =
		objective.ContainsNaN() ||
		!FMath::IsFinite(objective.X) ||
		!FMath::IsFinite(objective.Y) ||
		!FMath::IsFinite(objective.Z);

	if (bInvalidObjW || bInvalidObjective)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[ShipAI::FindObjective] INVALID OBJECTIVE Ship='%hs' ObjW=%s Objective=%s Resetting."),
			ship ? ship->GetName() : "NULL",
			*obj_w.ToString(),
			*objective.ToString());

		obj_w = FVector::ZeroVector;
		objective = FVector::ZeroVector;
		distance = 0.0;
		return;
	}

	if (!objective.IsNearlyZero())
	{
		distance = (objective - ShipLoc).Size();
	}
	else
	{
		distance = 0.0;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[ShipAI::FindObjective WORLD] Ship='%hs' Target='%hs' Threat='%hs' Support='%hs' Ward='%hs' Navpt=%d Patrol=%d Rumor='%hs' ObjW=%s ObjectiveWorld=%s ShipLoc=%s ObjectiveDelta=%s Distance=%.2f ElementIndex=%d Form=%d"),
		ship ? ship->GetName() : "NULL",
		target ? target->GetName() : "NULL",
		threat ? threat->GetName() : "NULL",
		support ? support->GetName() : "NULL",
		ward ? ward->GetName() : "NULL",
		navpt ? 1 : 0,
		patrol ? 1 : 0,
		rumor ? rumor->GetName() : "NULL",
		*obj_w.ToString(),
		*objective.ToString(),
		*ShipLoc.ToString(),
		*(objective - ShipLoc).ToString(),
		distance,
		element_index,
		form ? 1 : 0);
}

// +--------------------------------------------------------------------+

void
ShipAI::FindObjectiveTarget(SimObject* tgt)
{
	if (!self || !ship)
	{
		obj_w = FVector::ZeroVector;
		distance = 0;
		return;
	}

	if (!tgt)
	{
		obj_w = FVector::ZeroVector;
		distance = 0;
		return;
	}

	// This tells fire control that we are chasing a target.
	navpt = nullptr;

	const FVector SelfLoc = self->GetLocation();
	const FVector TargetLoc = tgt->GetLocation();

	const FVector ClosingVel = ClosingVelocity();
	const double ClosingSpeed = ClosingVel.Size();

	double time = 0;

	if (ClosingSpeed > 50)
	{
		distance = (TargetLoc - SelfLoc).Size();
		time = distance / ClosingSpeed;

		if (time < 15)
		{
			const FVector TargetVel = tgt->GetVelocity();

			obj_w = TargetLoc + TargetVel * static_cast<float>(time);

			if (time < 10)
			{
				obj_w += tgt->GetAcceleration() *
					static_cast<float>(0.33 * time * time);
			}
		}
		else
		{
			obj_w = TargetLoc;
		}
	}
	else
	{
		obj_w = TargetLoc;
	}

	distance = (obj_w - SelfLoc).Size();

	if (ClosingSpeed > 50)
	{
		time = distance / ClosingSpeed;

		if (time < 15)
		{
			const FVector SelfDest =
				SelfLoc + ClosingVel * static_cast<float>(time);

			const FVector Error =
				obj_w - SelfDest;

			obj_w += Error;
		}
	}

	const FVector Approach =
		obj_w - SelfLoc;

	distance = Approach.Size();

	if (bracket && distance > 25e3)
	{
		FVector Offset =
			FVector::CrossProduct(Approach, FVector(0, 1, 0));

		if (!Offset.IsNearlyZero())
		{
			Offset.Normalize();
			Offset *= 15e3f;

			Ship* s = static_cast<Ship*>(self);

			if (s && (s->GetElementIndex() & 1))
			{
				obj_w -= Offset;
			}
			else
			{
				obj_w += Offset;
			}
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[ShipAI::FindObjectiveTarget LEGACY] Ship='%hs' Target='%hs' SelfLoc=%s TargetLoc=%s TargetVel=%s ClosingVel=%s ClosingSpeed=%.2f ObjW=%s Distance=%.2f Bracket=%d"),
		ship ? ship->GetName() : "NULL",
		tgt ? tgt->GetName() : "NULL",
		*SelfLoc.ToString(),
		*TargetLoc.ToString(),
		*tgt->GetVelocity().ToString(),
		*ClosingVel.ToString(),
		ClosingSpeed,
		*obj_w.ToString(),
		distance,
		bracket ? 1 : 0);
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
	if (!ship || !src_rgn || !dst_rgn)
	{
		obj_w = FVector::ZeroVector;
		objective = FVector::ZeroVector;
		distance = 0;
		return;
	}

	if (!farcaster)
	{
		ListIter<Ship> s = src_rgn->GetShips();

		while (++s && !farcaster)
		{
			if (s->GetFarcaster())
			{
				const Ship* dest = s->GetFarcaster()->GetDest();

				if (dest && dest->GetRegion() == dst_rgn)
				{
					farcaster = s->GetFarcaster();
				}
			}
		}
	}

	if (farcaster)
	{
		const FVector apt = farcaster->ApproachPoint(0);
		const FVector npt = farcaster->StartPoint();

		const double r1 =
			(ship->GetLocation() - npt).Size();

		if (r1 > 50e3)
		{
			obj_w = apt;
			distance = r1;
		}
		else
		{
			const double r2 =
				(ship->GetLocation() - apt).Size();

			const double r3 =
				(npt - apt).Size();

			if (r1 + r2 < 1.2 * r3)
			{
				obj_w = npt;
				distance = r1;
			}
			else
			{
				obj_w = apt;
				distance = r2;
			}
		}

		// LEGACY SIM SPACE ONLY.
		objective = obj_w;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[ShipAI::FindObjectiveFarcaster LEGACY] Ship='%hs' Farcaster=%p ObjW=%s Objective=%s ShipLoc=%s Distance=%.2f"),
		ship ? ship->GetName() : "NULL",
		farcaster,
		*obj_w.ToString(),
		*objective.ToString(),
		ship ? *ship->GetLocation().ToString() : TEXT("NULL"),
		distance);
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

	if (!ship)
	{
		obj_w = FVector::ZeroVector;
		objective = FVector::ZeroVector;
		distance = 0.0;
		return;
	}

	SimElement* Element = ship->GetElement();
	Ship* WardShip = ship->GetWard();
	Ship* LeadShip = nullptr;

	if (Element)
	{
		LeadShip = Element->GetShip(1);

		if (LeadShip == ship && Element->NumShips() > 1)
		{
			for (int i = 1; i <= Element->NumShips(); i++)
			{
				Ship* Candidate = Element->GetShip(i);

				if (Candidate && Candidate != ship)
				{
					LeadShip = Candidate;
					break;
				}
			}
		}
	}

	if ((!LeadShip || LeadShip == ship) && WardShip && WardShip != ship)
	{
		LeadShip = WardShip;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[FORMATION DEBUG] Ship='%hs' Element=%p ElementIndex=%d ElementShips=%d Ward='%hs' Lead='%hs' FormationDelta=%s ShipLoc=%s"),
		ship ? ship->GetName() : "NULL",
		Element,
		element_index,
		Element ? Element->NumShips() : -1,
		WardShip ? WardShip->GetName() : "NULL",
		LeadShip ? LeadShip->GetName() : "NULL",
		*formation_delta.ToString(),
		*ship->GetLocation().ToString());

	if (!LeadShip || LeadShip == ship)
	{
		obj_w =
			ship->GetLocation() +
			ship->GetHeading() * 100000.0f;

		objective = obj_w;
		distance = (objective - ship->GetLocation()).Size();

		UE_LOG(LogTemp, Error,
			TEXT("[ShipAI::FindObjectiveFormation] NO VALID LEAD Ship='%hs' FallbackObjW=%s Distance=%.2f"),
			ship ? ship->GetName() : "NULL",
			*obj_w.ToString(),
			distance);

		return;
	}

	obj_w =
		LeadShip->GetLocation() +
		LeadShip->GetVelocity() *
		static_cast<float>(Prediction);

	const FVector LeadForward = LeadShip->GetCam().vpn();
	const FVector LeadRight = LeadShip->GetCam().vrt();
	const FVector LeadUp = LeadShip->GetCam().vup();

	const FVector FormationOffsetWorld =
		LeadRight * formation_delta.X +
		LeadUp * formation_delta.Y +
		LeadForward * formation_delta.Z;

	obj_w += FormationOffsetWorld;

	const FVector PredictedSelf =
		ship->GetLocation() +
		ship->GetVelocity() *
		static_cast<float>(Prediction);

	const FVector DeltaWorld =
		obj_w - PredictedSelf;

	distance = DeltaWorld.Size();

	FVector LocalSlot =
		WorldPointToLegacyLocalObjective(obj_w);

	slot_dist = LocalSlot.Z;

	objective = obj_w;

	UE_LOG(LogTemp, Warning,
		TEXT("[ShipAI::FindObjectiveFormation WORLD] Ship='%hs' Lead='%hs' Ward='%hs' LeadLoc=%s ShipLoc=%s ObjW=%s Objective=%s DeltaWorld=%s Distance=%.2f SlotDist=%.2f LocalSlot=%s LeadBasis F=%s R=%s U=%s"),
		ship ? ship->GetName() : "NULL",
		LeadShip ? LeadShip->GetName() : "NULL",
		WardShip ? WardShip->GetName() : "NULL",
		*LeadShip->GetLocation().ToString(),
		*ship->GetLocation().ToString(),
		*obj_w.ToString(),
		*objective.ToString(),
		*DeltaWorld.ToString(),
		distance,
		slot_dist,
		*LocalSlot.ToString(),
		*LeadForward.ToString(),
		*LeadRight.ToString(),
		*LeadUp.ToString());
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

	UE_LOG(LogTemp, Warning,
		TEXT("[ShipAI::Navigator] ENTER Ship='%hs' Target='%hs' Rumor='%hs' Navpt=%p Hold=%d Distance=%.2f Objective=%s"),
		ship ? ship->GetName() : "NULL",
		target ? target->GetName() : "NULL",
		rumor ? rumor->GetName() : "NULL",
		navpt,
		hold ? 1 : 0,
		distance,
		*objective.ToString());

	hold = false;

	if ((ship->GetElement() && ship->GetElement()->GetHoldTime() > 0) ||
		(navpt && navpt->GetStatus() == INSTRUCTION_STATUS::COMPLETE && navpt->GetHoldTime() > 0))
	{
		hold = true;
	}

	ship->SetFLCSMode(EFLCSMode::HELM);

	if (target)
	{
		ship->SetDirectorInfo("Seek Target");
	}
	else if (rumor)
	{
		ship->SetDirectorInfo("Seek Rumor");
	}
	else
	{
		ship->SetDirectorInfo("Cruise");
	}

	Accumulate(AvoidCollision());
	Accumulate(AvoidTerrain());

	if (!hold)
	{
		Steer Seek =
			SeekTarget();

		UE_LOG(LogTemp, Warning,
			TEXT("[ShipAI::Navigator] SEEK Ship='%hs' Target='%hs' Yaw=%.4f Pitch=%.4f Brake=%.2f Stop=%d"),
			ship ? ship->GetName() : "NULL",
			target ? target->GetName() : "NULL",
			Seek.yaw,
			Seek.pitch,
			Seek.brake,
			Seek.stop ? 1 : 0);

		Accumulate(Seek);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ShipAI::Navigator] HOLD Ship='%hs'"),
			ship ? ship->GetName() : "NULL");
	}

	HelmControl();
	ThrottleControl();
	FireControl();
	AdjustDefenses();

	UE_LOG(LogTemp, Warning,
		TEXT("[ShipAI::Navigator] EXIT Ship='%hs' Target='%hs' HelmHeading=%.4f HelmPitch=%.4f Throttle=%.2f Request=%.2f FLCSMode=%d"),
		ship ? ship->GetName() : "NULL",
		target ? target->GetName() : "NULL",
		ship ? ship->GetHelmHeading() : 0.0,
		ship ? ship->GetHelmPitch() : 0.0,
		ship ? ship->GetThrottle() : 0.0,
		ship ? ship->GetThrottleRequest() : 0.0,
		ship ? static_cast<int32>(ship->GetFLCSMode()) : -1);
}

// +--------------------------------------------------------------------+

void
ShipAI::HelmControl()
{
	if (!ship)
	{
		return;
	}

	double trans_x = 0.0;
	double trans_y = 0.0;
	double trans_z = 0.0;

	/*
	 * Base ShipAI contract:
	 *
	 * objective is WORLD SPACE.
	 * accumulator already contains the steering command produced by Seek().
	 *
	 * Do not read objective.X/Y/Z directly as a local-space vector here.
	 */

	ship->SetHelmHeading(accumulator.yaw);

	if (FMath::Abs(accumulator.pitch) < 5.0 * DEGREES ||
		FMath::Abs(accumulator.pitch) > 45.0 * DEGREES)
	{
		ship->SetHelmPitch(0.0);
	}
	else
	{
		ship->SetHelmPitch(accumulator.pitch);
	}

	ship->SetTransX(trans_x);
	ship->SetTransY(trans_y);
	ship->SetTransZ(trans_z);

	UE_LOG(LogTemp, Warning,
		TEXT("[ShipAI::HelmControl BASE] Ship='%hs' ObjectiveWorld=%s ShipLoc=%s AccYaw=%.4f AccPitch=%.4f Trans=(%.2f %.2f %.2f)"),
		ship ? ship->GetName() : "NULL",
		*objective.ToString(),
		*ship->GetLocation().ToString(),
		accumulator.yaw,
		accumulator.pitch,
		trans_x,
		trans_y,
		trans_z);
}


void
ShipAI::ThrottleControl()
{
	if (!ship)
	{
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[ShipAI::ThrottleControl] Ship='%hs' This=%p"),
		ship ? ship->GetName() : "NULL",
		this);

	//-------------------------------------------------------------
	// Navpoint complete stop behavior
	//-------------------------------------------------------------

	if (navpt &&
		navpt->GetStatus() == INSTRUCTION_STATUS::COMPLETE &&
		!target &&
		!threat)
	{
		throttle =
			0.0;

		old_throttle =
			0.0;

		ship->SetThrottle(0.0);
		ship->SetThrottleRequest(0.0);

		ship->SetTransX(0.0);
		ship->SetTransY(0.0);
		ship->SetTransZ(0.0);

		UE_LOG(LogTemp, Warning,
			TEXT("[ShipAI::ThrottleControl] NAV COMPLETE Ship='%hs'"),
			ship ? ship->GetName() : "NULL");

		return;
	}

	//-------------------------------------------------------------
	// Detect aggressive turn-to-target state
	//-------------------------------------------------------------

	const double AbsYaw =
		FMath::Abs(accumulator.yaw);

	const double AbsPitch =
		FMath::Abs(accumulator.pitch);

	const bool bHardTurn =
		AbsYaw > 0.60;

	const bool bExtremeTurn =
		AbsYaw > 0.90;

	const bool bTurningToFaceTarget =
		(target || threat) &&
		bHardTurn;

	//-------------------------------------------------------------
	// Navigation / cruise
	//-------------------------------------------------------------

	if (navpt && !threat && !target)
	{
		double speed =
			navpt->GetSpeed();

		if (speed > 0.0 &&
			ship->GetVelocityLimit() > 0.0)
		{
			throttle =
				speed /
				ship->GetVelocityLimit() *
				100.0;
		}
		else
		{
			throttle = 50.0;
		}
	}

	//-------------------------------------------------------------
	// Patrol
	//-------------------------------------------------------------

	else if (patrol && !threat && !target)
	{
		double speed = 200.0;

		if (distance > 5000.0)
		{
			speed = 500.0;
		}

		if (ship->GetVelocity().Size() > speed)
		{
			throttle = 0.0;
		}
		else
		{
			throttle = 50.0;
		}
	}

	//-------------------------------------------------------------
	// Combat / attack
	//-------------------------------------------------------------

	else
	{
		if (threat || target || element_index < 2)
		{
			//-----------------------------------------------------
			// TURN-IN-PLACE BEHAVIOR
			//-----------------------------------------------------

			if (bTurningToFaceTarget)
			{
				const FVector Vel =
					ship->GetVelocity();

				const double Speed =
					Vel.Size();

				//-------------------------------------------------
				// Hard braking while rotating
				//-------------------------------------------------

				throttle = 0.0;

				accumulator.brake =
					FMath::Clamp(
						AbsYaw,
						0.25,
						1.0);

				//-------------------------------------------------
				// Kill translational drift
				//-------------------------------------------------

				if (Speed > 10.0)
				{
					const double BrakeStrength =
						ship->Design()
						? ship->Design()->trans_y
						: 0.0;

					//-------------------------------------------------
					// Negative trans_y opposes forward motion
					//-------------------------------------------------

					ship->SetTransY(
						-BrakeStrength *
						accumulator.brake);
				}
				else
				{
					ship->SetTransY(0.0);
				}

				UE_LOG(LogTemp, Warning,
					TEXT("[ShipAI::ThrottleControl TURN-IN-PLACE] Ship='%hs' Target='%hs' AbsYaw=%.3f Brake=%.3f Speed=%.2f"),
					ship ? ship->GetName() : "NULL",
					target ? target->GetName() : "NULL",
					AbsYaw,
					accumulator.brake,
					Speed);

				UE_LOG(LogTemp, Warning,
					TEXT("[TURN CHECK] Ship='%hs' AbsYaw=%.3f TransY=%.3f Throttle=%.2f Speed=%.2f"),
					ship ? ship->GetName() : "NULL",
					AbsYaw,
					ship->GetTransY(),
					throttle,
					Speed);
			}

			//-----------------------------------------------------
			// Forward attack run
			//-----------------------------------------------------

			else
			{
				throttle = 100.0;

				accumulator.brake = 0.0;

				ship->SetTransY(0.0);

				if (!threat && !target)
				{
					throttle = 50.0;
				}

				if (accumulator.brake > 0.0)
				{
					throttle *=
						(1.0 - accumulator.brake);
				}
			}
		}

		//---------------------------------------------------------
		// Formation logic
		//---------------------------------------------------------

		else
		{
			Ship* lead = nullptr;

			if (ship->GetWard())
			{
				lead = ship->GetWard();
			}
			else if (ship->GetElement())
			{
				lead =
					ship->GetElement()->GetShip(1);
			}

			if (lead && lead != ship)
			{
				const double LeadSpeed =
					lead->GetVelocity().Size();

				const double ShipSpeed =
					ship->GetVelocity().Size();

				if (distance > 10000.0)
				{
					throttle = 75.0;
				}
				else if (distance > 5000.0)
				{
					throttle = 50.0;
				}
				else if (distance > 1500.0)
				{
					throttle = 25.0;
				}
				else if (LeadSpeed > 1.0)
				{
					throttle = 15.0;
				}
				else
				{
					throttle = 0.0;
				}

				UE_LOG(LogTemp, Warning,
					TEXT("[ShipAI::ThrottleControl WINGMAN] Ship='%hs' Lead='%hs' Distance=%.2f LeadSpeed=%.2f ShipSpeed=%.2f Throttle=%.2f"),
					ship ? ship->GetName() : "NULL",
					lead ? lead->GetName() : "NULL",
					distance,
					LeadSpeed,
					ShipSpeed,
					throttle);
			}
			else
			{
				throttle = 50.0;

				UE_LOG(LogTemp, Warning,
					TEXT("[ShipAI::ThrottleControl WINGMAN NO LEAD] Ship='%hs' Distance=%.2f Throttle=%.2f"),
					ship ? ship->GetName() : "NULL",
					distance,
					throttle);
			}
		}
	}

	//-------------------------------------------------------------
	// Clamp
	//-------------------------------------------------------------

	throttle =
		FMath::Clamp(
			throttle,
			0.0,
			100.0);

	old_throttle =
		throttle;

	//-------------------------------------------------------------
	// Runtime propagation
	//-------------------------------------------------------------

	ship->SetThrottle(throttle);
	ship->SetThrottleRequest(throttle);

	UE_LOG(LogTemp, Warning,
		TEXT("[ShipAI::ThrottleControl] Ship='%hs' Target='%hs' AbsYaw=%.3f AbsPitch=%.3f Throttle=%.2f Request=%.2f Brake=%.2f Vel=%s"),
		ship ? ship->GetName() : "NULL",
		target ? target->GetName() : "NULL",
		AbsYaw,
		AbsPitch,
		throttle,
		ship ? ship->GetThrottleRequest() : 0.0,
		accumulator.brake,
		ship ? *ship->GetVelocity().ToString() : TEXT("NULL"));
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
	{
		return avoid;
	}

	if (other && (other->GetLife() == 0 || other->GetIntegrity() < 1))
	{
		other = nullptr;
		last_avoid_time = 0;
	}

	if (!other && Game::GetGameTime() - last_avoid_time < 500)
	{
		return avoid;
	}

	brake = 0.0;

	double avoid_dist = 5.0 * ship->GetRadius();

	if (avoid_dist < 1e3)
	{
		avoid_dist = 1e3;
	}
	else if (avoid_dist > 12e3)
	{
		avoid_dist = 12e3;
	}

	double avoid_time = 15.0;

	if (ship->Design()->avoid_time > 0)
	{
		avoid_time = ship->Design()->avoid_time;
	}
	else if (ship->IsStarship())
	{
		avoid_time *= 1.5;
	}

	FVector bearing = ship->GetVelocity();

	if (!bearing.IsNearlyZero())
	{
		bearing.Normalize();
	}
	else
	{
		bearing = ship->GetHeading();
		bearing.Normalize();
	}

	bool found = false;

	ListIter<SimContact> contact = ship->GetContactList();

	if (other)
	{
		found =
			AvoidTestSingleObject(
				other,
				bearing,
				avoid_dist,
				avoid_time,
				avoid);
	}

	if (!found)
	{
		while (++contact && !found)
		{
			Ship* c_ship = contact->GetShip();

			if (c_ship && c_ship != ship && c_ship->IsStarship())
			{
				found =
					AvoidTestSingleObject(
						c_ship,
						bearing,
						avoid_dist,
						avoid_time,
						avoid);
			}
		}

		if (!found)
		{
			ListIter<Debris> iter =
				ship->GetRegion()->GetRocks();

			while (++iter && !found)
			{
				Debris* debris = iter.value();

				if (debris->GetMass() > ship->GetMass())
				{
					found =
						AvoidTestSingleObject(
							debris,
							bearing,
							avoid_dist,
							avoid_time,
							avoid);
				}
			}
		}

		if (!found)
		{
			avoid_dist *= 8.0;

			ListIter<Asteroid> iter =
				ship->GetRegion()->GetRoids();

			while (++iter && !found)
			{
				Asteroid* roid = iter.value();

				found =
					AvoidTestSingleObject(
						roid,
						bearing,
						avoid_dist,
						avoid_time,
						avoid);
			}

			if (!found)
			{
				avoid_dist /= 8.0;
			}
		}

		if (other)
		{
			avoid =
				Avoid(
					WorldPointToLegacyLocalObjective(obstacle, false),
					(float)(
						ship->GetRadius() +
						other->GetRadius() +
						avoid_dist * 0.9));

			avoid.brake = brake;

			ship->SetDirectorInfo(
				Game::GetText("ai.avoid-collision"));
		}
	}

	last_avoid_time = Game::GetGameTime();

	return avoid;
}

bool
ShipAI::AvoidTestSingleObject(
	SimObject* obj,
	const FVector& bearing,
	double avoid_dist,
	double& avoid_time,
	Steer& avoid)
{
	if (!ship || !obj)
	{
		return false;
	}

	if (too_close == obj->GetIdentity())
	{
		const double dist =
			(ship->GetLocation() - obj->GetLocation()).Size();

		const double closure =
			FVector::DotProduct(
				ship->GetVelocity() - obj->GetVelocity(),
				bearing);

		if (closure > 1 && dist < avoid_dist)
		{
			avoid = AvoidCloseObject(obj);
			return true;
		}
		else
		{
			too_close = 0;
		}
	}

	const double time =
		ClosestApproachTime(
			ship->GetLocation(),
			ship->GetVelocity(),
			obj->GetLocation(),
			obj->GetVelocity());

	if (time <= 0)
	{
		if (other == obj)
		{
			other = nullptr;
		}

		return false;
	}

	const FVector CurrentRelation =
		ship->GetLocation() - obj->GetLocation();

	const double current_distance =
		CurrentRelation.Size() -
		ship->GetRadius() -
		obj->GetRadius();

	if (current_distance > 25e3)
	{
		if (other == obj)
		{
			other = nullptr;
		}

		return false;
	}

	if (obj->GetType() == SimObject::SIM_SHIP)
	{
		Ship* c_ship = static_cast<Ship*>(obj);

		if (c_ship && c_ship->GetFarcaster())
		{
			FVector dir = ship->GetVelocity();

			if (!dir.IsNearlyZero())
			{
				dir.Normalize();

				double angle_off =
					FMath::Abs(
						FMath::Acos(
							(double)FVector::DotProduct(
								dir,
								obj->GetCam().vpn())));

				if (angle_off > 90 * DEGREES)
				{
					angle_off = 180 * DEGREES - angle_off;
				}

				if (angle_off < 35 * DEGREES)
				{
					const FVector d =
						ship->GetLocation() +
						dir * static_cast<float>(
							current_distance +
							ship->GetRadius() +
							obj->GetRadius());

					const double err =
						(obj->GetLocation() - d).Size();

					if (err < 0.667 * obj->GetRadius())
					{
						return false;
					}
				}
			}
		}
	}

	const double closing_velocity =
		FVector::DotProduct(
			ship->GetVelocity() - obj->GetVelocity(),
			bearing);

	if (current_distance < (avoid_dist * 0.35))
	{
		if (closing_velocity > 1 ||
			current_distance < ship->GetRadius())
		{
			avoid = AvoidCloseObject(obj);
			return true;
		}
	}

	const double separation =
		avoid_dist + obj->GetRadius();

	if (closing_velocity <= 0)
	{
		if (other == obj)
		{
			other = nullptr;
		}

		return false;
	}

	if ((current_distance - separation) / closing_velocity > avoid_time)
	{
		if (other == obj)
		{
			other = nullptr;
		}

		return false;
	}

	const FVector selfpt =
		ship->GetLocation() +
		ship->GetVelocity() * static_cast<float>(time);

	const FVector testpt =
		obj->GetLocation() +
		obj->GetVelocity() * static_cast<float>(time);

	const double dist =
		(testpt - selfpt).Size() -
		ship->GetRadius() -
		obj->GetRadius();

	if (dist < avoid_dist)
	{
		if (dist < avoid_dist * 0.25 &&
			time < avoid_time * 0.5)
		{
			avoid = AvoidCloseObject(obj);
			return true;
		}

		obstacle = testpt;

		FVector LocalObstacle =
			WorldPointToLegacyLocalObjective(obstacle, false);

		/*
		 * Legacy avoidance assumes:
		 *
		 * Local Z = forward
		 */

		if (LocalObstacle.Z > 0)
		{
			other = obj;
			avoid_time = time;
			brake = 0.5;

			Observe(other);
		}
	}
	else if (other == obj && dist > avoid_dist * 1.25)
	{
		other = nullptr;
	}

	return false;
}

// +--------------------------------------------------------------------+

Steer
ShipAI::AvoidCloseObject(SimObject* obj)
{
	if (!obj)
	{
		return Steer();
	}

	too_close = obj->GetIdentity();

	obstacle = obj->GetLocation();

	other = obj;

	Observe(other);

	Steer avoid =
		Flee(
			WorldPointToLegacyLocalObjective(obstacle, false));

	avoid.brake = 0.3;

	if (ship)
	{
		ship->SetDirectorInfo("Avoid collision");
	}

	return avoid;
}

// +--------------------------------------------------------------------+

Steer
ShipAI::SeekTarget()
{
	Steer NoSteer;

	if (!ship)
	{
		return NoSteer;
	}

	Ship* ward = ship->GetWard();

	const FVector ShipLoc = ship->GetLocation();
	const FVector ObjectiveDelta = objective - ShipLoc;

	UE_LOG(LogTemp, Warning,
		TEXT("[ShipAI::SeekTarget WORLD] Ship='%hs' ShipLoc=%s ObjectiveWorld=%s ObjectiveDelta=%s"),
		ship ? ship->GetName() : "NULL",
		*ShipLoc.ToString(),
		*objective.ToString(),
		*ObjectiveDelta.ToString());

	if (objective.ContainsNaN() ||
		!FMath::IsFinite(objective.X) ||
		!FMath::IsFinite(objective.Y) ||
		!FMath::IsFinite(objective.Z))
	{
		return NoSteer;
	}

	//-------------------------------------------------------------
	// No direct target/objective sources
	//-------------------------------------------------------------
	if (!target && !ward && !navpt && !patrol)
	{
		if (element_index > 1)
		{
			return Seek(
				WorldPointToLegacyLocalObjective(objective, false));
		}

		if (farcaster)
		{
			return Seek(
				WorldPointToLegacyLocalObjective(objective, false));
		}

		if (rumor)
		{
			return Seek(
				WorldPointToLegacyLocalObjective(objective, false));
		}

		return NoSteer;
	}

	//-------------------------------------------------------------
	// Patrol pathing
	//-------------------------------------------------------------
	if (patrol)
	{
		Steer result =
			Seek(
				WorldPointToLegacyLocalObjective(objective, false));

		if (distance < 2000.0)
		{
			result.brake = 1.0f;
		}

		return result;
	}

	//-------------------------------------------------------------
	// Collision avoidance
	//-------------------------------------------------------------
	if (target && too_close == target->GetIdentity())
	{
		drop_time = 4.0f;

		return Avoid(
			WorldPointToLegacyLocalObjective(objective, false),
			0.0f);
	}
	else if (drop_time > 0.0f)
	{
		return NoSteer;
	}

	//-------------------------------------------------------------
	// Normal seek
	//-------------------------------------------------------------
	Steer Result =
		Seek(
			WorldPointToLegacyLocalObjective(objective, false));

	UE_LOG(LogTemp, Warning,
		TEXT("[ShipAI::SeekTarget RESULT] Ship='%hs' Yaw=%.4f Pitch=%.4f Brake=%.2f"),
		ship ? ship->GetName() : "NULL",
		Result.yaw,
		Result.pitch,
		Result.brake);

	return Result;
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

FVector
ShipAI::WorldPointToLegacyLocalObjective(
	const FVector& WorldPoint,
	bool bPointIsUEWorld) const
{
	if (!ship)
	{
		return FVector::ZeroVector;
	}

	FVector LegacyWorldPoint =
		WorldPoint;

	if (bPointIsUEWorld)
	{
		LegacyWorldPoint = FVector(
			WorldPoint.Y,
			WorldPoint.Z,
			WorldPoint.X);
	}

	const FVector ShipWorld =
		ship->GetLocation();

	FVector ToTarget =
		LegacyWorldPoint - ShipWorld;

	ToTarget.Z = 0.0f;

	const double Dist =
		ToTarget.Size();

	if (Dist < KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}

	ToTarget /= Dist;

	FVector Forward =
		ship->GetHeading().GetSafeNormal();

	Forward.Z = 0.0f;

	if (!Forward.Normalize())
	{
		Forward = FVector(1.0f, 0.0f, 0.0f);
	}

	const FVector Up(
		0.0f,
		0.0f,
		1.0f);

	FVector Right =
		FVector::CrossProduct(
			Up,
			Forward).GetSafeNormal();

	if (Right.IsNearlyZero())
	{
		Right = FVector(0.0f, 1.0f, 0.0f);
	}

	const double LocalForward =
		FVector::DotProduct(
			ToTarget,
			Forward);

	const double LocalRight =
		FVector::DotProduct(
			ToTarget,
			Right);

	const double LocalUp =
		0.0f;

	const FVector Result(
		LocalRight,
		LocalUp,
		LocalForward);

	return Result;
}