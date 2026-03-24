/*  Project STARSHATTER WARS
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR: John DiCamillo
	ORIGINAL STUDIO: Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         FlightDeck.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Everything needed to launch and recover space craft...
*/

#include "FlightDeck.h"

#include "Math/Vector.h"
#include "Logging/LogMacros.h"

#include "Ship.h"
#include "ShipManager.h"
#include "ShipDesign.h"
#include "SimElement.h"
#include "Mission.h"
#include "MissionEvent.h"
#include "Drive.h"
#include "Sim.h"
#include "Instruction.h"
#include "Hoop.h"
#include "LandingGear.h"
#include "RadioMessage.h"
#include "RadioTraffic.h"
#include "SimEvent.h"
#include "AudioConfig.h"
#include "CameraManager.h"
#include "Combatant.h"
#include "CombatGroup.h"
#include "CombatUnit.h"

#include "Game.h"
#include "Solid.h"
#include "SimLight.h"
#include "Sound.h"
#include "DataLoader.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlightDeck, Log, All);

static USound* tire_sound = 0;
static USound* catapult_sound = 0;

// +======================================================================+

class FlightDeckSlot
{
public:
	FlightDeckSlot() : ship(0), state(0), sequence(0), time(0), filter(0xf), clearance(0.0) {}
	int operator == (const FlightDeckSlot& that) const { return this == &that; }

	Ship* ship;
	FVector           spot_rel;
	FVector           spot_loc;

	int               state;
	int               sequence;
	double            time;
	double            clearance;
	DWORD             filter;
};

// +======================================================================+

InboundSlot::InboundSlot(Ship* s, FlightDeck* d, int squad, int index)
	: ship(s), deck(d), squadron(squad), slot(index), cleared(0), final(0), approach(0)
{
	if (ship)
		Observe(ship);

	if (deck && deck->GetCarrier())
		Observe((SimObject*)deck->GetCarrier());
}

int InboundSlot::operator < (const InboundSlot& that) const
{
	double dthis = 0.0;
	double dthat = 0.0;

	if (ship == that.ship)
		return 0;

	// Cleared slots sort ahead of non-cleared slots:
	if (cleared && !that.cleared)
		return 1;

	if (!cleared && that.cleared)
		return 0;

	// Distance-to-deck sort as secondary key:
	if (ship && deck && that.ship)
	{
		const FVector DeckLoc = deck->MountLocation();
		dthis = (ship->Location() - DeckLoc).Size();
		dthat = (that.ship->Location() - DeckLoc).Size();
	}

	// Tie-breaker: stable ordering without 32-bit pointer truncation
	if (FMath::IsNearlyEqual(dthis, dthat))
	{
		// Note: std::less is the canonical way to order pointers in C++
		return std::less<const InboundSlot*>()(this, &that) ? 1 : 0;
	}

	return (dthis < dthat) ? 1 : 0;
}


int InboundSlot::operator <= (const InboundSlot& that) const
{
	double dthis = 0;
	double dthat = 0;

	if (ship == that.ship)
		return true;

	if (cleared && !that.cleared)
		return true;

	if (!cleared && that.cleared)
		return false;

	if (ship && deck && that.ship) {
		dthis = (ship->Location() - deck->MountLocation()).Size();
		dthat = (that.ship->Location() - deck->MountLocation()).Size();
	}

	return dthis <= dthat;
}

int InboundSlot::operator == (const InboundSlot& that) const
{
	return this == &that;
}

void InboundSlot::Clear(bool c)
{
	if (ship)
		cleared = c;
}

double InboundSlot::Distance()
{
	if (ship && deck) {
		return (ship->Location() - deck->MountLocation()).Size();
	}

	return 1e9;
}

// +----------------------------------------------------------------------+

bool
InboundSlot::Update(SimObject* obj)
{
	if (obj->Type() == SimObject::SIM_SHIP) {
		Ship* s = (Ship*)obj;

		if (s == ship) {
			ship = 0;
		}

		// Actually, this can't happen.  The flight deck
		// owns the slot.  When the carrier is destroyed,
		// the flight decks and slots are destroyed before
		// the carrier updates the observers.

		// I'm leaving this block in, just in case.

		else if (deck && s == deck->GetCarrier()) {
			ship = 0;
			deck = 0;
		}
	}

	return SimObserver::Update(obj);
}

const char*
InboundSlot::GetObserverName() const
{
	return "InboundSlot";
}

// +======================================================================+

FlightDeck::FlightDeck()
	: SimSystem(SYSTEM_CATEGORY::FLIGHT_DECK, FLIGHT_DECK_LAUNCH, "Flight Deck", 1, 1),
	carrier(0), index(0), num_slots(0), slots(0), cycle_time(5), num_hoops(0), hoops(0),
	azimuth(0), light(0), num_catsounds(0), num_approach_pts(0)
{
	name = Game::GetText("sys.flight-deck");
	abrv = Game::GetText("sys.flight-deck.abrv");
}

// +----------------------------------------------------------------------+

FlightDeck::FlightDeck(const FlightDeck& s)
	: SimSystem(s),
	carrier(0), index(0), start_rel(s.start_rel),
	end_rel(s.end_rel), cam_rel(s.cam_rel),
	cycle_time(s.cycle_time),
	num_slots(s.num_slots), slots(0),
	num_hoops(0), hoops(0), azimuth(s.azimuth), light(0),
	num_catsounds(0), num_approach_pts(s.num_approach_pts)
{
	Mount(s);

	// NOTE: MemDebug allocation removed for Unreal builds
	slots = new FlightDeckSlot[num_slots];
	for (int i = 0; i < num_slots; i++) {
		slots[i].spot_rel = s.slots[i].spot_rel;
		slots[i].filter = s.slots[i].filter;
	}

	for (int i = 0; i < NUM_APPROACH_PTS; i++)
		approach_rel[i] = s.approach_rel[i];

	for (int i = 0; i < 2; i++)
		runway_rel[i] = s.runway_rel[i];

	if (s.light) {
		// NOTE: MemDebug allocation removed for Unreal builds
		light = new SimLight(*s.light);
	}
}

// +--------------------------------------------------------------------+

FlightDeck::~FlightDeck()
{
	if (hoops && num_hoops) {
		for (int i = 0; i < num_hoops; i++) {
			Hoop* h = &hoops[i];
			SimScene* scene = h->GetScene();
			if (scene)
				scene->DelGraphic(h);
		}
	}

	delete[] slots;
	delete[] hoops;

	SIMLIGHT_DESTROY(light);

	recovery_queue.destroy();
}

// +--------------------------------------------------------------------+

void
FlightDeck::Initialize()
{
	static int initialized = 0;
	if (initialized) return;

	DataLoader* loader = DataLoader::GetLoader();
	loader->SetDataPath("Sounds/");

	const int SOUND_FLAGS = USound::LOCALIZED |
		USound::LOC_3D;

	loader->LoadSound("Tires.wav", tire_sound, SOUND_FLAGS);
	loader->LoadSound("Catapult.wav", catapult_sound, SOUND_FLAGS);
	loader->SetDataPath("");

	if (tire_sound)
		tire_sound->SetMaxDistance(2.5e3f);

	if (catapult_sound)
		catapult_sound->SetMaxDistance(0.5e3f);

	initialized = 1;
}

// +--------------------------------------------------------------------+

void
FlightDeck::Close()
{
	delete tire_sound;
	delete catapult_sound;

	tire_sound = 0;
	catapult_sound = 0;
}

// +--------------------------------------------------------------------+

void
FlightDeck::ExecFrame(double Seconds)
{
	SimSystem::ExecFrame(Seconds);

	bool bAdvanceQueue = false;

	const long MaxVol = AudioConfig::EfxVolume();
	long Volume = -1000; // keep original behavior

	Sim* SimInst = Sim::GetSim();

	if (Volume > MaxVol)
		Volume = MaxVol;

	// update ship status:
	for (int SlotIndex = 0; SlotIndex < num_slots; SlotIndex++)
	{
		FlightDeckSlot* Slot = &slots[SlotIndex];
		Ship* SlotShip = nullptr;

		if (Slot->ship == nullptr)
		{
			Slot->state = CLEAR;
		}
		else
		{
			SlotShip = Slot->ship;
			SlotShip->SetThrottle(0);
		}

		switch (Slot->state)
		{
		case CLEAR:
			if (Slot->time > 0)
			{
				Slot->time -= Seconds;
			}
			else if (IsRecoveryDeck())
			{
				GrantClearance();
			}
			break;

		case READY:
		{
			Camera C;
			C.Clone(carrier->Cam());
			C.Yaw(azimuth);

			if (SlotShip)
			{
				SlotShip->CloneCam(C);
				SlotShip->MoveTo(Slot->spot_loc);
				SlotShip->TranslateBy(carrier->Cam().vup() * Slot->clearance);
				SlotShip->SetVelocity(carrier->Velocity());
			}

			Slot->time = 0;
		}
		break;

		case QUEUED:
			if (Slot->time > 0)
			{
				Camera C;
				C.Clone(carrier->Cam());
				C.Yaw(azimuth);

				Slot->time -= Seconds;

				if (SlotShip)
				{
					SlotShip->CloneCam(C);
					SlotShip->MoveTo(Slot->spot_loc);
					SlotShip->TranslateBy(carrier->Cam().vup() * Slot->clearance);
					SlotShip->SetFlightPhase(Ship::ALERT);
				}
			}

			if (Slot->sequence == 1 && Slot->time <= 0)
			{
				bool bClearForLaunch = true;
				for (int j = 0; j < num_slots; j++)
				{
					if (slots[j].state == LOCKED)
					{
						bClearForLaunch = false;
						break;
					}
				}

				if (bClearForLaunch)
				{
					Slot->sequence = 0;
					Slot->state = LOCKED;
					Slot->time = cycle_time;

					if (SlotShip)
						SlotShip->SetFlightPhase(Ship::LOCKED);

					num_catsounds = 0;
					bAdvanceQueue = true;
				}
			}
			break;

		case LOCKED:
			if (Slot->time > 0)
			{
				Slot->time -= Seconds;

				if (SlotShip)
				{
					const double Ct4 = cycle_time / 4.0;

					// UE-friendly (but still uses your legacy Matrix/Camera types):
					const double Dx = start_rel.X - Slot->spot_rel.X;
					const double Dy = start_rel.Z - Slot->spot_rel.Z;
					const double Dyaw = atan2(Dx, Dy) - azimuth;

					Camera C;
					C.Clone(carrier->Cam());
					C.Yaw(azimuth);

					// rotate:
					if (Slot->time > 3 * Ct4)
					{
						const double Step = 1.0 - (Slot->time - 3 * Ct4) / Ct4;

						C.Yaw(Dyaw * Step);
						SlotShip->CloneCam(C);
						SlotShip->MoveTo(Slot->spot_loc);
						SlotShip->TranslateBy(carrier->Cam().vup() * Slot->clearance);

						if (carrier->IsGroundUnit())
						{
							SlotShip->SetThrottle(25);
						}
						else if (num_catsounds < 1)
						{
							if (catapult_sound)
							{
								USound* Snd = catapult_sound->Duplicate();
								if (Snd)
								{
									Snd->SetLocation(SlotShip->Location());
									Snd->SetVolume(Volume);
									Snd->Play();
								}
							}
							num_catsounds = 1;
						}
					}

					// translate:
					else if (Slot->time > 2 * Ct4)
					{
						const double Step = (Slot->time - 2 * Ct4) / Ct4;

						const FVector Loc =
							start_point + (Slot->spot_loc - start_point) * (float)Step;

						C.Yaw(Dyaw);
						SlotShip->CloneCam(C);
						SlotShip->MoveTo(Loc);
						SlotShip->TranslateBy(carrier->Cam().vup() * Slot->clearance);

						if (carrier->IsGroundUnit())
						{
							SlotShip->SetThrottle(25);
						}
						else if (num_catsounds < 2)
						{
							if (catapult_sound)
							{
								USound* Snd = catapult_sound->Duplicate();
								if (Snd)
								{
									Snd->SetLocation(SlotShip->Location());
									Snd->SetVolume(Volume);
									Snd->Play();
								}
							}
							num_catsounds = 2;
						}
					}

					// rotate:
					else if (Slot->time > Ct4)
					{
						const double Step = (Slot->time - Ct4) / Ct4;

						C.Yaw(Dyaw * Step);
						SlotShip->CloneCam(C);
						SlotShip->MoveTo(start_point);
						SlotShip->TranslateBy(carrier->Cam().vup() * Slot->clearance);

						if (carrier->IsGroundUnit())
						{
							SlotShip->SetThrottle(25);
						}
						else if (num_catsounds < 3)
						{
							if (catapult_sound)
							{
								USound* Snd = catapult_sound->Duplicate();
								if (Snd)
								{
									Snd->SetLocation(SlotShip->Location());
									Snd->SetVolume(Volume);
									Snd->Play();
								}
							}
							num_catsounds = 3;
						}
					}

					// wait:
					else
					{
						SlotShip->SetThrottle(100);
						SlotShip->CloneCam(C);
						SlotShip->MoveTo(start_point);
						SlotShip->TranslateBy(carrier->Cam().vup() * Slot->clearance);
					}

					SlotShip->SetFlightPhase(Ship::LOCKED);
				}
			}
			else
			{
				Slot->state = LAUNCH;
				Slot->time = 0;
			}
			break;

		case LAUNCH:
			LaunchShip(SlotShip);
			break;

		case DOCKING:
			if (SlotShip && !SlotShip->IsAirborne())
			{
				// do arresting gear stuff:
				if (SlotShip->GetFlightModel() == Ship::FM_ARCADE)
					SlotShip->ArcadeStop();

				SlotShip->SetVelocity(carrier->Velocity());
			}

			if (Slot->time > 0)
			{
				Slot->time -= Seconds;
			}
			else
			{
				if (SlotShip)
				{
					SlotShip->SetFlightPhase(Ship::DOCKED);
					SlotShip->Stow();

					// NetUtil::SendObjKill(SlotShip, carrier, NetObjKill::KILL_DOCK, SlotIndex);
				}

				Clear(SlotIndex);
			}
			break;

		default:
			break;
		}
	}

	if (bAdvanceQueue)
	{
		for (int i = 0; i < num_slots; i++)
		{
			FlightDeckSlot* Slot = &slots[i];
			if (Slot->state == QUEUED && Slot->sequence > 1)
				Slot->sequence--;
		}
	}
}

bool
FlightDeck::LaunchShip(Ship* slot_ship)
{
	FlightDeckSlot* slot = 0;
	Sim* sim = Sim::GetSim();

	if (slot_ship) {
		for (int i = 0; i < num_slots; i++) {
			if (slots[i].ship == slot_ship)
				slot = &(slots[i]);
		}

		// only suggest a launch point if no flight plan has been filed...
		if (slot_ship->GetElement()->FlightPlanLength() == 0) {
			// NOTE: Starshatter's OtherHand() coordinate-hand conversion must be handled by your project.
			FVector departure = end_point;

			Instruction* launch_point =
				new Instruction(carrier->GetRegion(), departure, INSTRUCTION_ACTION::LAUNCH);
			launch_point->SetSpeed(350);

			slot_ship->SetLaunchPoint(launch_point);
		}

		if (!slot_ship->IsAirborne()) {
			FVector cat;

			if (fabs(azimuth) < 5 * DEGREES) {
				cat = carrier->Heading() * 300.0f;
			}
			else {
				Camera c;
				c.Clone(carrier->Cam());
				c.Yaw(azimuth);
				cat = c.vpn() * 300.0f;
			}

			slot_ship->SetVelocity(carrier->Velocity() + cat);
			slot_ship->SetFlightPhase(Ship::LAUNCH);
		}
		else {
			slot_ship->SetFlightPhase(Ship::TAKEOFF);
		}

		SimDirector* dir = slot_ship->GetDirector();
		if (dir && dir->Type() == ShipManager::DIR_TYPE) {
			ShipManager* ctrl = (ShipManager*)dir;
			ctrl->Launch();
		}

		ShipStats* c = ShipStats::Find(carrier->Name());
		if (c) c->AddEvent(SimEvent::LAUNCH_SHIP, slot_ship->Name());

		ShipStats* stats = ShipStats::Find(slot_ship->Name());
		if (stats) {
			stats->SetRegion(carrier->GetRegion()->GetName());
			stats->SetType(slot_ship->Design()->name);

			if (slot_ship->GetElement()) {
				SimElement* elem = slot_ship->GetElement();
				stats->SetRole(Mission::RoleName(elem->Type()));
				stats->SetCombatGroup(elem->GetCombatGroup());
				stats->SetCombatUnit(elem->GetCombatUnit());
				stats->SetElementIndex(slot_ship->GetElementIndex());
			}

			stats->SetIFF(slot_ship->GetIFF());
			stats->AddEvent(SimEvent::LAUNCH, carrier->Name());

			if (slot_ship == sim->GetPlayerShip())
				stats->SetPlayer(true);
		}

		sim->ProcessEventTrigger(MissionEvent::TRIGGER_LAUNCH, 0, slot_ship->Name());

		if (slot) {
			slot->ship = 0;
			slot->state = CLEAR;
			slot->sequence = 0;
			slot->time = 0;
		}

		return true;
	}

	return false;
}

// +----------------------------------------------------------------------+

void
FlightDeck::SetLight(double l)
{
	SIMLIGHT_DESTROY(light);
	light = new SimLight((float)l);
}

void
FlightDeck::SetApproachPoint(int i, FVector loc)
{
	if (i >= 0 && i < NUM_APPROACH_PTS) {
		approach_rel[i] = loc;

		if (i >= num_approach_pts)
			num_approach_pts = i + 1;
	}
}

void
FlightDeck::SetRunwayPoint(int i, FVector loc)
{
	if (i >= 0 && i < 2)
		runway_rel[i] = loc;
}

void
FlightDeck::SetStartPoint(FVector loc)
{
	start_rel = loc;
}

void
FlightDeck::SetEndPoint(FVector loc)
{
	end_rel = loc;
}

void
FlightDeck::SetCamLoc(FVector loc)
{
	cam_rel = loc;
}

// +----------------------------------------------------------------------+

void
FlightDeck::AddSlot(const FVector& spot, DWORD filter)
{
	if (!slots)
		slots = new FlightDeckSlot[10];

	slots[num_slots].spot_rel = spot;
	slots[num_slots].filter = filter;
	num_slots++;
}

// +----------------------------------------------------------------------+

void
FlightDeck::SetCycleTime(double t)
{
	cycle_time = t;
}

// +----------------------------------------------------------------------+

void
FlightDeck::Orient(const Physical* rep)
{
	// Base orientation logic:
	SimSystem::Orient(rep);

	if (!rep)
		return;

	/*
		Starshatter camera basis:
			vrt = right
			vup = up
			vpn = forward (view-plane normal)
	*/

	const FVector RepLoc = rep->Location();

	// Extract and normalize camera basis:
	FVector XAxis = rep->Cam().vrt(); // right
	FVector YAxis = rep->Cam().vup(); // up
	FVector ZAxis = rep->Cam().vpn(); // forward

	XAxis = XAxis.GetSafeNormal();
	YAxis = YAxis.GetSafeNormal();
	ZAxis = ZAxis.GetSafeNormal();

	// Defensive orthonormalization:
	ZAxis = (ZAxis - (ZAxis | XAxis) * XAxis).GetSafeNormal();
	YAxis = (XAxis ^ ZAxis).GetSafeNormal();

	// Build world transform from camera frame:
	const FMatrix CamM(
		FPlane(XAxis.X, XAxis.Y, XAxis.Z, 0.0f),
		FPlane(YAxis.X, YAxis.Y, YAxis.Z, 0.0f),
		FPlane(ZAxis.X, ZAxis.Y, ZAxis.Z, 0.0f),
		FPlane(RepLoc.X, RepLoc.Y, RepLoc.Z, 1.0f)
	);

	const FTransform RepXform(CamM);

	// Transform deck-relative points into world space:
	start_point = RepXform.TransformPosition(start_rel);
	end_point = RepXform.TransformPosition(end_rel);
	cam_loc = RepXform.TransformPosition(cam_rel);

	for (int i = 0; i < num_approach_pts; i++)
		approach_point[i] = RepXform.TransformPosition(approach_rel[i]);

	for (int i = 0; i < num_slots; i++)
		slots[i].spot_loc = RepXform.TransformPosition(slots[i].spot_rel);

	// ------------------------------------------------------------------
	// Recovery deck logic:
	// ------------------------------------------------------------------

	if (IsRecoveryDeck())
	{
		if (carrier && carrier->IsAirborne())
		{
			runway_point[0] = RepXform.TransformPosition(runway_rel[0]);
			runway_point[1] = RepXform.TransformPosition(runway_rel[1]);
		}

		if (num_hoops < 1)
		{
			num_hoops = 4;
			hoops = new Hoop[num_hoops];
		}

		const FVector HoopVecRaw = start_point - end_point;
		const float   HoopLen = HoopVecRaw.Size();
		const FVector HoopDir =
			(HoopLen > KINDA_SMALL_NUMBER) ? (HoopVecRaw / HoopLen) : FVector::ZeroVector;

		const float HoopStep =
			(num_hoops > 0) ? (HoopLen / (float)num_hoops) : 0.0f;

		// Base rotation from the deck:
		const FQuat RepQ = RepXform.GetRotation();

		// Apply azimuth (legacy value is radians) around deck UP axis:
		const FQuat AzQ(YAxis, (float)azimuth);

		// World-space azimuth:
		const FQuat HoopQ = AzQ * RepQ;

		// Solid expects FMatrix:
		const FMatrix HoopM = FQuatRotationMatrix(HoopQ);

		for (int i = 0; i < num_hoops; i++)
		{
			const FVector HoopPos =
				end_point + HoopDir * (HoopStep * (float)(i + 1));

			hoops[i].MoveTo(HoopPos);
			hoops[i].SetOrientation(HoopM);
		}
	}

	// ------------------------------------------------------------------
	// Mount + light:
	// ------------------------------------------------------------------

	mount_loc = RepXform.TransformPosition(mount_rel);

	if (light)
		light->MoveTo(mount_loc);
}

// +----------------------------------------------------------------------+

int
FlightDeck::SpaceLeft(int Type) const
{
	int SpaceLeftCount = 0;

	for (int i = 0; i < num_slots; ++i)
	{
		if (slots[i].ship == nullptr && (slots[i].filter & Type))
		{
			++SpaceLeftCount;
		}
	}

	return SpaceLeftCount;
}

// +----------------------------------------------------------------------+

bool FlightDeck::Spot(Ship* s, int& outIndex)
{
	if (!s)
		return false;

	// Build ship compatibility mask from classification
	// (Starshatter-style: one bit per ship class)
	const uint32 ShipMask = 1u << static_cast<uint32>(s->Class());

	// If caller did not specify a slot, find first compatible free slot
	if (outIndex < 0)
	{
		outIndex = -1;

		for (int i = 0; i < num_slots; ++i)
		{
			const uint32 SlotMask = static_cast<uint32>(slots[i].filter);

			if (slots[i].ship == nullptr &&
				(SlotMask & ShipMask) != 0)
			{
				outIndex = i;
				break;
			}
		}
	}

	// Validate slot index
	if (outIndex < 0 || outIndex >= num_slots)
		return false;

	// Slot must be empty
	if (slots[outIndex].ship != nullptr)
		return false;

	// Assign ship to slot
	slots[outIndex].state = READY;
	slots[outIndex].ship = s;
	slots[outIndex].clearance = 0.0f;

	if (LandingGear* Gear = s->GetGear())
		slots[outIndex].clearance = Gear->GetClearance();

	// Recovery deck: bleed off velocity if already landed
	if (IsRecoveryDeck() && !s->IsAirborne())
	{
		s->SetVelocity(s->Velocity() * 0.01);
	}

	// Non-recovery deck positioning
	if (!IsRecoveryDeck())
	{
		Camera WorkCam;
		WorkCam.Clone(carrier->Cam());
		WorkCam.Yaw(azimuth);

		s->CloneCam(WorkCam);
		s->MoveTo(slots[outIndex].spot_loc);

		if (LandingGear* Gear = s->GetGear())
		{
			Gear->SetState(LandingGear::GEAR_DOWN);
			Gear->ExecFrame(0);

			const FVector Up = carrier->Cam().vup();
			s->TranslateBy(Up * slots[outIndex].clearance);
		}

		s->SetFlightPhase(Ship::ALERT);
	}

	// Finalize carrier ownership and observation
	s->SetCarrier(carrier, this);
	Observe(s);

	return true;
}

bool
FlightDeck::Clear(int slotIndex)
{
	if (slotIndex < 0 || slotIndex >= num_slots)
		return false;

	if (slots[slotIndex].ship == nullptr)
		return false;

	Ship* s = slots[slotIndex].ship;

	slots[slotIndex].ship = nullptr;
	slots[slotIndex].time = cycle_time;
	slots[slotIndex].state = CLEAR;

	// Remove ship from recovery queue (legacy: delete removed item):
	ListIter<InboundSlot> iter = recovery_queue;
	while (++iter)
	{
		if (iter->GetShip() == s)
		{
			delete iter.removeItem(); // legacy ownership model
			break;
		}
	}

	return true;
}


// +----------------------------------------------------------------------+

bool
FlightDeck::Launch(int slotIndex)
{
	// Avoid C4458 if FlightDeck has a member named "index"
	if (subtype != FLIGHT_DECK_LAUNCH)
		return false;

	if (slotIndex < 0 || slotIndex >= num_slots)
		return false;

	FlightDeckSlot* slot = &slots[slotIndex];

	if (slot->ship == nullptr || slot->state != READY)
		return false;

	int lastSeq = 0;
	FlightDeckSlot* lastQueuedSlot = nullptr;
	FlightDeckSlot* lockedSlot = nullptr;

	for (int i = 0; i < num_slots; i++)
	{
		FlightDeckSlot* s = &slots[i];

		if (s->state == QUEUED && s->sequence > lastSeq)
		{
			lastSeq = s->sequence;
			lastQueuedSlot = s;
		}
		else if (s->state == LOCKED)
		{
			lockedSlot = s;
		}
	}

	// Queue the slot for launch:
	slot->state = QUEUED;
	slot->sequence = lastSeq + 1;
	slot->time = 0;

	if (lastQueuedSlot)
	{
		slot->time = lastQueuedSlot->time + cycle_time;
	}
	else if (lockedSlot)
	{
		slot->time = lockedSlot->time;
	}

	return true;
}


// +----------------------------------------------------------------------+

bool
FlightDeck::Recover(Ship* s)
{
	if (!s)
		return false;

	if (subtype != FLIGHT_DECK_RECOVERY)
		return false;

	if (!OverThreshold(s))
	{
		// If the ship fell back under the approach threshold, revert RECOVERY -> ACTIVE:
		if (s->GetFlightPhase() == Ship::RECOVERY)
			s->SetFlightPhase(Ship::ACTIVE);

		return false;
	}

	// Ensure ship is in RECOVERY phase and bound to this carrier/deck for dock camera logic:
	if (s->GetFlightPhase() < Ship::RECOVERY)
	{
		s->SetFlightPhase(Ship::RECOVERY);
		s->SetCarrier(carrier, this);
	}

	// "Are we there yet?" Docking gate:
	if (s->GetFlightPhase() >= Ship::ACTIVE && s->GetFlightPhase() < Ship::DOCKING)
	{
		// UE FIX: nullptr instead of 0
		// Legacy behavior: only check docking clearance if slot 0 is empty (approach gate)
		if (slots[0].ship == nullptr)
		{
			const FVector DeckMountLoc = MountLocation();
			const FVector ShipLoc = s->Location();

			const double DockDistance = (ShipLoc - DeckMountLoc).Size();

			if (s->IsAirborne())
			{
				// UE FIX: FVector uses Z as "up" in Unreal. Starshatter often used Y as up.
				// If your port standardizes to Unreal coordinates, use Z; if not, keep Y.
				// Pick ONE and keep it consistent engine-wide.
				const double Altitude =
#if 1   // set to 0 if your world-up is Y
					ShipLoc.Z - DeckMountLoc.Z;
#else
					ShipLoc.Y - DeckMountLoc.Y;
#endif

				if (DockDistance < Radius() * 3.0 && Altitude < s->Radius())
					Dock(s);
			}
			else
			{
				if (DockDistance < s->Radius())
					Dock(s);
			}
		}
	}

	return true;
}


bool
FlightDeck::Dock(Ship* s)
{
	if (!s)
		return false;

	if (subtype != FLIGHT_DECK_RECOVERY)
		return false;

	if (slots[0].time > 0)
		return false;

	int slotIndex = 0;
	if (!Spot(s, slotIndex))
		return false;

	s->SetFlightPhase(Ship::DOCKING);
	s->SetCarrier(carrier, this);

	// hard landings?
	if (Ship::GetLandingModel() == 0)
	{
		const double base_damage = s->Design()->integrity;

		// did player do something stupid?
		if (s->GetGear() && !s->IsGearDown())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("FlightDeck::Dock(%s) Belly landing!"),
				ANSI_TO_TCHAR(s->Name()));
			s->InflictDamage(0.5 * base_damage);
		}

		const double docking_deflection =
			FMath::Abs(carrier->Cam().vup().Y - s->Cam().vup().Y);

		if (docking_deflection > 0.35)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("Landing upside down? y = %.3f"),
				docking_deflection);
			s->InflictDamage(0.8 * base_damage);
		}

		// did incoming ship exceed safe landing parameters?
		if (s->IsAirborne())
		{
			if (s->Velocity().Y < -20.0)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("FlightDeck::Dock(%s) Slammed it!"),
					ANSI_TO_TCHAR(s->Name()));
				s->InflictDamage(0.1 * base_damage);
			}
		}
		// did incoming ship exceed safe docking speed?
		else
		{
			const FVector DeltaV = s->Velocity() - carrier->Velocity();
			const double  Excess = DeltaV.Size() - 100.0;

			if (Excess > 0.0)
				s->InflictDamage(Excess);
		}
	}

	if (s->IsAirborne())
	{
		if (s == Sim::GetSim()->GetPlayerShip() && tire_sound)
		{
			USound* sound = tire_sound->Duplicate();
			sound->Play();
		}
	}

	if (s->GetDrive())
		s->GetDrive()->PowerOff();

	slots[slotIndex].state = DOCKING;
	slots[slotIndex].time = s->IsAirborne() ? 7.5 : 5.0;

	return true;
}


int
FlightDeck::Inbound(InboundSlot*& s)
{
	if (s && s->GetShip()) {
		Ship* inbound = s->GetShip();

		if (recovery_queue.contains(s)) {
			InboundSlot* orig = s;
			s = recovery_queue.find(s);
			delete orig;
		}
		else {
			recovery_queue.append(s);
			Observe(s->GetShip());
		}

		inbound->SetInbound(s);

		// find the best initial approach point for this ship:
		double current_distance = 1e9;
		for (int i = 0; i < num_approach_pts; i++) {
			const double distance = (inbound->Location() - approach_point[i]).Size();
			if (distance < current_distance) {
				current_distance = distance;
				s->SetApproach(i);
			}
		}

		FVector offset(
			(float)(rand() - 16000),
			(float)(rand() - 16000),
			(float)(rand() - 16000)
		);
		offset = offset.GetSafeNormal() * 200.0f;

		s->SetOffset(offset);

		// *** DEBUG ***
		// PrintQueue();

		// if the new guy is first in line, and the deck is almost ready,
		// go ahead and clear him for approach now
		recovery_queue.sort();
		if (recovery_queue[0] == s && !s->Cleared() && slots[0].time <= 3)
			s->Clear();

		return recovery_queue.index(s) + 1;
	}

	return 0;
}

void
FlightDeck::GrantClearance()
{
	if (recovery_queue.size() > 0) {
		if (recovery_queue[0]->Cleared() && recovery_queue[0]->Distance() > 45e3) {
			recovery_queue[0]->Clear(false);
		}

		if (!recovery_queue[0]->Cleared()) {
			recovery_queue.sort();

			if (recovery_queue[0]->Distance() < 35e3) {
				recovery_queue[0]->Clear();

				Ship* dst = recovery_queue[0]->GetShip();

				RadioMessage* clearance = new RadioMessage(dst, carrier, RadioMessageAction::CALL_CLEARANCE);
				clearance->SetInfo(Text("for final approach to ") + Name());
				RadioTraffic::Transmit(clearance);
			}
		}
	}
}

// +----------------------------------------------------------------------+

void
FlightDeck::PrintQueue()
{
	UE_LOG(LogTemp, Log, TEXT("Recovery Queue for %s"), ANSI_TO_TCHAR(Name()));

	if (recovery_queue.size() < 1) {
		UE_LOG(LogTemp, Log, TEXT("  (empty)"));
		return;
	}

	for (int i = 0; i < recovery_queue.size(); i++) {
		InboundSlot* s = recovery_queue.at(i);

		if (!s) {
			UE_LOG(LogTemp, Log, TEXT("  %2d. null"), i);
		}
		else if (!s->GetShip()) {
			UE_LOG(LogTemp, Log, TEXT("  %2d. ship is null"), i);
		}
		else {
			const double d = (s->GetShip()->Location() - MountLocation()).Size();
			UE_LOG(
				LogTemp,
				Log,
				TEXT("  %2d. %c %-20s %8d km"),
				i,
				s->Cleared() ? TEXT('*') : TEXT(' '),
				ANSI_TO_TCHAR(s->GetShip()->Name()),
				(int)(d / 1000.0)
			);
		}
	}
}

// +----------------------------------------------------------------------+

Ship*
FlightDeck::GetShip(int slotIndex) const
{
	if (slotIndex >= 0 && slotIndex < num_slots)
		return slots[slotIndex].ship;

	return nullptr;
}

double
FlightDeck::TimeRemaining(int slotIndex) const
{
	if (slotIndex >= 0 && slotIndex < num_slots)
		return slots[slotIndex].time;

	return 0.0;
}

int
FlightDeck::State(int slotIndex) const
{
	if (slotIndex >= 0 && slotIndex < num_slots)
		return slots[slotIndex].state;

	return 0;
}

int
FlightDeck::Sequence(int slotIndex) const
{
	if (slotIndex >= 0 && slotIndex < num_slots)
		return slots[slotIndex].sequence;

	return 0;
}

// +----------------------------------------------------------------------+

bool
FlightDeck::Update(SimObject* obj)
{
	if (obj->Type() == SimObject::SIM_SHIP) {
		Ship* s = (Ship*)obj;

		ListIter<InboundSlot> iter = recovery_queue;
		while (++iter) {
			InboundSlot* recovery_slot = iter.value();

			if (recovery_slot->GetShip() == s || recovery_slot->GetShip() == 0) {
				delete iter.removeItem();
			}
		}

		for (int i = 0; i < num_slots; i++) {
			FlightDeckSlot* slot = &slots[i];

			if (slot->ship == s) {
				slot->ship = 0;
				slot->state = 0;
				slot->sequence = 0;
				slot->time = 0;
				break;
			}
		}
	}

	return SimObserver::Update(obj);
}

const char*
FlightDeck::GetObserverName() const
{
	return Name();
}

// +----------------------------------------------------------------------+

bool
FlightDeck::OverThreshold(Ship* s) const
{
	if (carrier->IsAirborne()) {
		if (s->AltitudeAGL() > s->Radius() * 4)
			return false;

		const FVector sloc = s->Location();

		// is ship between the markers?
		double distance = 1e9;

		const FVector d0 = runway_point[0] - sloc;
		const FVector d1 = runway_point[1] - sloc;
		const double  dd = FVector::DotProduct(d0, d1);
		const bool    between = (dd < 0);

		if (between) {
			// distance from point to line:
			const FVector src = runway_point[0];
			const FVector dir = runway_point[1] - src;
			const FVector w = FVector::CrossProduct((sloc - src), dir);

			const double dir_len = dir.Size();
			if (dir_len > 0)
				distance = w.Size() / dir_len;
		}

		return distance < Radius();
	}

	return (s->Location() - MountLocation()).Size() < (s->Radius() + Radius());
}

// +----------------------------------------------------------------------+

bool
FlightDeck::ContainsPoint(const FVector& p) const
{
	return (p - MountLocation()).Size() < Radius();
}
