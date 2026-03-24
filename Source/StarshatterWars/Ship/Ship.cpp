/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         Ship.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios

	OVERVIEW
	========
	Starship class
*/
#include "Ship.h"
#include "CoreMinimal.h"        // UE_LOG, FVector, basic UE types
#include "Math/Vector.h"        // FVector (explicit)
#include <cstring>
#include <cstdio>

// Starshatter core includes (kept):

#include "ShipAI.h"
#include "ShipManager.h"
#include "ShipDesign.h"
#include "ShipKiller.h"
#include "SimShot.h"
#include "Drone.h"
#include "SeekerAI.h"
#include "HardPoint.h"
#include "Weapon.h"
#include "WeaponGroup.h"
#include "Shield.h"
#include "ShieldRep.h"
#include "Computer.h"
#include "FlightComputer.h"
#include "Drive.h"
#include "QuantumDrive.h"
#include "Farcaster.h"
#include "Thruster.h"
#include "Power.h"
#include "FlightDeck.h"
#include "LandingGear.h"
#include "Hangar.h"
#include "Sensor.h"
#include "SimContact.h"
#include "CombatUnit.h"
#include "SimElement.h"
#include "Instruction.h"
#include "RadioMessage.h"
#include "RadioHandler.h"
#include "RadioTraffic.h"
#include "NavLight.h"
#include "NavSystem.h"
#include "NavAI.h"
#include "DropShipAI.h"
#include "Explosion.h"
#include "MissionEvent.h"
#include "ShipSolid.h"
#include "Sim.h"
#include "SimEvent.h"
#include "StarSystem.h"
#include "TerrainRegion.h"
#include "Terrain.h"
#include "SimSystem.h"
#include "SimComponent.h"
#include "KeyMap.h"
#include "RadioView.h"
#include "AudioConfig.h"
#include "CameraManager.h"
#include "HUDView.h"
#include "Random.h"
#include "RadioVox.h"

#include "MotionController.h"
#include "Keyboard.h"
#include "Joystick.h"
#include "Bolt.h"
#include "Game.h"
#include "Solid.h"
#include "SimModel.h"
#include "Shadow.h"
#include "Skin.h"
#include "Sprite.h"
#include "SimLight.h"
#include "Bitmap.h"
#include "UIButton.h"
#include "Sound.h"
#include "DataLoader.h"

#include "Parser.h"
#include "Reader.h"
#include "GameStructs.h"
#include "StarshatterWarsLog.h"


// ---------------------------------------------------------------------
// NOTE ON POINT/VEC3 CONVERSION
// ---------------------------------------------------------------------
// Per your instruction, Point/Vec3 usages are being migrated toward FVector.
// This file was pasted with extensive Point/Matrix math (length(), cross(),
// OtherHand(), etc.) that does notxmap 1:1 to FVector without additional
// helper shims. This conversion is applied where the pasted content allows
// mechanical replacement without inventing missing engine glue.
// ---------------------------------------------------------------------


// +----------------------------------------------------------------------+

static int     base_contact_id = 0;
static double  range_min = 0;
static double  range_max = 250e3;

int      Ship::control_model = 0; // standard
int      Ship::flight_model = 0; // standard
int      Ship::landing_model = 0; // standard
double   Ship::friendly_fire_level = 1; // 100%

const int HIT_NOTHING = 0;
const int HIT_HULL = 1;
const int HIT_SHIELD = 2;
const int HIT_BOTH = 3;
const int HIT_TURRET = 4;


static FORCEINLINE FMatrix ToFMatrix(const Matrix& InM)
{
	FMatrix Out = FMatrix::Identity;

	Out.M[0][0] = (float)InM(0, 0);
	Out.M[0][1] = (float)InM(0, 1);
	Out.M[0][2] = (float)InM(0, 2);

	Out.M[1][0] = (float)InM(1, 0);
	Out.M[1][1] = (float)InM(1, 1);
	Out.M[1][2] = (float)InM(1, 2);

	Out.M[2][0] = (float)InM(2, 0);
	Out.M[2][1] = (float)InM(2, 1);
	Out.M[2][2] = (float)InM(2, 2);

	return Out;
}

// +----------------------------------------------------------------------+

Ship::Ship(const char* ship_name, const char* reg_num, ShipDesign* ship_dsn, int IFF, int cmd_ai, const int* load)
	: IFF_code(IFF), killer(0), throttle(0), augmenter(false), throttle_request(0),
	shield(0), shieldRep(0), main_drive(0), quantum_drive(0), farcaster(0),
	check_fire(false), probe(0), sensor_drone(0), primary(0), secondary(1),
	cmd_chain_index(0), target(0), subtarget(0), radio_orders(0), launch_point(0),
	g_force(0.0f), sensor(0), navsys(0), flcs(0), hangar(0), respawns(0), invulnerable(false),
	thruster(0), decoy(0), ai_mode(2), command_ai_level(cmd_ai), flcs_mode(FLCS_AUTO), loadout(0),
	emcon(3), old_emcon(3), master_caution(false), cockpit(0), gear(0), skin(0),
	auto_repair(true), last_repair_time(0), last_eval_time(0), last_beam_time(0), last_bolt_time(0),
	warp_fov(1), flight_phase(LAUNCH), launch_time(0), carrier(0), dock(0), ff_count(0),
	inbound(0), element(0), director_info("Init"), combat_unit(0), net_control(0),
	track(0), ntrack(0), track_time(0), helm_heading(0.0f), helm_pitch(0.0f),
	altitude_agl(-1.0e6f), transition_time(0.0f), transition_type(TRANSITION_NONE),
	friendly_fire_time(0), ward(0), net_observer_mode(false), orig_elem_index(-1)
{
	sim = Sim::GetSim();

	strcpy_s(name, ship_name);
	if (reg_num && *reg_num)
		strcpy_s(regnum, reg_num);
	else
		regnum[0] = 0;

	design = ship_dsn;

	if (!design) {
		char msg[256];
		sprintf_s(msg, "No ship design found for '%s'\n", ship_name);

		UE_LOG(LogStarshatterWars, Error, TEXT("%hs"), msg);
		Game::Panic(msg);
	}

	obj_type = SimObject::SIM_SHIP;

	radius = design->radius;
	mass = design->mass;
	integrity = design->integrity;
	vlimit = design->vlimit;

	agility = design->agility;
	wep_mass = 0.0f;
	wep_resist = 0.0f;

	CL = design->CL;
	CD = design->CD;
	stall = design->stall;

	chase_vec = design->chase_vec;
	bridge_vec = design->bridge_vec;

	acs = design->acs;
	pcs = design->acs;

	auto_repair = design->repair_auto;

	while (!base_contact_id)
		base_contact_id = rand() % 1000;

	contact_id = base_contact_id++;
	int sys_id = 0;

	for (int i = 0; i < design->reactors.size(); i++) {
		PowerSource* reactor = new PowerSource(*design->reactors[i]);
		reactor->SetShip(this);
		reactor->SetID(sys_id++);
		reactors.append(reactor);
		systems.append(reactor);
	}

	for (int i = 0; i < design->drives.size(); i++) {
		Drive* drive = new Drive(*design->drives[i]);
		drive->SetShip(this);
		drive->SetID(sys_id++);

		int src_index = drive->GetSourceIndex();
		if (src_index >= 0 && src_index < reactors.size())
			reactors[src_index]->AddClient(drive);

		drives.append(drive);
		systems.append(drive);
	}

	if (design->quantum_drive) {
		quantum_drive = new QuantumDrive(*design->quantum_drive);
		quantum_drive->SetShip(this);
		quantum_drive->SetID(sys_id++);

		int src_index = quantum_drive->GetSourceIndex();
		if (src_index >= 0 && src_index < reactors.size())
			reactors[src_index]->AddClient(quantum_drive);

		quantum_drive->SetShip(this);
		systems.append(quantum_drive);
	}

	if (design->farcaster) {
		farcaster = new Farcaster(*design->farcaster);
		farcaster->SetShip(this);
		farcaster->SetID(sys_id++);

		int src_index = farcaster->GetSourceIndex();
		if (src_index >= 0 && src_index < reactors.size())
			reactors[src_index]->AddClient(farcaster);

		farcaster->SetShip(this);
		systems.append(farcaster);
	}

	if (design->thruster) {
		thruster = new Thruster(*design->thruster);
		thruster->SetShip(this);
		thruster->SetID(sys_id++);

		int src_index = thruster->GetSourceIndex();
		if (src_index >= 0 && src_index < reactors.size())
			reactors[src_index]->AddClient(thruster);

		thruster->SetShip(this);
		systems.append(thruster);
	}

	if (design->shield) {
		shield = new Shield(*design->shield);
		shield->SetShip(this);
		shield->SetID(sys_id++);

		int src_index = shield->GetSourceIndex();
		if (src_index >= 0 && src_index < reactors.size())
			reactors[src_index]->AddClient(shield);

		if (design->shield_model) {
			shieldRep = new ShieldRep;
			shieldRep->UseModel(design->shield_model);
		}

		systems.append(shield);
	}

	for (int i = 0; i < design->flight_decks.size(); i++) {
		FlightDeck* deck = new FlightDeck(*design->flight_decks[i]);
		deck->SetShip(this);
		deck->SetCarrier(this);
		deck->SetID(sys_id++);
		deck->SetIndex(i);

		int src_index = deck->GetSourceIndex();
		if (src_index >= 0 && src_index < reactors.size())
			reactors[src_index]->AddClient(deck);

		flight_decks.append(deck);
		systems.append(deck);
	}

	if (design->flight_decks.size() > 0) {
		if (!hangar) {
			hangar = new Hangar;
			hangar->SetShip(this);
		}
	}

	if (design->squadrons.size() > 0) {
		if (!hangar) {
			hangar = new Hangar;
			hangar->SetShip(this);
		}

		for (int i = 0; i < design->squadrons.size(); i++) {
			ShipSquadron* s = design->squadrons[i];
			hangar->CreateSquadron(s->name, 0, s->design, s->count, GetIFF(), 0, 0, s->avail);
		}
	}

	if (design->gear) {
		gear = new LandingGear(*design->gear);
		gear->SetShip(this);
		gear->SetID(sys_id++);

		int src_index = gear->GetSourceIndex();
		if (src_index >= 0 && src_index < reactors.size())
			reactors[src_index]->AddClient(gear);

		systems.append(gear);
	}

	if (design->sensor) {
		sensor = new Sensor(*design->sensor);
		sensor->SetShip(this);
		sensor->SetID(sys_id++);

		int src_index = sensor->GetSourceIndex();
		if (src_index >= 0 && src_index < reactors.size())
			reactors[src_index]->AddClient(sensor);

		if (IsStarship() || IsStatic() || !strncmp(design->name, "Camera", 6))
			sensor->SetMode(Sensor::CST);

		systems.append(sensor);
	}

	int wep_index = 1;

	for (int i = 0; i < design->weapons.size(); i++) {
		Weapon* gun = new Weapon(*design->weapons[i]);
		gun->SetID(sys_id++);
		gun->SetOwner(this);
		gun->SetIndex(wep_index++);

		int src_index = gun->GetSourceIndex();
		if (src_index >= 0 && src_index < reactors.size())
			reactors[src_index]->AddClient(gun);

		WeaponGroup* group = FindWeaponGroup(gun->Group());
		group->AddWeapon(gun);
		group->SetAbbreviation(gun->Abbreviation());

		systems.append(gun);

		if (IsDropship() && gun->GetTurret())
			gun->SetFiringOrders(WeaponsOrders::POINT_DEFENSE);
		else
			gun->SetFiringOrders(WeaponsOrders::MANUAL);
	}

	int loadout_size = design->hard_points.size();

	if (load && loadout_size > 0) {
		loadout = new int[loadout_size];

		for (int i = 0; i < loadout_size; i++) {
			int mounted_weapon = loadout[i] = load[i];

			if (mounted_weapon < 0)
				continue;

			Weapon* missile = design->hard_points[i]->CreateWeapon(mounted_weapon);

			if (missile) {
				missile->SetID(sys_id++);
				missile->SetOwner(this);
				missile->SetIndex(wep_index++);

				WeaponGroup* group = FindWeaponGroup(missile->Group());
				group->AddWeapon(missile);
				group->SetAbbreviation(missile->Abbreviation());

				systems.append(missile);
			}
		}
	}

	if (weapons.size() > 1) {
		primary = -1;
		secondary = -1;

		for (int i = 0; i < weapons.size(); i++) {
			WeaponGroup* group = weapons[i];
			if (group->IsPrimary() && primary < 0) {
				primary = i;

				// turrets on fighters are set to point defense by default,
				// this forces the primary turret back to manual control
				group->SetFiringOrders(WeaponsOrders::MANUAL);
			}

			else if (group->IsMissile() && secondary < 0) {
				secondary = i;
			}
		}

		if (primary < 0)   primary = 0;
		if (secondary < 0)   secondary = 1;

		if (weapons.size() > 4) {
			UE_LOG(LogStarshatterWars, Warning,
				TEXT("Ship '%hs' type '%hs' has %d wep groups (max=4)"),
				Name(), DesignName(), weapons.size());
		}
	}

	if (design->decoy) {
		decoy = new Weapon(*design->decoy);
		decoy->SetOwner(this);
		decoy->SetID(sys_id++);
		decoy->SetIndex(wep_index++);

		int src_index = decoy->GetSourceIndex();
		if (src_index >= 0 && src_index < reactors.size())
			reactors[src_index]->AddClient(decoy);

		systems.append(decoy);
	}

	for (int i = 0; i < design->navlights.size(); i++) {
		NavLight* navlight = new NavLight(*design->navlights[i]);
		navlight->SetShip(this);
		navlight->SetID(sys_id++);
		navlight->SetOffset(static_cast<uint32>(reinterpret_cast<UPTRINT>(this) << 2));
		navlights.append(navlight);
		systems.append(navlight);
	}

	if (design->navsys) {
		navsys = new NavSystem(*design->navsys);
		navsys->SetShip(this);
		navsys->SetID(sys_id++);

		int src_index = navsys->GetSourceIndex();
		if (src_index >= 0 && src_index < reactors.size())
			reactors[src_index]->AddClient(navsys);

		systems.append(navsys);
	}

	if (design->probe) {
		probe = new Weapon(*design->probe);
		probe->SetOwner(this);
		probe->SetID(sys_id++);
		probe->SetIndex(wep_index++);

		int src_index = probe->GetSourceIndex();
		if (src_index >= 0 && src_index < reactors.size())
			reactors[src_index]->AddClient(probe);

		systems.append(probe);
	}

	for (int i = 0; i < design->computers.size(); i++) {
		Computer* comp = 0;

		if (design->computers[i]->Subtype() == Computer::FLIGHT) {
			flcs = new FlightComputer(*design->computers[i]);

			flcs->SetShip(this);
			flcs->SetMode(flcs_mode);
			flcs->SetVelocityLimit(vlimit);

			if (thruster)
				flcs->SetTransLimit(thruster->TransXLimit(),
					thruster->TransYLimit(),
					thruster->TransZLimit());
			else
				flcs->SetTransLimit(design->trans_x,
					design->trans_y,
					design->trans_z);

			comp = flcs;
		}
		else {
			comp = new Computer(*design->computers[i]);
		}

		comp->SetShip(this);
		comp->SetID(sys_id++);
		int src_index = comp->GetSourceIndex();
		if (src_index >= 0 && src_index < reactors.size())
			reactors[src_index]->AddClient(comp);

		computers.append(comp);
		systems.append(comp);
	}

	// NOTE: Original code used Point(0,0,0). Conversion to FVector requires Instruction ctor support.
	// Keeping original callsite signature; update Instruction to accept FVector during the broader migration.
	radio_orders = new Instruction("", Point(0, 0, 0));

	// Load Detail Set:
	for (int i = 0; i < DetailSet::MAX_DETAIL; i++) {
		if (design->models[i].size() > 0) {
			Solid* solid = new ShipSolid(this);
			solid->UseModel(design->models[i].at(0));
			solid->CreateShadows(1);

			Point* offset = 0;
			Point* spin = 0;

			if (design->offsets[i].size() > 0)
				offset = new Point(*design->offsets[i].at(0));

			if (design->spin_rates.size() > 0)
				spin = new Point(*design->spin_rates.at(0));

			detail_level = detail.DefineLevel(design->feature_size[i], solid, offset, spin);
		}

		if (design->models[i].size() > 1) {
			for (int n = 1; n < design->models[i].size(); n++) {
				Solid* solid = new ShipSolid(this); //Solid;
				solid->UseModel(design->models[i].at(n));
				solid->CreateShadows(1);

				Point* offset = 0;
				Point* spin = 0;

				if (design->offsets[i].size() > n)
					offset = new Point(*design->offsets[i].at(n));

				if (design->spin_rates.size() > n)
					spin = new Point(*design->spin_rates.at(n));

				detail.AddToLevel(detail_level, solid, offset, spin);
			}
		}
	}

	// start with lowest available detail:
	detail_level = 0; // this is highest -> detail.NumLevels()-1);
	rep = detail.GetRep(detail_level);

	if (design->cockpit_model) {
		cockpit = new Solid;
		cockpit->UseModel(design->cockpit_model);
		cockpit->SetForeground(true);
	}

	if (design->main_drive >= 0 && design->main_drive < drives.size())
		main_drive = drives[design->main_drive];

	// only use light from drives:
	light = 0;

	// setup starship helm stuff:
	if (IsStarship()) {
		flcs_mode = FLCS_HELM;
	}

	// initialize the AI:
	dir = 0;
	SetControls(0);

	for (int i = 0; i < 4; i++) {
		missile_id[i] = 0;
		missile_eta[i] = 0;
		trigger[i] = false;
	}
}

// +--------------------------------------------------------------------+

Ship::~Ship()
{
	// the loadout can notxbe cleared during Destroy, because it
	// is needed after Destroy to create the re-spawned ship

	delete[] loadout;
	loadout = 0;

	Destroy();
}

// +--------------------------------------------------------------------+

void
Ship::Destroy()
{
	// destroy fighters on deck:
	ListIter<FlightDeck> deck = flight_decks;
	while (++deck) {
		for (int i = 0; i < deck->NumSlots(); i++) {
			Ship* s = deck->GetShip(i);

			if (s && !s->IsDying() && !s->IsDead()) {
				if (sim && sim->IsActive()) {
					s->DeathSpiral();
				}
				else {
					s->transition_type = TRANSITION_DEAD;
					s->Destroy();
				}
			}
		}
	}

	if (element) {
		// mission ending for this ship, evaluate objectives one last time:
		for (int i = 0; i < element->NumObjectives(); i++) {
			Instruction* obj = element->GetObjective(i);

			if (obj->GetStatus() <= INSTRUCTION_STATUS::ACTIVE) {
				obj->Evaluate(this);
			}
		}

		combat_unit = element->GetCombatUnit();
		SetElement(0);
	}

	delete[] track;
	track = 0;

	delete shield;
	shield = 0;
	delete sensor;
	sensor = 0;
	delete navsys;
	navsys = 0;
	delete thruster;
	thruster = 0;
	delete farcaster;
	farcaster = 0;
	delete quantum_drive;
	quantum_drive = 0;
	delete decoy;
	decoy = 0;
	delete probe;
	probe = 0;
	delete gear;
	gear = 0;

	main_drive = 0;
	flcs = 0;

	// repair queue does notxown the systems under repair:
	repair_queue.clear();

	navlights.destroy();
	flight_decks.destroy();
	computers.destroy();
	weapons.destroy();
	drives.destroy();
	reactors.destroy();

	// this is now a list of dangling pointers:
	systems.clear();

	delete hangar;
	hangar = 0;

	// this also destroys the rep:
	detail.Destroy();
	rep = 0;

	GRAPHIC_DESTROY(cockpit);
	GRAPHIC_DESTROY(shieldRep);
	SIMLIGHT_DESTROY(light);

	delete launch_point;
	launch_point = 0;

	delete radio_orders;
	radio_orders = 0;

	delete dir;
	dir = 0;

	delete killer;
	killer = 0;

	// inbound slot is deleted by flight deck:
	inbound = 0;

	life = 0;
	Notify();
}

// +--------------------------------------------------------------------+

void
Ship::Initialize()
{
	ShipDesign::Initialize();
	Thruster::Initialize();
}

// +--------------------------------------------------------------------+

void
Ship::Close()
{
	ShipDesign::Close();
	Thruster::Close();
}

void
Ship::SetupAgility()
{
	const float ROLL_SPEED = (float)(PI * 0.1500);
	const float PITCH_SPEED = (float)(PI * 0.0250);
	const float YAW_SPEED = (float)(PI * 0.0250);

	drag = design->drag;
	dr_drg = design->roll_drag;
	dp_drg = design->pitch_drag;
	dy_drg = design->yaw_drag;

	if (IsDying()) {
		drag = 0.0f;
		dr_drg *= 0.25f;
		dp_drg *= 0.25f;
		dy_drg *= 0.25f;
	}

	if (flight_model > 0) {
		drag = design->arcade_drag;
		thrust *= 10.0f;
	}

	float yaw_air_factor = 1.0f;

	if (IsAirborne()) {
		bool grounded = AltitudeAGL() < Radius() / 2;

		if (flight_model > 0) {
			drag *= 2.0f;

			if (gear && gear->GetState() != LandingGear::GEAR_UP)
				drag *= 2.0f;

			if (grounded)
				drag *= 3.0f;
		}

		else {
			if (Class() != CLASSIFICATION::LCA)
				yaw_air_factor = 0.3f;

			double rho = GetDensity();
			double speed = Velocity().Length();

			agility = design->air_factor * rho * speed - wep_resist;

			if (grounded && agility < 0)
				agility = 0;

			else if (!grounded && agility < 0.5 * design->agility)
				agility = 0.5 * design->agility;

			else if (agility > 2 * design->agility)
				agility = 2 * design->agility;

			// undercarriage aerodynamic drag
			if (gear && gear->GetState() != LandingGear::GEAR_UP)
				drag *= 5.0f;

			// wheel rolling friction
			if (grounded)
				drag *= 10.0f;

			// dead engine drag ;-)
			if (thrust < 10)
				drag *= 5.0f;
		}
	}

	else {
		agility = design->agility - wep_resist;

		if (agility < 0.5 * design->agility)
			agility = 0.5 * design->agility;

		if (flight_model == 0)
			drag = 0.0f;
	}

	float rr = (float)(design->roll_rate * PI / 180);
	float pr = (float)(design->pitch_rate * PI / 180);
	float yr = (float)(design->yaw_rate * PI / 180);

	if (rr == 0) rr = (float)agility * ROLL_SPEED;
	if (pr == 0) pr = (float)agility * PITCH_SPEED;
	if (yr == 0) yr = (float)agility * YAW_SPEED * yaw_air_factor;

	SetAngularRates(rr, pr, yr);
}

// +--------------------------------------------------------------------+

void
Ship::SetRegion(SimRegion* rgn)
{
	SimObject::SetRegion(rgn);

	if (IsGroundUnit()) {
		// glue buildings to the terrain:
		FVector Loc = Location();
		Terrain* TerrainObj = region->GetTerrain();

		if (TerrainObj) {
			Loc.Y = TerrainObj->Height(Loc.X, Loc.Z);
			MoveTo(Loc);
		}
	}

	else if (IsAirborne()) {
		Orbital* Primary = GetRegion()->GetOrbitalRegion()->Primary();

		const double M0 = Primary->Mass();
		const double R = Primary->Radius();

		SetGravity((float)(GRAV * M0 / (R * R)));
		SetBaseDensity(1.0f);
	}

	else {
		SetGravity(0.0f);
		SetBaseDensity(0.0f);

		if (IsStarship())
			flcs_mode = FLCS_HELM;
		else
			flcs_mode = FLCS_AUTO;
	}
}

// +--------------------------------------------------------------------+

int
Ship::GetTextureList(List<UTexture2D*>& Textures)
{
	Textures.clear();

	for (int d = 0; d < detail.NumLevels(); d++) {
		for (int i = 0; i < detail.NumModels(d); i++) {
			Graphic* g = detail.GetRep(d, i);

			if (g && g->IsSolid()) {
				Solid* solid = (Solid*)g;
				SimModel* model = solid->GetModel();

				if (model) {
					for (int n = 0; n < model->NumMaterials(); n++) {
						// NOTE: Starshatter model/material system needs a UE bridge layer.
						// This is intentionally left as a stub until Material/Model exposes UTexture2D*.
						// Textures.append(model->textures[n]);
					}
				}
			}
		}
	}

	return Textures.size();
}

// +--------------------------------------------------------------------+

void
Ship::Activate(SimScene& Scene)
{
	SimObject::Activate(Scene);

	for (int ModelIndex = 0; ModelIndex < detail.NumModels(detail_level); ModelIndex++) {
		Graphic* ModelRep = detail.GetRep(detail_level, ModelIndex);
		Scene.AddGraphic(ModelRep);
	}

	for (int DeckIndex = 0; DeckIndex < flight_decks.size(); DeckIndex++) {
		Scene.AddLight(flight_decks[DeckIndex]->GetLight());
	}

	if (shieldRep)
		Scene.AddGraphic(shieldRep);

	if (cockpit) {
		Scene.AddForeground(cockpit);
		cockpit->Hide();
	}

	Drive* DriveComp = GetDrive();
	if (DriveComp) {
		for (int EngineIndex = 0; EngineIndex < DriveComp->NumEngines(); EngineIndex++) {
			Graphic* Flare = DriveComp->GetFlare(EngineIndex);
			if (Flare)
				Scene.AddGraphic(Flare);

			Graphic* Trail = DriveComp->GetTrail(EngineIndex);
			if (Trail)
				Scene.AddGraphic(Trail);
		}
	}

	Thruster* ThrusterComp = GetThruster();
	if (ThrusterComp) {
		for (int ThrusterIndex = 0; ThrusterIndex < ThrusterComp->NumThrusters(); ThrusterIndex++) {
			Graphic* Flare = ThrusterComp->Flare(ThrusterIndex);
			if (Flare)
				Scene.AddGraphic(Flare);

			Graphic* Trail = ThrusterComp->Trail(ThrusterIndex);
			if (Trail)
				Scene.AddGraphic(Trail);
		}
	}

	for (int LightIndex = 0; LightIndex < navlights.size(); LightIndex++) {
		NavLight* NavLightComp = navlights[LightIndex];
		for (int BeaconIndex = 0; BeaconIndex < NavLightComp->NumBeacons(); BeaconIndex++) {
			Graphic* Beacon = NavLightComp->Beacon(BeaconIndex);
			if (Beacon)
				Scene.AddGraphic(Beacon);
		}
	}

	ListIter<WeaponGroup> GroupIter = weapons;
	while (++GroupIter) {
		ListIter<Weapon> WeaponIter = GroupIter->GetWeapons();
		while (++WeaponIter) {
			Solid* Turret = WeaponIter->GetTurret();
			if (Turret) {
				Scene.AddGraphic(Turret);

				Solid* TurretBase = WeaponIter->GetTurretBase();
				if (TurretBase)
					Scene.AddGraphic(TurretBase);
			}

			if (WeaponIter->IsMissile()) {
				for (int AmmoIndex = 0; AmmoIndex < WeaponIter->Ammo(); AmmoIndex++) {
					Solid* VisibleStore = WeaponIter->GetVisibleStore(AmmoIndex);
					if (VisibleStore)
						Scene.AddGraphic(VisibleStore);
				}
			}
		}
	}

	if (gear && gear->GetState() != LandingGear::GEAR_UP) {
		for (int GearIndex = 0; GearIndex < gear->NumGear(); GearIndex++) {
			Scene.AddGraphic(gear->GetGear(GearIndex));
		}
	}
}

void
Ship::Deactivate(SimScene& Scene)
{
	SimObject::Deactivate(Scene);

	for (int ModelIndex = 0; ModelIndex < detail.NumModels(detail_level); ModelIndex++) {
		Graphic* ModelRep = detail.GetRep(detail_level, ModelIndex);
		Scene.DelGraphic(ModelRep);
	}

	for (int DeckIndex = 0; DeckIndex < flight_decks.size(); DeckIndex++) {
		Scene.DelLight(flight_decks[DeckIndex]->GetLight());
	}

	if (shieldRep)
		Scene.DelGraphic(shieldRep);

	if (cockpit)
		Scene.DelForeground(cockpit);

	Drive* DriveComp = GetDrive();
	if (DriveComp) {
		for (int EngineIndex = 0; EngineIndex < DriveComp->NumEngines(); EngineIndex++) {
			Graphic* Flare = DriveComp->GetFlare(EngineIndex);
			if (Flare)
				Scene.DelGraphic(Flare);

			Graphic* Trail = DriveComp->GetTrail(EngineIndex);
			if (Trail)
				Scene.DelGraphic(Trail);
		}
	}

	Thruster* ThrusterComp = GetThruster();
	if (ThrusterComp) {
		for (int ThrusterIndex = 0; ThrusterIndex < ThrusterComp->NumThrusters(); ThrusterIndex++) {
			Graphic* Flare = ThrusterComp->Flare(ThrusterIndex);
			if (Flare)
				Scene.DelGraphic(Flare);

			Graphic* Trail = ThrusterComp->Trail(ThrusterIndex);
			if (Trail)
				Scene.DelGraphic(Trail);
		}
	}

	for (int LightIndex = 0; LightIndex < navlights.size(); LightIndex++) {
		NavLight* NavLightComp = navlights[LightIndex];
		for (int BeaconIndex = 0; BeaconIndex < NavLightComp->NumBeacons(); BeaconIndex++) {
			Graphic* Beacon = NavLightComp->Beacon(BeaconIndex);
			if (Beacon)
				Scene.DelGraphic(Beacon);
		}
	}

	ListIter<WeaponGroup> GroupIter = weapons;
	while (++GroupIter) {
		ListIter<Weapon> WeaponIter = GroupIter->GetWeapons();
		while (++WeaponIter) {
			Solid* Turret = WeaponIter->GetTurret();
			if (Turret) {
				Scene.DelGraphic(Turret);

				Solid* TurretBase = WeaponIter->GetTurretBase();
				if (TurretBase)
					Scene.DelGraphic(TurretBase);
			}

			if (WeaponIter->IsMissile()) {
				for (int AmmoIndex = 0; AmmoIndex < WeaponIter->Ammo(); AmmoIndex++) {
					Solid* VisibleStore = WeaponIter->GetVisibleStore(AmmoIndex);
					if (VisibleStore)
						Scene.DelGraphic(VisibleStore);
				}
			}
		}
	}

	if (gear) {
		for (int GearIndex = 0; GearIndex < gear->NumGear(); GearIndex++) {
			Scene.DelGraphic(gear->GetGear(GearIndex));
		}
	}
}

void
Ship::MatchOrientation(const Ship& OtherShip)
{
	const FVector SavedPos = cam.Pos();

	cam.Clone(OtherShip.cam);
	cam.MoveTo(SavedPos);

	const FMatrix NewOrient = ToFMatrix(cam.Orientation());

	if (rep)
		rep->SetOrientation(NewOrient);

	if (cockpit)
		cockpit->SetOrientation(NewOrient);
}

// +--------------------------------------------------------------------+

void
Ship::ClearTrack()
{
	const int DEFAULT_TRACK_LENGTH = 20; // 10 seconds

	if (!track) {
		track = new  FVector[DEFAULT_TRACK_LENGTH];
	}

	track[0] = Location();
	ntrack = 1;
	track_time = Game::GameTime();
}

void
Ship::UpdateTrack()
{
	const int DEFAULT_TRACK_UPDATE = 500; // milliseconds
	const int DEFAULT_TRACK_LENGTH = 20;  // 10 seconds

	DWORD time = Game::GameTime();

	if (!track) {
		track = new  FVector[DEFAULT_TRACK_LENGTH];
		track[0] = Location();
		ntrack = 1;
		track_time = time;
	}

	else if (time - track_time > DEFAULT_TRACK_UPDATE) {
		if (Location() != track[0]) {
			for (int i = DEFAULT_TRACK_LENGTH - 2; i >= 0; i--)
				track[i + 1] = track[i];

			track[0] = Location();
			if (ntrack < DEFAULT_TRACK_LENGTH) ntrack++;
		}

		track_time = time;
	}
}

FVector
Ship::TrackPoint(int i) const
{
	if (track && i < ntrack)
		return track[i];

	return FVector::ZeroVector;
}

// +--------------------------------------------------------------------+

const char*
Ship::Abbreviation() const
{
	return design->abrv;
}

const char*
Ship::DesignName() const
{
	return design->DisplayName();
}

const char*
Ship::DesignFileName() const
{
	return design->filename;
}

const char*
Ship::GetShipClassName() const
{
	return ShipDesign::ClassName(design->type);
}

const char*
Ship::GetShipClassName(int c)
{
	return ShipDesign::ClassName(c);
}

int
Ship::ClassForName(const char* name)
{
	return ShipDesign::ClassForName(name);
}

CLASSIFICATION
Ship::Class() const
{
	return (CLASSIFICATION)design->type;
}

bool
Ship::IsGroundUnit() const
{
	return (design->type & (int) CLASSIFICATION::GROUND_UNITS) ? true : false;
}

bool
Ship::IsStarship() const
{
	return (design->type & (int)CLASSIFICATION::STARSHIPS) ? true : false;
}

bool
Ship::IsDropship() const
{
	return (design->type & (int)CLASSIFICATION::DROPSHIPS) ? true : false;
}

bool
Ship::IsStatic() const
{
	return design->type >= (int)CLASSIFICATION::STATION;
}

bool
Ship::IsRogue() const
{
	return ff_count >= 50;
}

// +--------------------------------------------------------------------+

bool
Ship::IsHostileTo(const SimObject* o) const
{
	if (o) {
		if (IsRogue())
			return true;

		if (o->Type() == SIM_SHIP) {
			Ship* s = (Ship*)o;

			if (s->IsRogue())
				return true;

			if (GetIFF() == 0) {
				if (s->GetIFF() > 1)
					return true;
			}
			else {
				if (s->GetIFF() > 0 && s->GetIFF() != GetIFF())
					return true;
			}
		}

		else if (o->Type() == SIM_SHOT || o->Type() == SIM_DRONE) {
			SimShot* s = (SimShot*)o;

			if (GetIFF() == 0) {
				if (s->GetIFF() > 1)
					return true;
			}
			else {
				if (s->GetIFF() > 0 && s->GetIFF() != GetIFF())
					return true;
			}
		}
	}

	return false;
}

// +--------------------------------------------------------------------+

double
Ship::RepairSpeed() const
{
	return design->repair_speed;
}

int
Ship::RepairTeams() const
{
	return design->repair_teams;
}

// +--------------------------------------------------------------------+

int
Ship::NumContacts() const
{
	// cast-away const:
	return ((Ship*)this)->ContactList().size();
}

List<SimContact>&
Ship::ContactList()
{
	if (region)
		return region->TrackList(GetIFF());

	static List<SimContact> empty_contact_list;
	return empty_contact_list;
}

SimContact*
Ship::FindContact(SimObject* s) const
{
	if (!s) return 0;

	ListIter<SimContact> c_iter = ((Ship*)this)->ContactList();
	while (++c_iter) {
		SimContact* c = c_iter.value();

		if (c->GetShip() == s)
			return c;

		if (c->GetShot() == s)
			return c;
	}

	return 0;
}

// +--------------------------------------------------------------------+

Ship*
Ship::GetController() const
{
	Ship* controller = 0;

	if (carrier) {
		// are we in same region as carrier?
		if (carrier->GetRegion() == GetRegion()) {
			return carrier;
		}

		// if not, figure out who our control unit is:
		else {
			double distance = 10e6;

			ListIter<Ship> iter = GetRegion()->GetCarriers();
			while (++iter) {
				Ship* test = iter.value();
				if (test->GetIFF() == GetIFF()) {
					const double d = (Location() - test->Location()).Length();
					if (d < distance) {
						controller = test;
						distance = d;
					}
				}
			}
		}
	}

	if (!controller) {
		if (element && element->GetCommander())
			controller = element->GetCommander()->GetShip(1);
	}

	return controller;
}

int
Ship::NumInbound() const
{
	int result = 0;

	for (int i = 0; i < flight_decks.size(); i++) {
		result += flight_decks[i]->GetRecoveryQueue().size();
	}

	return result;
}

int
Ship::NumFlightDecks() const
{
	return flight_decks.size();
}

FlightDeck*
Ship::GetFlightDeck(int i) const
{
	if (i >= 0 && i < flight_decks.size())
		return flight_decks[i];

	return 0;
}

// +--------------------------------------------------------------------+

void
Ship::SetFlightPhase(OP_MODE phase)
{
	if (phase == ACTIVE && !launch_time) {
		launch_time = Game::GameTime() + 1;
		dock = 0;

		if (element)
			element->SetLaunchTime(launch_time);
	}

	flight_phase = phase;

	if (flight_phase == ACTIVE)
		dock = 0;
}

void
Ship::SetCarrier(Ship* c, FlightDeck* d)
{
	carrier = c;
	dock = d;

	if (carrier)
		Observe(carrier);
}

void
Ship::SetInbound(InboundSlot* s)
{
	inbound = s;

	if (inbound && flight_phase == ACTIVE) {
		flight_phase = APPROACH;

		SetCarrier((Ship*)inbound->GetDeck()->GetCarrier(), inbound->GetDeck());

		HUDView* hud = HUDView::GetInstance();

		if (hud && hud->GetShip() == this)
			hud->SetHUDMode(EHUDMode::ILS);
	}
}

void
Ship::Stow()
{
	if (carrier && carrier->GetHangar())
		carrier->GetHangar()->Stow(this);
}

bool
Ship::IsGearDown()
{
	if (gear && gear->GetState() == LandingGear::GEAR_DOWN)
		return true;

	return false;
}

void
Ship::LowerGear()
{
	if (gear && gear->GetState() != LandingGear::GEAR_DOWN) {
		gear->SetState(LandingGear::GEAR_LOWER);
		SimScene* scene = 0;

		if (rep)
			scene = rep->GetScene();

		if (scene) {
			for (int i = 0; i < gear->NumGear(); i++) {
				Solid* g = gear->GetGear(i);
				if (g) {
					if (detail_level == 0)
						scene->DelGraphic(g);
					else
						scene->AddGraphic(g);
				}
			}
		}
	}
}

void
Ship::RaiseGear()
{
	if (gear && gear->GetState() != LandingGear::GEAR_UP)
		gear->SetState(LandingGear::GEAR_RAISE);
}

void
Ship::ToggleGear()
{
	if (gear) {
		if (gear->GetState() == LandingGear::GEAR_UP ||
			gear->GetState() == LandingGear::GEAR_RAISE) {
			LowerGear();
		}
		else {
			RaiseGear();
		}
	}
}

void
Ship::ToggleNavlights()
{
	bool enable = false;

	for (int i = 0; i < navlights.size(); i++) {
		if (i == 0)
			enable = !navlights[0]->IsEnabled();

		if (enable)
			navlights[i]->Enable();
		else
			navlights[i]->Disable();
	}
}

// +--------------------------------------------------------------------+

int
Ship::CollidesWith(Physical& o)
{
	// bounding spheres test:
	const FVector DeltaLoc = Location() - o.Location();
	if (DeltaLoc.Length() > radius + o.Radius())
		return 0;

	if (!o.Rep())
		return 1;

	for (int i = 0; i < detail.NumModels(detail_level); i++) {
		Graphic* g = detail.GetRep(detail_level, i);

		if (o.Type() == SimObject::SIM_SHIP) {
			Ship* o_ship = (Ship*)&o;
			const int o_det = o_ship->detail_level;

			for (int j = 0; j < o_ship->detail.NumModels(o_det); j++) {
				Graphic* o_g = o_ship->detail.GetRep(o_det, j);

				if (g->CollidesWith(*o_g))
					return 1;
			}
		}
		else {
			// representation collision test (will do bounding spheres first):
			if (g->CollidesWith(*o.Rep()))
				return 1;
		}
	}

	return 0;
}

// +--------------------------------------------------------------------+

static DWORD ff_warn_time = 0;

int
Ship::HitBy(SimShot* Shot, FVector& Impact)
{
	if (!Shot)
		return HIT_NOTHING;

	if (Shot->Owner() == this || IsNetObserver())
		return HIT_NOTHING;

	if (Shot->IsFlak())
		return HIT_NOTHING;

	if (InTransition())
		return HIT_NOTHING;

	const FVector ShotLoc = Shot->Location();
	FVector Delta = ShotLoc - Location();
	const double DistLen = (double)Delta.Size();

	FVector HullImpact(0, 0, 0);
	int HitType = HIT_NOTHING;
	double DamageScale = 1.0;

	float Scale = design ? design->explosion_scale : 0.0f;
	Weapon* HitWeapon = 0;

	if (!Shot->IsMissile() && !Shot->IsBeam()) {
		if (DistLen > Radius() * 2.0)
			return HIT_NOTHING;
	}

	if (Scale <= 0.0f && design)
		Scale = (float)design->scale;

	if (Shot->Owner()) {
		const ShipDesign* OwnerDesign = Shot->Owner()->Design();
		if (OwnerDesign && OwnerDesign->scale < Scale)
			Scale = (float)OwnerDesign->scale;
	}

	// MISSILE PROCESSING ------------------------------------------------
	if (Shot->IsMissile() && rep) {
		if (DistLen < rep->Radius()) {
			HullImpact = Impact = ShotLoc;

			HitType = CheckShotIntersection(Shot, Impact, HullImpact, &HitWeapon);

			if (HitType) {
				if (Shot->Damage() > 0) {
					DWORD Flash = Explosion::HULL_FLASH;

					if ((HitType & HIT_SHIELD) != 0)
						Flash = Explosion::SHIELD_FLASH;

					sim->CreateExplosion(Impact, Velocity(), Flash, 0.30f * Scale, Scale, region);
					sim->CreateExplosion(Impact, FVector::ZeroVector, Explosion::SHOT_BLAST, 2.0f, Scale, region);
				}
			}
		}

		if (HitType == HIT_NOTHING && Shot->IsArmed()) {
			SeekerAI* Seeker = (SeekerAI*)Shot->GetDirector();

			// if the missile overshot us, take damage proportional to distance
			const double DamageRadius = Shot->Design()->lethal_radius;
			if (DistLen < (DamageRadius + Radius())) {
				if (Seeker && Seeker->Overshot()) {
					DamageScale = 1.0 - (DistLen / (DamageRadius + Radius()));

					if (DamageScale > 1.0)
						DamageScale = 1.0;
					if (DamageScale < 0.0)
						DamageScale = 0.0;

					if (ShieldStrength() > 5) {
						HullImpact = Impact = ShotLoc;

						if (Shot->Damage() > 0) {
							if (shieldRep)
								shieldRep->Hit(Impact, Shot, Shot->Damage() * DamageScale);

							sim->CreateExplosion(Impact, Velocity(), Explosion::SHIELD_FLASH, 0.20f * Scale, Scale, region);
							sim->CreateExplosion(Impact, FVector::ZeroVector, Explosion::SHOT_BLAST, 20.0f * Scale, Scale, region);
						}

						HitType = HIT_BOTH;
					}
					else {
						HullImpact = Impact = ShotLoc;

						if (Shot->Damage() > 0) {
							sim->CreateExplosion(Impact, Velocity(), Explosion::HULL_FLASH, 0.30f * Scale, Scale, region);
							sim->CreateExplosion(Impact, FVector::ZeroVector, Explosion::SHOT_BLAST, 20.0f * Scale, Scale, region);
						}

						HitType = HIT_HULL;
					}
				}
			}
		}
	}

	// ENERGY WEP PROCESSING ---------------------------------------------
	else {
		HitType = CheckShotIntersection(Shot, Impact, HullImpact, &HitWeapon);

		// impact:
		if (HitType) {

			if (HitType & HIT_SHIELD) {
				if (shieldRep)
					shieldRep->Hit(Impact, Shot, Shot->Damage());

				sim->CreateExplosion(Impact, Velocity(), Explosion::SHIELD_FLASH, 0.20f * Scale, Scale, region);
			}
			else {
				if (Shot->IsBeam())
					sim->CreateExplosion(Impact, Velocity(), Explosion::BEAM_FLASH, 0.30f * Scale, Scale, region);
				else
					sim->CreateExplosion(Impact, Velocity(), Explosion::HULL_FLASH, 0.30f * Scale, Scale, region);

				if (IsStarship()) {
					FVector BurstVel = HullImpact - Location();
					BurstVel.Normalize();
					BurstVel *= Radius() * 0.5f;
					BurstVel += Velocity();

					sim->CreateExplosion(HullImpact, BurstVel, Explosion::HULL_BURST, 0.50f * Scale, Scale, region, this);
				}
			}
		}
	}

	// DAMAGE RESOLUTION -------------------------------------------------
	if (HitType != HIT_NOTHING && Shot->IsArmed()) {

		double EffectiveDamage = Shot->Damage() * DamageScale;

		// FRIENDLY FIRE --------------------------------------------------
		if (Shot->Owner()) {
			Ship* OwnerShip = (Ship*)Shot->Owner();

			if (!IsRogue() &&
				OwnerShip &&
				OwnerShip->GetIFF() == GetIFF() &&
				OwnerShip->GetDirector() &&
				OwnerShip->GetDirector()->Type() < 1000) {

				const bool WasRogue = OwnerShip->IsRogue();

				// only count beam hits once
				if (Shot->Damage() && !Shot->HitTarget() && GetFriendlyFireLevel() > 0) {
					int Penalty = 1;

					if (Shot->IsBeam())       Penalty = 5;
					else if (Shot->IsDrone()) Penalty = 7;

					if (OwnerShip->GetTarget() == this)
						Penalty *= 3;

					OwnerShip->IncFriendlyFire(Penalty);
				}

				EffectiveDamage *= GetFriendlyFireLevel();

				if (Class() > CLASSIFICATION::DRONE && OwnerShip->Class() > CLASSIFICATION::DRONE) {
					if (OwnerShip->IsRogue() && !WasRogue) {
						RadioMessage* Warn = new RadioMessage(OwnerShip, this, RadioMessageAction::DECLARE_ROGUE);
						RadioTraffic::Transmit(Warn);
					}
					else if (!OwnerShip->IsRogue() && (Game::GameTime() - ff_warn_time) > 5000) {
						ff_warn_time = Game::GameTime();

						RadioMessage* Warn = 0;
						if (OwnerShip->GetTarget() == this)
							Warn = new RadioMessage(OwnerShip, this, RadioMessageAction::WARN_TARGETED);
						else
							Warn = new RadioMessage(OwnerShip, this, RadioMessageAction::WARN_ACCIDENT);

						RadioTraffic::Transmit(Warn);
					}
				}
			}
		}

		if (EffectiveDamage > 0.0) {
			if (!Shot->IsBeam() && Shot->Design()->damage_type == WeaponDesign::DMG_NORMAL)
				ApplyTorque(Shot->Velocity() * (float)EffectiveDamage * 1e-6f);

			InflictDamage(EffectiveDamage, Shot, HitType, HullImpact);
		}
	}

	return HitType;
}


static bool CheckRaySphereIntersection(const FVector& loc, double radius, const FVector& Q, const FVector& w, double len)
{
	const FVector d0 = loc - Q;
	const FVector d1 = FVector::CrossProduct(d0, w);
	const double dlen = d1.Length();         // distance of point from line

	if (dlen > radius)                       // clean miss
		return false;                        // (no impact)

	// possible collision course...
	// find the point on the ray that is closest
	// to the sphere's location:
	const FVector closest = Q + w * (float)(d0 | w);

	// find the leading edge, and it's distance from the location:
	const FVector leading_edge = Q + w * (float)len;
	const FVector leading_delta = leading_edge - loc;
	const double  leading_dist = leading_delta.Length();

	// if the leading edge is notxwithin the sphere,
	if (leading_dist > radius) {
		// check to see if the closest point is between the
		// ray's endpoints:
		const FVector delta1 = closest - Q;
		const FVector delta2 = leading_edge - Q; // this is w*len

		// if the closest point is notxbetween the leading edge
		// and the origin, this ray does notxintersect:
		if ((delta1 | delta2) < 0.0f || delta1.Length() > len) {
			return false;
		}
	}

	return true;
}

int
Ship::CheckShotIntersection(SimShot* shot, FVector& ipt, FVector& hpt, Weapon** wep)
{
	int      hit_type = HIT_NOTHING;
	const FVector shot_loc = shot->Location();
	const FVector shot_org = shot->Origin();
	FVector shot_vpn = shot_loc - shot_org;
	double   shot_len = shot_vpn.Normalize();
	double   blow_len = shot_len;
	bool     hit_hull = false;
	bool     easy = false;

	if (shot_len <= 0)
		return hit_type;

	if (shot_len < 1000)
		shot_len = 1000;

	FVector hull_impact;
	FVector shield_impact;
	FVector turret_impact;
	FVector closest;
	double   d0 = 1e9;
	double   d1 = 1e9;
	double   ds = 1e9;

	if (dir && dir->Type() == SteerAI::FIGHTER) {
		ShipAI* shipAI = (ShipAI*)dir;
		easy = shipAI->GetAILevel() < 2;
	}

	if (shieldRep && ShieldStrength() > 5) {
		if (shieldRep->CheckRayIntersection(shot_org, shot_vpn, shot_len, shield_impact)) {
			hit_type = HIT_SHIELD;
			closest = shield_impact;
			d0 = (closest - shot_org).Length();
			ds = d0;

			ipt = shield_impact;
		}
	}

	if (shieldRep && hit_type == HIT_SHIELD && !shot->IsBeam())
		blow_len = shieldRep->Radius() * 2;

	for (int i = 0; i < detail.NumModels(detail_level) && !hit_hull; i++) {
		Solid* s = (Solid*)detail.GetRep(detail_level, i);
		if (s) {
			if (easy) {
				hit_hull = CheckRaySphereIntersection(s->Location(), s->Radius(), shot_org, shot_vpn, shot_len);
			}
			else {
				hit_hull = s->CheckRayIntersection(shot_org, shot_vpn, blow_len, hull_impact) ? true : false;
			}
		}
	}

	if (hit_hull) {
		if (ShieldStrength() > 5 && !shieldRep)
			hit_type = HIT_SHIELD;

		hit_type = hit_type | HIT_HULL;
		hpt = hull_impact;

		d1 = (hull_impact - shot_org).Length();

		if (d1 < d0) {
			closest = hull_impact;
			d0 = d1;
		}
	}

	if (IsStarship() || IsStatic()) {
		ListIter<WeaponGroup> g_iter = GetWeapons();
		while (++g_iter) {
			WeaponGroup* g = g_iter.value();

			if (g->GetDesign() && g->GetDesign()->turret_model) {
				const double tsize = g->GetDesign()->turret_model->GetRadius();

				ListIter<Weapon> w_iter = g->GetWeapons();
				while (++w_iter) {
					Weapon* w = w_iter.value();

					const FVector tloc = w->GetTurret()->Location();

					if (CheckRaySphereIntersection(tloc, tsize, shot_org, shot_vpn, shot_len)) {
						const FVector delta = tloc - shot_org;
						d1 = delta.Length();

						if (d1 < d0) {
							if (wep) *wep = w;
							hit_type = hit_type | HIT_TURRET;
							turret_impact = tloc;

							d0 = d1;

							closest = turret_impact;
							hull_impact = turret_impact;
							hpt = turret_impact;

							if (d1 < ds)
								ipt = turret_impact;
						}
					}
				}
			}
		}
	}

	// trim beam shots to closest impact point:
	if (hit_type && shot->IsBeam()) {
		shot->SetBeamPoints(shot_org, closest);
	}

	return hit_type;
}

// +--------------------------------------------------------------------+

void
Ship::InflictNetDamage(double damage, SimShot* shot)
{
	if (damage > 0) {
		Physical::InflictDamage(damage, 0);

		// shake by percentage of maximum damage
		const double newshake = 50 * damage / design->integrity;
		const double MAX_SHAKE = 7;

		if (shake < MAX_SHAKE)  shake += (float)newshake;
		if (shake > MAX_SHAKE)  shake = (float)MAX_SHAKE;
	}
}

void
Ship::InflictNetSystemDamage(SimSystem* system, double damage, BYTE dmg_type)
{
	if (system && damage > 0 && !IsNetObserver()) {
		const bool dmg_normal = dmg_type == WeaponDesign::DMG_NORMAL;
		const bool dmg_emp = dmg_type == WeaponDesign::DMG_EMP;

		const double sys_damage = damage;
		const double avail = system->Availability();

		if (dmg_normal || (system->IsPowerCritical() && dmg_emp)) {
			system->ApplyDamage(sys_damage);
			master_caution = true;

			if (system->GetExplosionType() && (avail - system->Availability()) >= 50) {
				float scale = design->explosion_scale;
				if (scale <= 0)
					scale = design->scale;

				sim->CreateExplosion(system->MountLocation(),
					Velocity() * 0.7f,
					system->GetExplosionType(),
					0.2f * scale,
					scale,
					region,
					this,
					system);
			}
		}
	}
}

void
Ship::SetNetSystemStatus(SimSystem* system, SYSTEM_STATUS status, int power, int reactor, double avail)
{
	if (system && !IsNetObserver()) {
		if (system->GetPowerLevel() != power)
			system->SetPowerLevel(power);

		if (system->GetSourceIndex() != reactor) {
			SimSystem* s = GetSystem(reactor);

			if (s && s->GetType() == SYSTEM_CATEGORY::POWER_SOURCE) {
				PowerSource* reac = (PowerSource*)s;
				reac->AddClient(system);
			}
		}

		if (system->GetStatus() != status) {
			if (status == SYSTEM_STATUS::MAINT) {
				ListIter<SimComponent> comp = system->GetComponents();
				while (++comp) {
					SimComponent* c = comp.value();

					if (c->GetStatus() < SYSTEM_STATUS::NOMINAL && c->Availability() < 75) {
						if (c->SpareCount() &&
							c->ReplaceTime() <= 300 &&
							(c->Availability() < 50 ||
								c->ReplaceTime() < c->RepairTime())) {

							c->Replace();
						}

						else if (c->Availability() >= 50 || c->NumJerried() < 5) {
							c->Repair();
						}
					}
				}

				RepairSystem(system);
			}
		}

		if (system->Availability() < avail) {
			system->SetNetAvail(avail);
		}
		else {
			system->SetNetAvail(-1);
		}
	}
}

// +----------------------------------------------------------------------+

static bool IsWeaponBlockedFriendly(Weapon* w, const SimObject* test)
{
	if (w && test && w->GetTarget()) {
		const FVector tgt = w->GetTarget()->Location();
		const FVector obj = test->Location();
		const FVector wep = w->MountLocation();

		FVector dir = tgt - wep;
		const double d = dir.Normalize();

		FVector rho = obj - wep;
		const double r = rho.Normalize();

		// if target is much closer than obstacle,
		// don't worry about friendly fire...
		if (d < 1.5 * r)
			return false;

		const FVector dst = dir * (float)r + wep;
		const double err = (obj - dst).Length();

		if (err < test->Radius() * 1.5)
			return true;
	}

	return false;
}

void
Ship::CheckFriendlyFire()
{
	// if no weapons, there is no worry about friendly fire...
	if (weapons.size() < 1)
		return;

	// only check once each second
	if (Game::GameTime() - friendly_fire_time < 1000)
		return;

	List<Weapon> w_list;
	int i = 0;
	int j = 0;

	// clear the FF blocked flag on all weapons
	for (i = 0; i < weapons.size(); i++) {
		WeaponGroup* g = weapons[i];

		for (j = 0; j < g->NumWeapons(); j++) {
			Weapon* w = g->GetWeapon(j);
			w_list.append(w);
			w->SetBlockedFriendly(false);
		}
	}

	// for each friendly ship within some kind of weapons range,
	ListIter<SimContact> c_iter = ContactList();
	while (++c_iter) {
		SimContact* c = c_iter.value();
		Ship* cship = c->GetShip();
		SimShot* cshot = c->GetShot();

		if (cship && cship != this && (cship->GetIFF() == 0 || cship->GetIFF() == GetIFF())) {
			const double range = (cship->Location() - Location()).Length();

			if (range > 100e3)
				continue;

			// check each unblocked weapon to see if it is blocked by that ship
			ListIter<Weapon> iter = w_list;
			while (++iter) {
				Weapon* w = iter.value();

				if (!w->IsBlockedFriendly())
					w->SetBlockedFriendly(IsWeaponBlockedFriendly(w, cship));
			}
		}

		else if (cshot && cshot->GetIFF() == GetIFF()) {
			const double range = (cshot->Location() - Location()).Length();

			if (range > 30e3)
				continue;

			// check each unblocked weapon to see if it is blocked by that shot
			ListIter<Weapon> iter = w_list;
			while (++iter) {
				Weapon* w = iter.value();

				if (!w->IsBlockedFriendly())
					w->SetBlockedFriendly(IsWeaponBlockedFriendly(w, cshot));
			}
		}
	}

	friendly_fire_time = Game::GameTime() + static_cast<uint32>(FMath::RandRange(0, 500));
}

// +----------------------------------------------------------------------+

Ship*
Ship::GetLeader() const
{
	if (element)
		return element->GetShip(1);

	return (Ship*)this;
}

int
Ship::GetElementIndex() const
{
	if (element)
		return element->FindIndex(this);

	return 0;
}

int
Ship::GetOrigElementIndex() const
{
	return orig_elem_index;
}

void
Ship::SetElement(SimElement* e)
{
	element = e;

	if (element) {
		combat_unit = element->GetCombatUnit();

		if (combat_unit) {
			integrity = (float)(design->integrity - combat_unit->GetSustainedDamage());
		}

		orig_elem_index = element->FindIndex(this);
	}
}

void
Ship::SetLaunchPoint(Instruction* pt)
{
	if (pt && !launch_point)
		launch_point = pt;
}

void
Ship::AddNavPoint(Instruction* pt, Instruction* after)
{
	if (GetElementIndex() == 1)
		element->AddNavPoint(pt, after);
}

void
Ship::DelNavPoint(Instruction* pt)
{
	if (GetElementIndex() == 1)
		element->DelNavPoint(pt);
}

void
Ship::ClearFlightPlan()
{
	if (GetElementIndex() == 1)
		element->ClearFlightPlan();
}

// +----------------------------------------------------------------------+

bool
Ship::IsAutoNavEngaged()
{
	if (navsys && navsys->AutoNavEngaged())
		return true;

	return false;
}

void
Ship::SetAutoNav(bool engage)
{
	if (navsys) {
		if (navsys->AutoNavEngaged()) {
			if (!engage)
				navsys->DisengageAutoNav();
		}
		else {
			if (engage)
				navsys->EngageAutoNav();
		}

		if (sim)
			SetControls(sim->GetControls());
	}
}

void
Ship::CommandMode()
{
	if (!dir || dir->Type() != ShipManager::DIR_TYPE) {
		const char* msg = "Captain on the bridge";
		RadioVox* vox = new  RadioVox(0, "1", msg);

		if (vox) {
			vox->AddPhrase(msg);

			if (!vox->Start()) {
				RadioView::Message(RadioTraffic::TranslateVox(msg));
				delete vox;
			}
		}

		SetControls(sim->GetControls());
	}

	else {
		const char* msg = "Exec, you have the conn";
		RadioVox* vox = new  RadioVox(0, "1", msg);

		if (vox) {
			vox->AddPhrase(msg);

			if (!vox->Start()) {
				RadioView::Message(RadioTraffic::TranslateVox(msg));
				delete vox;
			}
		}

		SetControls(0);
	}
}

// +----------------------------------------------------------------------+

Instruction*
Ship::GetNextNavPoint()
{
	if (launch_point && launch_point->GetStatus() <= INSTRUCTION_STATUS::ACTIVE)
		return launch_point;

	if (element)
		return element->GetNextNavPoint();

	return 0;
}

int
Ship::GetNavIndex(const Instruction* n)
{
	if (element)
		return element->GetNavIndex(n);

	return 0;
}

double
Ship::RangeToNavPoint(const Instruction* NavPoint)
{
	double Distance = 0.0;

	if (NavPoint && NavPoint->Region() && GetRegion()) {
		FVector NavLoc = NavPoint->Region()->GetLocation() + NavPoint->Location();
		NavLoc -= GetRegion()->GetLocation();

		Distance = (NavLoc - Location()).Size();
	}

	return Distance;
}

void
Ship::SetNavptStatus(Instruction* navpt, INSTRUCTION_STATUS status)
{
	if (navpt && navpt->GetStatus() != status) {
		if (status == INSTRUCTION_STATUS::COMPLETE) {
			if (navpt->GetAction() == INSTRUCTION_ACTION::ASSAULT) {
				UE_LOG(LogTemp, Log, TEXT("Completed Assault"));
			}
			else if (navpt->GetAction() == INSTRUCTION_ACTION::STRIKE) {
				UE_LOG(LogTemp, Log, TEXT("Completed Strike"));
			}
		}

		navpt->SetStatus(status);

		if (status == INSTRUCTION_STATUS::COMPLETE)
			sim->ProcessEventTrigger(MissionEvent::TRIGGER_NAVPT, 0, Name(), GetNavIndex(navpt));

		if (element) {
			const int index = element->GetNavIndex(navpt);

			if (index >= 0) {
				//NetUtil::SendNavData(false, element, index - 1, navpt);
			}
		}
	}
}

List<Instruction>&
Ship::GetFlightPlan()
{
	if (element)
		return element->GetFlightPlan();

	static List<Instruction> dummy_flight_plan;
	return dummy_flight_plan;
}

int
Ship::FlightPlanLength()
{
	if (element)
		return element->FlightPlanLength();

	return 0;
}

// +--------------------------------------------------------------------+

void
Ship::SetWard(Ship* s)
{
	if (ward == s)
		return;

	ward = s;

	if (ward)
		Observe(ward);
}

// +--------------------------------------------------------------------+

void
Ship::SetTarget(SimObject* targ, SimSystem* sub, bool from_net)
{
	if (targ && targ->Type() == SimObject::SIM_SHIP) {
		Ship* targ_ship = (Ship*)targ;

		if (targ_ship && targ_ship->IsNetObserver())
			return;
	}

	if (target != targ) {
		// DON'T IGNORE TARGET, BECAUSE IT MAY BE IN THREAT LIST
		target = targ;
		if (target) Observe(target);

		if (sim && target)
			sim->ProcessEventTrigger(MissionEvent::TRIGGER_TARGET, 0, target->Name());
	}

	subtarget = sub;

	ListIter<WeaponGroup> weapon = weapons;
	while (++weapon) {
		if (weapon->GetFiringOrders() != WeaponsOrders::POINT_DEFENSE) {
			weapon->SetTarget(target, subtarget);

			if (sub || !IsStarship())
				weapon->SetSweep(WeaponsSweep::SWEEP_NONE);
			else
				weapon->SetSweep(WeaponsSweep::SWEEP_TIGHT);
		}
	}

	//if (!from_net && NetGame::GetInstance())
	//	NetUtil::SendObjTarget(this);

	// track engagement:
	if (target && target->Type() == SimObject::SIM_SHIP) {
		SimElement* elem = GetElement();
		SimElement* tgt_elem = ((Ship*)target)->GetElement();

		if (elem)
			elem->SetAssignment(tgt_elem);
	}
}

void
Ship::DropTarget()
{
	target = 0;
	subtarget = 0;

	SetTarget(target, subtarget);
}

// +--------------------------------------------------------------------+

void
Ship::CycleSubTarget(int Dir)
{
	if (!target || target->Type() != SimObject::SIM_SHIP)
		return;

	Ship* TargetShip = (Ship*)target;
	if (!TargetShip || TargetShip->IsDropship())
		return;

	SimSystem* SubTarget = 0;

	ListIter<SimSystem> SysIter = TargetShip->GetSystems();

	if (Dir > 0) {
		int Latch = (subtarget == 0);

		while (++SysIter) {
			SimSystem* Sys = SysIter.value();

			if (!Sys)
				continue;

			// computers and sensors are not targetable
			if (Sys->GetType() == SYSTEM_CATEGORY::COMPUTER || Sys->GetType() == SYSTEM_CATEGORY::SENSOR)
				continue;

			if (Sys == subtarget) {
				Latch = 1;
			}
			else if (Latch) {
				SubTarget = Sys;
				break;
			}
		}
	}
	else {
		SimSystem* Prev = 0;

		while (++SysIter) {
			SimSystem* Sys = SysIter.value();

			if (!Sys)
				continue;

			// computers and sensors are not targetable
			if (Sys->GetType() == SYSTEM_CATEGORY::COMPUTER || Sys->GetType() == SYSTEM_CATEGORY::SENSOR)
				continue;

			if (Sys == subtarget) {
				SubTarget = Prev;
				break;
			}

			Prev = Sys;
		}

		if (!subtarget)
			SubTarget = Prev;
	}

	SetTarget(TargetShip, SubTarget);
}

// +--------------------------------------------------------------------+

void
Ship::ExecFrame(double seconds)
{
	FMemory::Memzero(trigger, sizeof(trigger));
	altitude_agl = -1.0e6f;

	if (flight_phase < LAUNCH) {
		DockFrame(seconds);
		return;
	}

	if (flight_phase == LAUNCH ||
		(flight_phase == TAKEOFF && AltitudeAGL() > Radius())) {
		SetFlightPhase(ACTIVE);
	}

	if (transition_time > 0) {
		transition_time -= (float)seconds;

		if (transition_time <= 0) {
			CompleteTransition();
			return;
		}

		if (rep && IsDying() && killer) {
			killer->ExecFrame(seconds);
		}
	}

	// observers do notxrun out of power:
	if (IsNetObserver()) {
		for (int i = 0; i < reactors.size(); i++)
			reactors[i]->SetFuelRange(1e6);
	}

	if (IsStatic()) {
		StatFrame(seconds);
		return;
	}

	CheckFriendlyFire();
	ExecNavFrame(seconds);
	ExecEvalFrame(seconds);

	if (IsAirborne()) {
		// are we trying to make orbit?
		if (Location().Y >= TERRAIN_ALTITUDE_LIMIT)
			MakeOrbit();
	}

	if (!InTransition()) {
		ExecSensors(seconds);
		ExecThrottle(seconds);
	}
	else if (IsDropping() || IsAttaining() || IsSkipping()) {
		throttle = 100;
	}

	if (target && target->Life() == 0) {
		DropTarget();
	}

	ExecPhysics(seconds);

	if (!InTransition()) {
		UpdateTrack();
	}

	// are we docking?
	if (IsDropship()) {
		ListIter<Ship> iter = GetRegion()->GetCarriers();

		while (++iter) {
			Ship* carrier_target = iter.value();

			const double range = (Location() - carrier_target->Location()).Length();
			if (range > carrier_target->Radius() * 1.5)
				continue;

			if (carrier_target->GetIFF() == GetIFF() || carrier_target->GetIFF() == 0) {
				for (int i = 0; i < carrier_target->NumFlightDecks(); i++) {
					if (carrier_target->GetFlightDeck(i)->Recover(this))
						break;
				}
			}
		}
	}

	ExecSystems(seconds);
	ExecMaintFrame(seconds);

	if (flight_decks.size() > 0) {
		Camera* global_cam = CameraManager::GetInstance()->GetCamera();
		FVector global_cam_loc = global_cam->Pos();
		bool disable_shadows = false;

		for (int i = 0; i < flight_decks.size(); i++) {
			if (flight_decks[i]->ContainsPoint(global_cam_loc))
				disable_shadows = true;
		}

		EnableShadows(!disable_shadows);
	}

	if (!FMath::IsFinite(Location().X)) {
		DropTarget();
	}

	if (!IsStatic() && !IsGroundUnit() && GetFlightModel() < 2)
		CalcFlightPath();
}

// +--------------------------------------------------------------------+

void
Ship::LaunchProbe()
{
	if (net_observer_mode)
		return;

	if (sensor_drone) {
		sensor_drone = 0;
	}

	if (probe) {
		sensor_drone = (Drone*)probe->Fire();

		if (sensor_drone)
			Observe(sensor_drone);

		else if (sim->GetPlayerShip() == this)
			UIButton::PlaySound(UIButton::SND_REJECT);
	}
}

void
Ship::SetProbe(Drone* d)
{
	if (sensor_drone != d) {
		sensor_drone = d;

		if (sensor_drone)
			Observe(sensor_drone);
	}
}

void
Ship::ExecSensors(double seconds)
{
	// how visible are we?
	DoEMCON();

	// what can we see?
	if (sensor)
		sensor->ExecFrame(seconds);

	// can we still see our target?
	if (target) {
		int target_found = 0;
		ListIter<SimContact> c_iter = ContactList();
		while (++c_iter) {
			SimContact* c = c_iter.value();

			if (target == c->GetShip() || target == c->GetShot()) {
				target_found = 1;

				const bool vis = c->Visible(this) || c->Threat(this);

				if (!vis && !c->PasLock() && !c->ActLock())
					DropTarget();
			}
		}

		if (!target_found)
			DropTarget();
	}
}

// +--------------------------------------------------------------------+

void
Ship::ExecNavFrame(double Seconds)
{
	bool AutoPilot = false;

	// update director info string:
	SetFLCSMode(flcs_mode);

	if (navsys) {
		navsys->ExecFrame(Seconds);

		if (navsys->AutoNavEngaged()) {
			if (dir && dir->Type() == NavAI::DIR_TYPE) {
				NavAI* NavAIComp = (NavAI*)dir;

				if (NavAIComp->Complete()) {
					navsys->DisengageAutoNav();
					SetControls(sim->GetControls());
				}
				else {
					AutoPilot = true;
				}
			}
		}
	}

	// even if we are not on auto pilot,
	// have we completed the next navpoint?

	Instruction* NavPt = GetNextNavPoint();
	if (NavPt && !AutoPilot) {
		if (NavPt->Region() == GetRegion()) {
			FVector NavLoc = NavPt->Location();

			if (NavPt->Region())
				NavLoc += NavPt->Region()->GetLocation();

			Sim* SimInst = Sim::GetSim();
			if (SimInst && SimInst->GetActiveRegion())
				NavLoc -= SimInst->GetActiveRegion()->GetLocation();

			// distance from self to navpt:
			const double Distance = (NavLoc - Location()).Size();

			if (Distance < 10.0 * Radius())
				SetNavptStatus(NavPt, INSTRUCTION_STATUS::COMPLETE);
		}
	}
}

// +--------------------------------------------------------------------+

void
Ship::ExecEvalFrame(double seconds)
{
	// is it already too late?
	if (life == 0 || integrity < 1) return;

	const DWORD EVAL_FREQUENCY = 1000;   // once every second
	static DWORD last_eval_frame = 0;    // one ship per game frame

	if (element && element->NumObjectives() > 0 &&
		Game::GameTime() - last_eval_time > EVAL_FREQUENCY &&
		last_eval_frame != Game::Frame()) {

		last_eval_time = Game::GameTime();
		last_eval_frame = Game::Frame();

		for (int i = 0; i < element->NumObjectives(); i++) {
			Instruction* obj = element->GetObjective(i);

			if (obj->GetStatus() <= INSTRUCTION_STATUS::ACTIVE) {
				obj->Evaluate(this);
			}
		}
	}
}

// +--------------------------------------------------------------------+

void
Ship::ExecPhysics(double seconds)
{
	if (net_control) {
		net_control->ExecFrame(seconds);
		Thrust(seconds);  // drive flare
	}
	else {
		thrust = (float)Thrust(seconds);
		SetupAgility();

		if (seconds > 0) {
			g_force = 0.0f;
		}

		if (IsAirborne()) {
			const FVector v1 = velocity;
			AeroFrame(seconds);
			const FVector v2 = velocity;
			const FVector dv = (v2 - v1) + FVector(0.0f, g_accel * (float)seconds, 0.0f);

			if (seconds > 0) {
				g_force = (float)(FVector::DotProduct(dv, cam.vup()) / seconds) / 9.8f;
			}
		}

		else if (IsDying() || flight_model < 2) { // standard and relaxed modes
			Physical::ExecFrame(seconds);
		}

		else { // arcade mode
			Physical::ArcadeFrame(seconds);
		}
	}
}

// +--------------------------------------------------------------------+

void
Ship::ExecThrottle(double seconds)
{
	const double spool = 75 * seconds;

	if (throttle < throttle_request) {
		if (throttle_request - throttle < spool)
			throttle = throttle_request;
		else
			throttle += spool;
	}

	else if (throttle > throttle_request) {
		if (throttle - throttle_request < spool)
			throttle = throttle_request;
		else
			throttle -= spool;
	}
}

// +--------------------------------------------------------------------+

void
Ship::ExecSystems(double seconds)
{
	if (!rep)
		return;

	int i = 0;

	ListIter<SimSystem> iter = systems;
	while (++iter) {
		SimSystem* sys = iter.value();

		sys->Orient(this);

		// sensors have already been executed,
		// they can notxbe run twice in a frame!
		if (sys->GetType() != SYSTEM_CATEGORY::SENSOR)
			sys->ExecFrame(seconds);
	}

	// hangars and weapon groups are notxsystems
	// they must be executed separately from above
	if (hangar)
		hangar->ExecFrame(seconds);

	wep_mass = 0.0f;
	wep_resist = 0.0f;

	bool winchester_cycle = false;

	for (i = 0; i < weapons.size(); i++) {
		WeaponGroup* w_group = weapons[i];
		w_group->ExecFrame(seconds);

		if (w_group->GetTrigger() && w_group->GetFiringOrders() == WeaponsOrders::MANUAL) {

			Weapon* gun = w_group->GetSelected();
			SimObject* gun_tgt = gun->GetTarget();

			// if no target has been designated for this
			// weapon, let it guide on the contact closest
			// to its boresight.  this must be done before
			// firing the weapon.
			if (sensor && gun->Guided() && !gun->Design()->beam && !gun_tgt) {
				gun->SetTarget(sensor->AcquirePassiveTargetForMissile(), 0);
			}

			gun->Fire();

			w_group->SetTrigger(false);
			w_group->CycleWeapon();
			w_group->CheckAmmo();

			// was that the last shot from this missile group?
			if (w_group->IsMissile() && w_group->Ammo() < 1) {
				// is this the current secondary weapon group?
				if (weapons[secondary] == w_group) {
					winchester_cycle = true;
				}
			}
		}

		wep_mass += w_group->Mass();
		wep_resist += w_group->Resistance();
	}

	// if we just fired the last shot in the current secondary
	// weapon group, auto cycle to another secondary weapon:
	if (winchester_cycle) {
		const int old_secondary = secondary;

		CycleSecondary();

		// do notxwinchester-cycle to an A2G missile type,
		// or a missile that is also out of ammo,
		// keep going!
		while (secondary != old_secondary) {
			Weapon* missile = GetSecondary();
			if (missile && missile->CanTarget((int)CLASSIFICATION::GROUND_UNITS))
				CycleSecondary();

			else if (weapons[secondary]->Ammo() < 1)
				CycleSecondary();

			else
				break;
		}
	}

	mass = (float)design->mass + wep_mass;

	if (IsDropship())
		agility = (float)design->agility - wep_resist;

	if (shieldRep) {
		Solid* solid = (Solid*)rep;
		shieldRep->MoveTo(solid->Location());
		shieldRep->SetOrientation(solid->Orientation());

		bool bubble = false;
		if (shield)
			bubble = shield->ShieldBubble();

		if (shieldRep->ActiveHits()) {
			shieldRep->Energize(seconds, bubble);
			shieldRep->Show();
		}
		else {
			shieldRep->Hide();
		}
	}

	if (cockpit) {
		Solid* solid = (Solid*)rep;

		const FVector cpos =
			cam.Pos() +
			cam.vrt() * (float)bridge_vec.X +
			cam.vpn() * (float)bridge_vec.Y +
			cam.vup() * (float)bridge_vec.Z;

		cockpit->MoveTo(cpos);
		cockpit->SetOrientation(solid->Orientation());
	}
}

// +--------------------------------------------------------------------+

void
Ship::AeroFrame(double seconds)
{
	const float g_save = g_accel;

	if (Class() == CLASSIFICATION::LCA) {
		lat_thrust = true;
		SetGravity(0.0f);
	}

	if (AltitudeAGL() < Radius()) {
		SetGravity(0.0f);

		// on the ground/runway?
		double bottom = 1e9;
		double tlevel = Location().Y - AltitudeAGL();

		// taking off or landing?
		if (flight_phase < ACTIVE || flight_phase > APPROACH) {
			if (dock)
				tlevel = dock->MountLocation().Y;
		}

		if (tlevel < 0)
			tlevel = 0;

		if (gear)
			bottom = gear->GetTouchDown() - 1;
		else
			bottom = Location().Y - 6;

		if (bottom < tlevel)
			TranslateBy(FVector(0.0f, (float)(bottom - tlevel), 0.0f));
	}

	// MODEL 2: ARCADE
	if (flight_model >= 2) {
		Physical::ArcadeFrame(seconds);
	}

	// MODEL 1: RELAXED
	else if (flight_model == 1) {
		Physical::ExecFrame(seconds);
	}

	// MODEL 0: STANDARD
	else {
		// apply drag-torque (i.e. turn ship into
		// velocity vector to minimize drag):

		FVector vnrm = velocity;
		const double v = vnrm.Size();
		if (v > 0.0)
			vnrm /= (float)v;

		const double pitch_deflection = FVector::DotProduct(vnrm, cam.vup());
		const double yaw_deflection = FVector::DotProduct(vnrm, cam.vrt());

		if (lat_thrust && v < 250) {
			// intentionally blank (original logic)
		}
		else {
			if (v < 250) {
				const double factor = 1.2 + (250 - v) / 100;

				ApplyPitch(pitch_deflection * -factor);
				ApplyYaw(yaw_deflection * factor);

				dp += (float)(dp_acc * seconds);
				dy += (float)(dy_acc * seconds);
			}

			else {
				if (FMath::Abs(pitch_deflection) > stall) {
					ApplyPitch(pitch_deflection * -1.2);
					dp += (float)(dp_acc * seconds);
				}

				ApplyYaw(yaw_deflection * 2);
				dy += (float)(dy_acc * seconds);
			}
		}

		// compute rest of physics:
		Physical::AeroFrame(seconds);
	}

	SetGravity(g_save);
}

// +--------------------------------------------------------------------+

void
Ship::LinearFrame(double seconds)
{
	Physical::LinearFrame(seconds);

	if (!IsAirborne() || Class() != CLASSIFICATION::LCA)
		return;

	// damp lateral movement in atmosphere:

	// side-to-side
	if (!trans_x) {
		FVector transvec = cam.vrt();
		transvec *= (float)(FVector::DotProduct(transvec, velocity) * seconds * 0.5);
		velocity -= transvec;
	}

	// fore-and-aft
	if (!trans_y && FMath::Abs(thrust) < 1.0f) {
		FVector transvec = cam.vpn();
		transvec *= (float)(FVector::DotProduct(transvec, velocity) * seconds * 0.25);
		velocity -= transvec;
	}

	// up-and-down
	if (!trans_z) {
		FVector transvec = cam.vup();
		transvec *= (float)(FVector::DotProduct(transvec, velocity) * seconds * 0.5);
		velocity -= transvec;
	}
}

// +--------------------------------------------------------------------+

void
Ship::DockFrame(double Seconds)
{
	SelectDetail(Seconds);

	if (sim && sim->GetPlayerShip() == this) {
		// Make sure the thruster sound is disabled
		// when the player is on the runway or catapult
		if (thruster) {
			thruster->ExecTrans(0, 0, 0);
		}
	}

	if (rep) {
		// Update the graphic rep and light sources:
		// (This is usually done by the physics class,
		// but when the ship is in dock, we skip the
		// standard physics processing):
		rep->MoveTo(cam.Pos());
		rep->SetOrientation(ToFMatrix(cam.Orientation()));

		if (light)
			light->MoveTo(cam.Pos());

		ListIter<SimSystem> Iter = systems;
		while (++Iter)
			Iter->Orient(this);

		const double Spool = 75.0 * Seconds;

		if (flight_phase == DOCKING) {
			throttle_request = 0;
			throttle = 0;
		}
		else if (throttle < throttle_request) {
			if ((throttle_request - throttle) < Spool)
				throttle = throttle_request;
			else
				throttle += Spool;
		}
		else if (throttle > throttle_request) {
			if ((throttle - throttle_request) < Spool)
				throttle = throttle_request;
			else
				throttle -= Spool;
		}

		// make sure there is power to run the drive:
		for (int ReactorIndex = 0; ReactorIndex < reactors.size(); ReactorIndex++)
			reactors[ReactorIndex]->ExecFrame(Seconds);

		// count up weapon ammo for status mfd:
		for (int WeaponIndex = 0; WeaponIndex < weapons.size(); WeaponIndex++)
			weapons[WeaponIndex]->ExecFrame(Seconds);

		// show drive flare while on catapult:
		if (main_drive) {
			main_drive->SetThrottle(throttle);

			if (throttle > 0)
				main_drive->Thrust(Seconds);  // show drive flare
		}
	}

	if (cockpit && !cockpit->Hidden() && rep) {
		Solid* SolidRep = (Solid*)rep;

		const FVector CockpitPos =
			cam.Pos() +
			cam.vrt() * (float)bridge_vec.X +
			cam.vpn() * (float)bridge_vec.Y +
			cam.vup() * (float)bridge_vec.Z;

		cockpit->MoveTo(CockpitPos);
		cockpit->SetOrientation(SolidRep->Orientation());
	}
}

// +--------------------------------------------------------------------+

void
Ship::StatFrame(double Seconds)
{
	if (flight_phase != ACTIVE) {
		flight_phase = ACTIVE;
		launch_time = Game::GameTime() + 1;

		if (element)
			element->SetLaunchTime(launch_time);
	}

	if (IsGroundUnit()) {
		// glue buildings to the terrain:
		FVector Loc = Location();
		Terrain* TerrainObj = region ? region->GetTerrain() : 0;

		if (TerrainObj) {
			Loc.Y = TerrainObj->Height(Loc.X, Loc.Z);
			MoveTo(Loc);
		}
	}

	if (rep) {
		rep->MoveTo(cam.Pos());
		rep->SetOrientation(ToFMatrix(cam.Orientation()));
	}

	if (light) {
		light->MoveTo(cam.Pos());
	}

	ExecSensors(Seconds);

	if (target && target->Life() == 0) {
		DropTarget();
	}

	if (dir)
		dir->ExecFrame(Seconds);

	SelectDetail(Seconds);

	if (rep) {
		ListIter<SimSystem> Iter = systems;
		while (++Iter)
			Iter->Orient(this);

		for (int ReactorIndex = 0; ReactorIndex < reactors.size(); ReactorIndex++)
			reactors[ReactorIndex]->ExecFrame(Seconds);

		for (int NavLightIndex = 0; NavLightIndex < navlights.size(); NavLightIndex++)
			navlights[NavLightIndex]->ExecFrame(Seconds);

		for (int WeaponIndex = 0; WeaponIndex < weapons.size(); WeaponIndex++)
			weapons[WeaponIndex]->ExecFrame(Seconds);

		if (farcaster) {
			farcaster->ExecFrame(Seconds);

			if (navlights.size() == 2) {
				if (farcaster->Charge() > 99) {
					navlights[0]->Enable();
					navlights[1]->Disable();
				}
				else {
					navlights[0]->Disable();
					navlights[1]->Enable();
				}
			}
		}

		if (shield)
			shield->ExecFrame(Seconds);

		if (hangar)
			hangar->ExecFrame(Seconds);

		if (flight_decks.size() > 0) {
			Camera* GlobalCam = CameraManager::GetInstance()->GetCamera();
			const FVector GlobalCamLoc = GlobalCam ? GlobalCam->Pos() : FVector::ZeroVector;

			bool bDisableShadows = false;

			for (int DeckIndex = 0; DeckIndex < flight_decks.size(); DeckIndex++) {
				if (!flight_decks[DeckIndex])
					continue;

				flight_decks[DeckIndex]->ExecFrame(Seconds);

				if (GlobalCam && flight_decks[DeckIndex]->ContainsPoint(GlobalCamLoc))
					bDisableShadows = true;
			}

			EnableShadows(!bDisableShadows);
		}
	}

	if (shieldRep && rep) {
		Solid* SolidRep = (Solid*)rep;

		shieldRep->MoveTo(SolidRep->Location());
		shieldRep->SetOrientation(SolidRep->Orientation());

		bool bBubble = false;
		if (shield)
			bBubble = shield->ShieldBubble();

		if (shieldRep->ActiveHits()) {
			shieldRep->Energize(Seconds, bBubble);
			shieldRep->Show();
		}
		else {
			shieldRep->Hide();
		}
	}

	if (!FMath::IsFinite(Location().X)) {
		DropTarget();
	}
}

// +--------------------------------------------------------------------+

Graphic*
Ship::Cockpit() const
{
	return cockpit;
}

void
Ship::ShowCockpit()
{
	if (cockpit) {
		cockpit->Show();

		ListIter<WeaponGroup> g = weapons;
		while (++g) {
			ListIter<Weapon> w = g->GetWeapons();
			while (++w) {
				Solid* turret = w->GetTurret();
				if (turret) {
					turret->Show();

					Solid* turret_base = w->GetTurretBase();
					if (turret_base)
						turret_base->Show();
				}

				if (w->IsMissile()) {
					for (int i = 0; i < w->Ammo(); i++) {
						Solid* store = w->GetVisibleStore(i);
						if (store) {
							store->Show();
						}
					}
				}
			}
		}
	}
}

void
Ship::HideCockpit()
{
	if (cockpit)
		cockpit->Hide();
}

// +--------------------------------------------------------------------+

void
Ship::SelectDetail(double Seconds)
{
	detail.ExecFrame(Seconds);
	detail.SetLocation(GetRegion(), Location());

	const int NewLevel = detail.GetDetailLevel();

	if (detail_level != NewLevel) {
		SimScene* Scene = 0;

		// remove current rep from scene (if necessary):
		for (int ModelIndex = 0; ModelIndex < detail.NumModels(detail_level); ModelIndex++) {
			Graphic* Gfx = detail.GetRep(detail_level, ModelIndex);
			if (Gfx) {
				Scene = Gfx->GetScene();
				if (Scene)
					Scene->DelGraphic(Gfx);
			}
		}

		// switch to new rep:
		detail_level = NewLevel;
		rep = detail.GetRep(detail_level);

		// add new rep to scene (if necessary):
		if (Scene) {
			for (int ModelIndex = 0; ModelIndex < detail.NumModels(detail_level); ModelIndex++) {
				Graphic* Gfx = detail.GetRep(detail_level, ModelIndex);
				if (!Gfx)
					continue;

				const FVector Spin = detail.GetSpin(detail_level, ModelIndex);

				Matrix StarOrient = cam.Orientation();
				StarOrient.Pitch(Spin.X);
				StarOrient.Yaw(Spin.Z);
				StarOrient.Roll(Spin.Y);

				Scene->AddGraphic(Gfx);

				Gfx->MoveTo(cam.Pos() + detail.GetOffset(detail_level, ModelIndex));
				Gfx->SetOrientation(ToFMatrix(StarOrient));
			}

			// show/hide external stores and landing gear...
			if (detail.NumLevels() > 0) {
				if (gear && (gear->GetState() != LandingGear::GEAR_UP)) {
					for (int GearIndex = 0; GearIndex < gear->NumGear(); GearIndex++) {
						Solid* GearGfx = gear->GetGear(GearIndex);
						if (GearGfx) {
							if (detail_level == 0)
								Scene->DelGraphic(GearGfx);
							else
								Scene->AddGraphic(GearGfx);
						}
					}
				}

				ListIter<WeaponGroup> GroupIter = weapons;
				while (++GroupIter) {
					ListIter<Weapon> WeaponIter = GroupIter->GetWeapons();
					while (++WeaponIter) {
						Solid* Turret = WeaponIter->GetTurret();
						if (Turret) {
							if (detail_level == 0)
								Scene->DelGraphic(Turret);
							else
								Scene->AddGraphic(Turret);

							Solid* TurretBase = WeaponIter->GetTurretBase();
							if (TurretBase) {
								if (detail_level == 0)
									Scene->DelGraphic(TurretBase);
								else
									Scene->AddGraphic(TurretBase);
							}
						}

						if (WeaponIter->IsMissile()) {
							for (int AmmoIndex = 0; AmmoIndex < WeaponIter->Ammo(); AmmoIndex++) {
								Solid* Store = WeaponIter->GetVisibleStore(AmmoIndex);
								if (Store) {
									if (detail_level == 0)
										Scene->DelGraphic(Store);
									else
										Scene->AddGraphic(Store);
								}
							}
						}
					}
				}
			}
		}
	}
	else {
		const int NumModels = detail.NumModels(detail_level);

		if (NumModels > 1) {
			for (int ModelIndex = 0; ModelIndex < NumModels; ModelIndex++) {
				Graphic* Gfx = detail.GetRep(detail_level, ModelIndex);
				if (!Gfx)
					continue;

				const FVector Spin = detail.GetSpin(detail_level, ModelIndex);

				Matrix StarOrient = cam.Orientation();
				StarOrient.Pitch(Spin.X);
				StarOrient.Yaw(Spin.Z);
				StarOrient.Roll(Spin.Y);

				Gfx->MoveTo(cam.Pos() + detail.GetOffset(detail_level, ModelIndex));
				Gfx->SetOrientation(ToFMatrix(StarOrient));
			}
		}
	}
}

// +--------------------------------------------------------------------+

void
Ship::ShowRep()
{
	for (int i = 0; i < detail.NumModels(detail_level); i++) {
		Graphic* g = detail.GetRep(detail_level, i);
		g->Show();
	}

	if (gear && (gear->GetState() != LandingGear::GEAR_UP)) {
		for (int i = 0; i < gear->NumGear(); i++) {
			Solid* g = gear->GetGear(i);
			if (g) g->Show();
		}
	}

	ListIter<WeaponGroup> g = weapons;
	while (++g) {
		ListIter<Weapon> w = g->GetWeapons();
		while (++w) {
			Solid* turret = w->GetTurret();
			if (turret) {
				turret->Show();

				Solid* turret_base = w->GetTurretBase();
				if (turret_base)
					turret_base->Show();
			}

			if (w->IsMissile()) {
				for (int i = 0; i < w->Ammo(); i++) {
					Solid* store = w->GetVisibleStore(i);
					if (store) {
						store->Show();
					}
				}
			}
		}
	}
}

void
Ship::HideRep()
{
	for (int i = 0; i < detail.NumModels(detail_level); i++) {
		Graphic* g = detail.GetRep(detail_level, i);
		g->Hide();
	}

	if (gear && (gear->GetState() != LandingGear::GEAR_UP)) {
		for (int i = 0; i < gear->NumGear(); i++) {
			Solid* g = gear->GetGear(i);
			if (g) g->Hide();
		}
	}

	ListIter<WeaponGroup> g = weapons;
	while (++g) {
		ListIter<Weapon> w = g->GetWeapons();
		while (++w) {
			Solid* turret = w->GetTurret();
			if (turret) {
				turret->Hide();

				Solid* turret_base = w->GetTurretBase();
				if (turret_base)
					turret_base->Hide();
			}

			if (w->IsMissile()) {
				for (int i = 0; i < w->Ammo(); i++) {
					Solid* store = w->GetVisibleStore(i);
					if (store) {
						store->Hide();
					}
				}
			}
		}
	}
}

void
Ship::EnableShadows(bool enable)
{
	for (int i = 0; i < detail.NumModels(detail_level); i++) {
		Graphic* g = detail.GetRep(detail_level, i);

		if (g->IsSolid()) {
			Solid* s = (Solid*)g;

			ListIter<Shadow> iter = s->GetShadows();
			while (++iter) {
				Shadow* shadow = iter.value();
				shadow->SetEnabled(enable);
			}
		}
	}
}

// +--------------------------------------------------------------------+

bool
Ship::Update(SimObject* obj)
{
	if (obj == ward)
		ward = 0;

	if (obj == target) {
		target = 0;
		subtarget = 0;
	}

	if (obj == carrier) {
		carrier = 0;
		dock = 0;
		inbound = 0;
	}

	if (obj->Type() == SimObject::SIM_SHOT ||
		obj->Type() == SimObject::SIM_DRONE) {
		SimShot* s = (SimShot*)obj;

		if (sensor_drone == s)
			sensor_drone = 0;

		if (decoy_list.contains(s))
			decoy_list.remove(s);

		if (threat_list.contains(s))
			threat_list.remove(s);
	}

	return SimObserver::Update(obj);
}

// +--------------------------------------------------------------------+

int
Ship::GetFuelLevel() const
{
	if (reactors.size() > 0) {
		PowerSource* reactor = reactors[0];
		if (reactor)
			return reactor->Charge();
	}

	return 0;
}

void
Ship::SetThrottle(double percent)
{
	throttle_request = percent;

	if (throttle_request < 0)
		throttle_request = 0;
	else if (throttle_request > 100)
		throttle_request = 100;

	if (throttle_request < 50)
		augmenter = false;
}

void
Ship::SetAugmenter(bool enable)
{
	if (throttle <= 50)
		enable = false;

	if (main_drive && main_drive->MaxAugmenter() <= 0)
		enable = false;

	augmenter = enable;
}

// +--------------------------------------------------------------------+

void
Ship::SetTransition(double trans_time, int trans_type, const FVector& trans_loc)
{
	transition_time = (float)trans_time;
	transition_type = trans_type;
	transition_loc = trans_loc;
}

void
Ship::DropOrbit()
{
	if (IsDropship() && transition_type == TRANSITION_NONE && !IsAirborne()) {
		SimRegion* dst_rgn = sim->FindNearestTerrainRegion(this);

		if (dst_rgn &&
			dst_rgn->GetOrbitalRegion()->Primary() ==
			GetRegion()->GetOrbitalRegion()->Primary()) {

			transition_time = 10.0f;
			transition_type = TRANSITION_DROP_ORBIT;
			transition_loc = Location() + Heading() * (float)(-2 * Radius());

			RadioTraffic::SendQuickMessage(this, RadioMessageAction::BREAK_ORBIT);
			SetControls(0);
		}
	}
}

void
Ship::MakeOrbit()
{
	if (IsDropship() && transition_type == TRANSITION_NONE && IsAirborne()) {
		transition_time = 5.0f;
		transition_type = TRANSITION_MAKE_ORBIT;
		transition_loc = Location() + Heading() * (float)(-2 * Radius());

		RadioTraffic::SendQuickMessage(this, RadioMessageAction::MAKE_ORBIT);
		SetControls(0);
	}
}

// +--------------------------------------------------------------------+

bool
Ship::IsInCombat()
{
	if (IsRogue())
		return true;

	bool combat = false;

	ListIter<SimContact> c_iter = ContactList();
	while (++c_iter) {
		SimContact* c = c_iter.value();
		Ship* cship = c->GetShip();
		const int ciff = c->GetIFF(this);
		const FVector delta = c->Location() - Location();
		const double dist = delta.Length();

		if (c->Threat(this) && !cship) {
			if (IsStarship())
				combat = dist < 120e3;
			else
				combat = dist < 60e3;
		}

		else if (cship && ciff > 0 && ciff != GetIFF()) {
			if (IsStarship() && cship->IsStarship())
				combat = dist < 120e3;
			else
				combat = dist < 60e3;
		}
	}

	return combat;
}

// +--------------------------------------------------------------------+

bool
Ship::CanTimeSkip()
{
	Instruction* NavPt = GetNextNavPoint();

	// Preserve original early-out logic
	if (MissionClock() < 10000 /* || NetGame::IsNetGame() */) {
		return false;
	}

	bool bCanSkip = false;

	if (NavPt) {
		bCanSkip = true;

		// Must be in the same region
		if (NavPt->Region() != GetRegion()) {
			bCanSkip = false;
		}
		else {
			// Removed OtherHand(); unified coordinate system
			const FVector TargetLoc = NavPt->Location();

			// Use UE vector API
			const double Distance = FVector::Dist(TargetLoc, Location());

			if (Distance < 30000.0)
				bCanSkip = false;
		}
	}

	// Cannot time skip during combat
	if (bCanSkip) {
		bCanSkip = !IsInCombat();
	}

	return bCanSkip;
}


void
Ship::TimeSkip()
{
	if (CanTimeSkip()) {
		// go back to regular time before performing the skip:
		Game::SetTimeCompression(1);

		transition_time = 7.5f;
		transition_type = TRANSITION_TIME_SKIP;
		transition_loc = Location() + Heading() * (float)(Velocity().Length() * 4);
		// 2500; //(8*Radius());

		if (rand() < 16000)
			transition_loc += BeamLine() * (float)(2.5 * Radius());
		else
			transition_loc += BeamLine() * (float)(-2 * Radius());

		if (rand() < 8000)
			transition_loc += LiftLine() * (float)(-1 * Radius());
		else
			transition_loc += LiftLine() * (float)(1.8 * Radius());

		SetControls(0);
	}

	else if (sim->GetPlayerShip() == this) {
		SetAutoNav(true);
	}
}

// +--------------------------------------------------------------------+

void
Ship::DropCam(double time, double range)
{
	transition_type = TRANSITION_DROP_CAM;

	if (time > 0)
		transition_time = (float)time;
	else
		transition_time = 10.0f;

	FVector offset = Heading() * (float)(Velocity().Length() * 5);
	double lateral_offset = 2 * Radius();
	double vertical_offset = Radius();

	if (vertical_offset > 300)
		vertical_offset = 300;

	if (rand() < 16000)
		lateral_offset *= -1;

	if (rand() < 8000)
		vertical_offset *= -1;

	offset += BeamLine() * (float)lateral_offset;
	offset += LiftLine() * (float)vertical_offset;

	if (range > 0)
		offset *= (float)range;

	transition_loc = Location() + offset;
}

// +--------------------------------------------------------------------+

void
Ship::DeathSpiral()
{
	if (!killer)
		killer = new  ShipKiller(this);

	ListIter<SimSystem> iter = systems;
	while (++iter)
		iter->PowerOff();

	// transfer arcade velocity to newtonian velocity:
	if (flight_model >= 2) {
		velocity += arcade_velocity;
	}

	if (GetIFF() < 100 && !IsGroundUnit()) {
		RadioTraffic::SendQuickMessage(this, RadioMessageAction::DISTRESS);
	}

	transition_type = TRANSITION_DEATH_SPIRAL;

	killer->BeginDeathSpiral();

	transition_time = killer->TransitionTime();
	transition_loc = killer->TransitionLoc();
}

// +--------------------------------------------------------------------+

void
Ship::CompleteTransition()
{
	const int OldType = transition_type;
	transition_time = 0.0f;
	transition_type = TRANSITION_NONE;

	switch (OldType) {
	case TRANSITION_NONE:
	case TRANSITION_DROP_CAM:
	default:
		return;

	case TRANSITION_DROP_ORBIT:
	{
		SetControls(0);

		SimRegion* DstRegion = sim ? sim->FindNearestTerrainRegion(this) : 0;
		if (!DstRegion || !sim)
			return;

		FVector DstLoc = Location() * 0.20f; // removed OtherHand()
		DstLoc.X += 6000.0f * (float)GetElementIndex();
		DstLoc.Z = (float)(TERRAIN_ALTITUDE_LIMIT * 0.95);
		DstLoc += RandomDirection() * 2000.0f;

		sim->RequestHyperJump(this, DstRegion, DstLoc, TRANSITION_DROP_ORBIT);

		ShipStats* Stats = ShipStats::Find(Name());
		if (Stats)
			Stats->AddEvent(SimEvent::BREAK_ORBIT, DstRegion->GetName());
	}
	break;

	case TRANSITION_MAKE_ORBIT:
	{
		SetControls(0);

		SimRegion* DstRegion = sim ? sim->FindNearestSpaceRegion(this) : 0;
		if (!DstRegion || !sim)
			return;

		const double Dist = 200.0e3 + 10.0e3 * GetElementIndex();

		FVector EscVec =
			DstRegion->GetOrbitalRegion()->Location() -
			DstRegion->GetOrbitalRegion()->Primary()->Location();

		EscVec.Z = -100.0f * (float)GetElementIndex();
		EscVec.Normalize();
		EscVec *= (float)(-Dist);
		EscVec += RandomDirection() * 2000.0f;

		sim->RequestHyperJump(this, DstRegion, EscVec, TRANSITION_MAKE_ORBIT);

		ShipStats* Stats = ShipStats::Find(Name());
		if (Stats)
			Stats->AddEvent(SimEvent::MAKE_ORBIT, DstRegion->GetName());
	}
	break;

	case TRANSITION_TIME_SKIP:
	{
		Instruction* NavPt = GetNextNavPoint();

		if (NavPt && sim) {
			const FVector Delta = NavPt->Location() - Location(); // removed OtherHand()

			FVector Unit = Delta;
			Unit.Normalize();

			const FVector Trans = Delta + Unit * -20000.0f;
			const double Dist = Trans.Size();

			double Speed = NavPt->Speed();
			if (Speed < 50.0)
				Speed = 500.0;

			const double ETR = Dist / Speed;

			sim->ResolveTimeSkip(ETR);
		}
	}
	break;

	case TRANSITION_DEATH_SPIRAL:
		SetControls(0);
		transition_type = TRANSITION_DEAD;
		break;
	}
}


bool
Ship::IsAirborne() const
{
	if (region)
		return region->GetType() == SimRegion::AIR_SPACE;

	return false;
}

double
Ship::CompassHeading() const
{
	const FVector heading = Heading();
	double compass_heading = atan2(FMath::Abs(heading.X), heading.Z);

	if (heading.X < 0)
		compass_heading *= -1;

	double result = compass_heading + PI;

	if (result >= 2 * PI)
		result -= 2 * PI;

	return result;
}

double
Ship::CompassPitch() const
{
	const FVector heading = Heading();
	return asin(heading.Y);
}

double
Ship::AltitudeMSL() const
{
	return Location().Y;
}

double
Ship::AltitudeAGL() const
{
	if (altitude_agl < -1000) {
		Ship* pThis = (Ship*)this; // cast-away const
		const FVector loc = Location();

		Terrain* terrain = region->GetTerrain();

		if (terrain)
			pThis->altitude_agl = (float)(loc.Y - terrain->Height(loc.X, loc.Z));
		else
			pThis->altitude_agl = (float)loc.Y;

		if (!FMath::IsFinite(altitude_agl)) {
			pThis->altitude_agl = 0.0f;
		}
	}

	return altitude_agl;
}

double
Ship::GForce() const
{
	return g_force;
}


// +--------------------------------------------------------------------+

WeaponGroup*
Ship::FindWeaponGroup(const char* InName)
{
	if (!InName || !*InName)
		return nullptr;

	WeaponGroup* FoundGroup = nullptr;

	ListIter<WeaponGroup> GroupIter = weapons;
	while (++GroupIter) {
		if (GroupIter->Name() && _stricmp(GroupIter->Name(), InName) == 0) {
			FoundGroup = GroupIter.value();
			break;
		}
	}

	// Create group if not found (original Starshatter behavior)
	if (!FoundGroup) {
		FoundGroup = new WeaponGroup(InName);
		weapons.append(FoundGroup);
	}

	return FoundGroup;
}

void
Ship::SelectWeapon(int n, int w)
{
	if (n < weapons.size())
		weapons[n]->SelectWeapon(w);
}

// +--------------------------------------------------------------------+

void
Ship::CyclePrimary()
{
	if (weapons.isEmpty())
		return;

	if (IsDropship() && primary < weapons.size()) {
		WeaponGroup* p = weapons[primary];
		Weapon* w = p->GetSelected();

		if (w && w->GetTurret()) {
			p->SetFiringOrders(WeaponsOrders::POINT_DEFENSE);
		}
	}

	int n = primary + 1;
	while (n != primary) {
		if (n >= weapons.size())
			n = 0;

		if (weapons[n]->IsPrimary()) {
			weapons[n]->SetFiringOrders(WeaponsOrders::MANUAL);
			break;
		}

		n++;
	}

	primary = n;
}

// +--------------------------------------------------------------------+

void
Ship::CycleSecondary()
{
	if (weapons.isEmpty())
		return;

	int n = secondary + 1;
	while (n != secondary) {
		if (n >= weapons.size())
			n = 0;

		if (weapons[n]->IsMissile())
			break;

		n++;
	}

	secondary = n;

	// automatically switch sensors to appropriate mode:
	if (IsAirborne()) {
		Weapon* missile = GetSecondary();
		if (missile && missile->CanTarget((int)CLASSIFICATION::GROUND_UNITS))
			SetSensorMode(Sensor::GM);
		else if (sensor && sensor->GetMode() == Sensor::GM)
			SetSensorMode(Sensor::STD);
	}
}

int
Ship::GetMissileEta(int Index) const
{
	return (Index >= 0 && Index < UE_ARRAY_COUNT(missile_eta))
		? missile_eta[Index]
		: 0;
}

void
Ship::SetMissileEta(int Id, int Eta)
{
	int Index = INDEX_NONE;

	// Are we already tracking this missile?
	for (int i = 0; i < UE_ARRAY_COUNT(missile_id); ++i)
	{
		if (missile_id[i] == Id)
		{
			Index = i;
			break;
		}
	}

	// If not, find an open slot
	if (Index == INDEX_NONE)
	{
		for (int i = 0; i < UE_ARRAY_COUNT(missile_eta); ++i)
		{
			if (missile_eta[i] == 0)
			{
				Index = i;
				missile_id[i] = Id;
				break;
			}
		}
	}

	// Track the ETA (clamped to original max)
	if (Index != INDEX_NONE)
	{
		Eta = FMath::Clamp(Eta, 0, 3599);
		missile_eta[Index] = static_cast<uint8>(Eta);
	}
}

// +--------------------------------------------------------------------+

void
Ship::DoEMCON()
{
	ListIter<SimSystem> iter = systems;
	while (++iter) {
		SimSystem* s = iter.value();
		s->DoEMCON(emcon);
	}

	old_emcon = emcon;
}

// +--------------------------------------------------------------------+

double
Ship::Thrust(double seconds) const
{
	double total_thrust = 0.0;

	if (main_drive) {
		const FVector H = Heading();
		FVector       V = Velocity();

		const double vmag = V.Normalize();
		double eff_throttle = throttle;
		double thrust_factor = 1.0;

		double Vfwd = FVector::DotProduct(H, V);
		const bool bAugOn = main_drive->IsAugmenterOn();

		if (vmag > vlimit && Vfwd > 0.0) {
			double Vmax = vlimit;
			if (bAugOn)
				Vmax *= 1.5;

			Vfwd = 0.5 * Vfwd + 0.5;

			thrust_factor =
				(Vfwd * FMath::Pow(Vmax, 3.0) / FMath::Pow(vmag, 3.0)) +
				(1.0 - Vfwd);
		}

		if (flcs)
			eff_throttle = flcs->Throttle();

		if (flight_model > 1) {
			eff_throttle /= 100.0;
			eff_throttle *= eff_throttle;
			eff_throttle *= 100.0;
		}

		main_drive->SetThrottle(eff_throttle, augmenter);
		total_thrust += thrust_factor * main_drive->Thrust(seconds);

		if (bAugOn && shake < 1.5f)
			((Ship*)this)->shake = 1.5f;
	}

	return total_thrust;
}


// +--------------------------------------------------------------------+

void
Ship::CycleFLCSMode()
{
	switch (flcs_mode) {
	case FLCS_MANUAL: SetFLCSMode(FLCS_HELM);   break;
	case FLCS_AUTO:   SetFLCSMode(FLCS_MANUAL); break;
	case FLCS_HELM:   SetFLCSMode(FLCS_AUTO);   break;

	default:
		if (IsStarship())
			flcs_mode = (BYTE)FLCS_HELM;
		else
			flcs_mode = (BYTE)FLCS_AUTO;
		break;
	}

	// reset helm heading to compass heading when switching
	// back to helm mode from manual mode:
	if (flcs_mode == FLCS_HELM) {
		if (IsStarship()) {
			SetHelmHeading(CompassHeading());
			SetHelmPitch(CompassPitch());
		}
		else {
			flcs_mode = (BYTE)FLCS_AUTO;
		}
	}
}

void
Ship::SetFLCSMode(int mode)
{
	flcs_mode = (BYTE)mode;

	if (IsAirborne())
		flcs_mode = (BYTE)FLCS_MANUAL;

	if (dir && dir->Type() < SteerAI::SEEKER) {
		switch (flcs_mode) {
		case FLCS_MANUAL: director_info = Game::GetText("flcs.manual"); break;
		case FLCS_AUTO:   director_info = Game::GetText("flcs.auto");   break;
		case FLCS_HELM:   director_info = Game::GetText("flcs.helm");   break;
		default:          director_info = Game::GetText("flcs.fault");  break;
		}

		if (!flcs || !flcs->IsPowerOn())
			director_info = Game::GetText("flcs.offline");
		else if (IsAirborne())
			director_info = Game::GetText("flcs.atmospheric");
	}

	if (flcs)
		flcs->SetMode(mode);
}

int
Ship::GetFLCSMode() const
{
	return (int)flcs_mode;
}

void
Ship::SetTransX(double t)
{
	float limit = design->trans_x;

	if (thruster)
		limit = (float)thruster->TransXLimit();

	trans_x = (float)t;

	if (trans_x) {
		if (trans_x > limit)
			trans_x = limit;
		else if (trans_x < -limit)
			trans_x = -limit;

		// reduce thruster efficiency at high fwd speed:
		const double vfwd = FVector::DotProduct(cam.vrt(), Velocity());
		const double vmag = fabs(vfwd);
		if (vmag > vlimit) {
			if ((trans_x > 0 && vfwd > 0) || (trans_x < 0 && vfwd < 0))
				trans_x *= (float)(pow(vlimit, 4) / pow(vmag, 4));
		}
	}
}

void
Ship::SetTransY(double t)
{
	float limit = design->trans_y;

	if (thruster)
		limit = (float)thruster->TransYLimit();

	trans_y = (float)t;

	if (trans_y) {
		const double vmag = Velocity().Length();

		if (trans_y > limit)
			trans_y = limit;
		else if (trans_y < -limit)
			trans_y = -limit;

		// reduce thruster efficiency at high fwd speed:
		if (vmag > vlimit) {
			const double vfwd = FVector::DotProduct(cam.vpn(), Velocity());

			if ((trans_y > 0 && vfwd > 0) || (trans_y < 0 && vfwd < 0))
				trans_y *= (float)(pow(vlimit, 4) / pow(vmag, 4));
		}
	}
}

void
Ship::SetTransZ(double t)
{
	float limit = design->trans_z;

	if (thruster)
		limit = (float)thruster->TransZLimit();

	trans_z = (float)t;

	if (trans_z) {
		if (trans_z > limit)
			trans_z = limit;
		else if (trans_z < -limit)
			trans_z = -limit;

		// reduce thruster efficiency at high fwd speed:
		const double vfwd = FVector::DotProduct(cam.vup(), Velocity());
		const double vmag = fabs(vfwd);
		if (vmag > vlimit) {
			if ((trans_z > 0 && vfwd > 0) || (trans_z < 0 && vfwd < 0))
				trans_z *= (float)(pow(vlimit, 4) / pow(vmag, 4));
		}
	}
}

void
Ship::ExecFLCSFrame()
{
	if (flcs)
		flcs->ExecSubFrame();
}

// +--------------------------------------------------------------------+

void
Ship::SetHelmHeading(double h)
{
	while (h < 0)
		h += 2 * PI;

	while (h >= 2 * PI)
		h -= 2 * PI;

	helm_heading = (float)h;
}

void
Ship::SetHelmPitch(double p)
{
	const double PITCH_LIMIT = 80 * DEGREES;

	if (p < -PITCH_LIMIT)
		p = -PITCH_LIMIT;
	else if (p > PITCH_LIMIT)
		p = PITCH_LIMIT;

	helm_pitch = (float)p;
}

void
Ship::ApplyHelmYaw(double y)
{
	// rotate compass into helm-relative orientation:
	double compass = CompassHeading() - helm_heading;
	const double turn = y * PI / 4;

	if (compass > PI)
		compass -= 2 * PI;
	else if (compass < -PI)
		compass += 2 * PI;

	// if requested turn is more than 170, reject it:
	if (fabs(compass + turn) > 170 * DEGREES)
		return;

	SetHelmHeading(helm_heading + turn);
}

void
Ship::ApplyHelmPitch(double p)
{
	SetHelmPitch(helm_pitch - p * PI / 4);
}

void
Ship::ApplyPitch(double p)
{
	if (flight_model == 0) { // standard flight model
		if (IsAirborne())
			p *= 0.5;

		// command for pitch up is negative
		if (p < 0) {
			if (alpha > PI / 6) {
				p *= 0.05;
			}
			else if (g_force > 12.0) {
				const double limit = 0.5 - (g_force - 12.0) / 10.0;

				if (limit < 0)
					p = 0;
				else
					p *= limit;
			}
		}

		// command for pitch down is positive
		else if (p > 0) {
			if (alpha < -PI / 8) {
				p *= 0.05;
			}
			else if (g_force < -3) {
				p *= 0.1;
			}
		}
	}

	Physical::ApplyPitch(p);
}

// +--------------------------------------------------------------------+

bool
Ship::FireWeapon(int n)
{
	bool fired = false;

	if (n >= 0 && !CheckFire()) {
		if (n < 4)
			trigger[n] = true;

		if (n < weapons.size()) {
			weapons[n]->SetTrigger(true);
			fired = weapons[n]->GetTrigger();
		}
	}

	if (!fired && sim->GetPlayerShip() == this)
		UIButton::PlaySound(UIButton::SND_REJECT);

	return fired;
}

bool
Ship::FireDecoy()
{
	SimShot* drone = 0;

	if (decoy && !CheckFire()) {
		drone = decoy->Fire();

		if (drone) {
			Observe(drone);
			decoy_list.append(drone);
		}
	}

	if (sim->GetPlayerShip() == this) {
		//if (NetGame::IsNetGame()) {
		//	if (decoy && decoy->Ammo() < 1)
		//		Button::PlaySound(Button::SND_REJECT);
		//}
		//else 
		if (!drone) {
			UIButton::PlaySound(UIButton::SND_REJECT);
		}
	}

	return drone != 0;
}

void
Ship::AddActiveDecoy(Drone* drone)
{
	if (drone) {
		Observe(drone);
		decoy_list.append(drone);
	}
}

Weapon*
Ship::GetPrimary() const
{
	if (weapons.size() > primary)
		return weapons[primary]->GetSelected();
	return 0;
}

Weapon*
Ship::GetSecondary() const
{
	if (weapons.size() > secondary)
		return weapons[secondary]->GetSelected();
	return 0;
}

Weapon*
Ship::GetWeaponByIndex(int n)
{
	for (int i = 0; i < weapons.size(); i++) {
		WeaponGroup* g = weapons[i];

		List<Weapon>& wlist = g->GetWeapons();
		for (int j = 0; j < wlist.size(); j++) {
			Weapon* w = wlist[j];

			if (w->GetIndex() == n) {
				return w;
			}
		}
	}

	return 0;
}

WeaponGroup*
Ship::GetPrimaryGroup() const
{
	if (weapons.size() > primary)
		return weapons[primary];
	return 0;
}

WeaponGroup*
Ship::GetSecondaryGroup() const
{
	if (weapons.size() > secondary)
		return weapons[secondary];
	return 0;
}

WeaponDesign*
Ship::GetPrimaryDesign() const
{
	if (weapons.size() > primary)
		return (WeaponDesign*)weapons[primary]->GetSelected()->Design();
	return 0;
}

WeaponDesign*
Ship::GetSecondaryDesign() const
{
	if (weapons.size() > secondary)
		return (WeaponDesign*)weapons[secondary]->GetSelected()->Design();
	return 0;
}

Weapon*
Ship::GetDecoy() const
{
	return decoy;
}

List<SimShot>&
Ship::GetActiveDecoys()
{
	return decoy_list;
}

List<SimShot>&
Ship::GetThreatList()
{
	return threat_list;
}

void
Ship::AddThreat(SimShot* s)
{
	if (!threat_list.contains(s)) {
		Observe(s);
		threat_list.append(s);
	}
}

void
Ship::DropThreat(SimShot* s)
{
	if (threat_list.contains(s)) {
		threat_list.remove(s);
	}
}

bool
Ship::GetTrigger(int i) const
{
	if (i >= 0) {
		if (i < 4)
			return trigger[i];
		else if (i < weapons.size())
			return weapons[i]->GetTrigger();
	}

	return false;
}

void
Ship::SetTrigger(int i)
{
	if (i >= 0 && !CheckFire()) {
		if (i < 4)
			trigger[i] = true;

		if (i < weapons.size())
			weapons[i]->SetTrigger();
	}
}

// +--------------------------------------------------------------------+

void
Ship::SetSensorMode(int mode)
{
	if (sensor)
		sensor->SetMode((Sensor::Mode)mode);
}

int
Ship::GetSensorMode() const
{
	if (sensor)
		return (int)sensor->GetMode();

	return 0;
}

// +--------------------------------------------------------------------+

bool
Ship::IsTracking(SimObject* tgt)
{
	if (tgt && sensor)
		return sensor->IsTracking(tgt);

	return false;
}

// +--------------------------------------------------------------------+

void
Ship::LockTarget(int type, bool closest, bool hostile)
{
	if (sensor)
		SetTarget(sensor->LockTarget(type, closest, hostile));
}

// +--------------------------------------------------------------------+

void
Ship::LockTarget(SimObject* candidate)
{
	if (sensor)
		SetTarget(sensor->LockTarget(candidate));
	else
		SetTarget(candidate);
}

// +--------------------------------------------------------------------+

double
Ship::InflictDamage(double damage, SimShot* shot, int hit_type, FVector impact)
{
	double damage_applied = 0;

	if (Game::Paused() || IsNetObserver() || IsInvulnerable())
		return damage_applied;

	if (Integrity() == 0) // already dead?
		return damage_applied;

	const double MAX_SHAKE = 7;
	double hull_damage = damage;
	const bool hit_shield = (hit_type & HIT_SHIELD) != 0;
	const bool hit_hull = (hit_type & HIT_HULL) != 0;
	const bool hit_turret = (hit_type & HIT_TURRET) != 0;

	if (impact == FVector::ZeroVector)
		impact = Location();

	if (hit_shield && ShieldStrength() > 0) {
		hull_damage = shield->DeflectDamage(shot, damage);

		if (shot) {
			if (shot->IsBeam()) {
				if (design->beam_hit_sound_resource) {
					if (Game::RealTime() - last_beam_time > 400) {
						USound* s = design->beam_hit_sound_resource->Duplicate();
						s->SetLocation(impact);
						s->SetVolume(AudioConfig::EfxVolume());
						s->Play();

						last_beam_time = Game::RealTime();
					}
				}
			}
			else {
				if (design->bolt_hit_sound_resource) {
					if (Game::RealTime() - last_bolt_time > 400) {
						USound* s = design->bolt_hit_sound_resource->Duplicate();
						s->SetLocation(impact);
						s->SetVolume(AudioConfig::EfxVolume());
						s->Play();

						last_bolt_time = Game::RealTime();
					}
				}
			}
		}
	}

	if (hit_hull) {
		hull_damage = InflictSystemDamage(hull_damage, shot, impact);

		int damage_type = WeaponDesign::DMG_NORMAL;

		if (shot && shot->Design())
			damage_type = shot->Design()->damage_type;

		if (damage_type == WeaponDesign::DMG_NORMAL) {
			damage_applied = hull_damage;
			Physical::InflictDamage(damage_applied, 0);
			//NetUtil::SendObjDamage(this, damage_applied, shot);
		}
	}
	else if (hit_turret) {
		hull_damage = InflictSystemDamage(hull_damage, shot, impact) * 0.3;

		int damage_type = WeaponDesign::DMG_NORMAL;

		if (shot && shot->Design())
			damage_type = shot->Design()->damage_type;

		if (damage_type == WeaponDesign::DMG_NORMAL) {
			damage_applied = hull_damage;
			Physical::InflictDamage(damage_applied, 0);
			//NetUtil::SendObjDamage(this, damage_applied, shot);
		}
	}

	// shake by percentage of maximum damage
	const double newshake = 50 * damage / design->integrity;

	if (shake < MAX_SHAKE) shake += (float)newshake;
	if (shake > MAX_SHAKE) shake = (float)MAX_SHAKE;

	// start fires as needed:
	if ((IsStarship() || IsGroundUnit() || RandomChance(1, 3)) && hit_hull && damage_applied > 0) {
		const int old_integrity = (int)((integrity + damage_applied) / design->integrity * 10);
		const int new_integrity = (int)((integrity) / design->integrity * 10);

		if (new_integrity < 5 && new_integrity < old_integrity) {
			// need accurate hull impact for starships,
			if (rep) {
				FVector detonation = impact * 2 - Location();
				FVector direction = Location() - detonation;
				const double distance = direction.Normalize() * 3;
				rep->CheckRayIntersection(detonation, direction, distance, impact);

				// pull fire back into hull a bit:
				direction = Location() - impact;
				impact += direction * 0.2;

				const float scale = (float)design->scale;

				if (IsDropship())
					sim->CreateExplosion(impact, Velocity(), Explosion::SMOKE_TRAIL, 0.01f * scale, 0.5f * scale, region, this);
				else
					sim->CreateExplosion(impact, Velocity(), Explosion::HULL_FIRE, 0.10f * scale, scale, region, this);
			}
		}
	}

	return damage_applied;
}

double
Ship::InflictSystemDamage(double damage, SimShot* shot, FVector impact)
{
	if (IsNetObserver())
		return 0;

	// find the system that is closest to the impact point:
	SimSystem* system = 0;
	double distance = 1e6;
	double blast_radius = 0;
	int dmg_type = 0;

	if (shot)
		dmg_type = shot->Design()->damage_type;

	const bool dmg_normal = dmg_type == WeaponDesign::DMG_NORMAL;
	const bool dmg_power = dmg_type == WeaponDesign::DMG_POWER;
	const bool dmg_emp = dmg_type == WeaponDesign::DMG_EMP;
	double to_level = 0;

	if (dmg_power) {
		to_level = 1 - damage / 1e4;

		if (to_level < 0)
			to_level = 0;
	}

	// damage caused by weapons applies to closest system:
	if (shot) {
		if (shot->IsMissile())
			blast_radius = 300;

		ListIter<SimSystem> iter = systems;
		while (++iter) {
			SimSystem* candidate = iter.value();
			const double sysrad = candidate->Radius();

			if (dmg_power)
				candidate->DrainPower(to_level);

			if (sysrad > 0 || (dmg_emp && candidate->IsPowerCritical())) {
				const double test_distance = (impact - candidate->MountLocation()).Length();

				if ((test_distance - blast_radius) < sysrad || (dmg_emp && candidate->IsPowerCritical())) {
					if (test_distance < distance) {
						system = candidate;
						distance = test_distance;
					}
				}
			}
		}

		// if a system was in range of the blast, assess the damage:
		if (system) {
			const double hull_damage = damage * system->HullProtection();
			const double sys_damage = damage - hull_damage;
			const double avail = system->Availability();

			if (dmg_normal || (system->IsPowerCritical() && dmg_emp)) {
				system->ApplyDamage(sys_damage);
				//NetUtil::SendSysDamage(this, system, sys_damage);

				master_caution = true;

				if (dmg_normal) {
					if (sys_damage < 100)
						damage -= sys_damage;
					else
						damage -= 100;
				}

				if (system->GetExplosionType() && (avail - system->Availability()) >= 50) {
					float scale = design->explosion_scale;
					if (scale <= 0)
						scale = design->scale;

					sim->CreateExplosion(system->MountLocation(), Velocity() * 0.7f, system->GetExplosionType(),
						0.2f * scale, scale, region, this, system);
				}
			}
		}
	}

	// damage caused by collision applies to all systems:
	else {
		// ignore incidental bumps:
		if (damage < 100)
			return damage;

		ListIter<SimSystem> iter = systems;
		while (++iter) {
			SimSystem* sys = iter.value();

			if (rand() > 24000) {
				const double base_damage = 33.0 + rand() / 1000.0;
				const double sys_damage = base_damage * (1.0 - sys->HullProtection());
				sys->ApplyDamage(sys_damage);
				//NetUtil::SendSysDamage(this, sys, sys_damage);
				damage -= sys_damage;

				master_caution = true;
			}
		}

		// just in case this ship has lots of systems...
		if (damage < 0)
			damage = 0;
	}

	// return damage remaining
	return damage;
}

// +--------------------------------------------------------------------+

int
Ship::ShieldStrength() const
{
	if (!shield) return 0;

	return (int)shield->ShieldLevel();
}

int
Ship::HullStrength() const
{
	if (design)
		return (int)(Integrity() / design->integrity * 100);

	return 10;
}

// +--------------------------------------------------------------------+

SimSystem*
Ship::GetSystem(int sys_id)
{
	SimSystem* s = 0;

	if (sys_id >= 0) {
		if (sys_id < systems.size()) {
			s = systems[sys_id];
			if (s->GetID() == sys_id)
				return s;
		}

		ListIter<SimSystem> iter = systems;
		while (++iter) {
			s = iter.value();

			if (s->GetID() == sys_id)
				return s;
		}
	}

	return 0;
}

// +--------------------------------------------------------------------+

void
Ship::RepairSystem(SimSystem* sys)
{
	if (!repair_queue.contains(sys)) {
		repair_queue.append(sys);
		sys->Repair();
	}
}

// +--------------------------------------------------------------------+

void
Ship::IncreaseRepairPriority(int task_index)
{
	if (task_index > 0 && task_index < repair_queue.size()) {
		SimSystem* task1 = repair_queue.at(task_index - 1);
		SimSystem* task2 = repair_queue.at(task_index);

		repair_queue.at(task_index - 1) = task2;
		repair_queue.at(task_index) = task1;
	}
}

void
Ship::DecreaseRepairPriority(int task_index)
{
	if (task_index >= 0 && task_index < repair_queue.size() - 1) {
		SimSystem* task1 = repair_queue.at(task_index);
		SimSystem* task2 = repair_queue.at(task_index + 1);

		repair_queue.at(task_index) = task2;
		repair_queue.at(task_index + 1) = task1;
	}
}

// +--------------------------------------------------------------------+

void
Ship::ExecMaintFrame(double seconds)
{
	// is it already too late?
	if (life == 0 || integrity < 1) return;

	const DWORD REPAIR_FREQUENCY = 5000;  // once every five seconds
	static DWORD last_repair_frame = 0;   // one ship per game frame

	if (auto_repair &&
		Game::GameTime() - last_repair_time > REPAIR_FREQUENCY &&
		last_repair_frame != Game::Frame()) {

		last_repair_time = Game::GameTime();
		last_repair_frame = Game::Frame();

		ListIter<SimSystem> iter = systems;
		while (++iter) {
			SimSystem* sys = iter.value();

			if (sys->GetStatus() != SYSTEM_STATUS::NOMINAL) {
				bool started_repairs = false;

				// emergency power routing:
				if (sys->GetType() == SYSTEM_CATEGORY::POWER_SOURCE && sys->Availability() < 33) {
					PowerSource* src = (PowerSource*)sys;
					PowerSource* dst = 0;

					for (int i = 0; i < reactors.size(); i++) {
						PowerSource* pwr = reactors[i];

						if (pwr != src && pwr->Availability() > src->Availability()) {
							if (!dst ||
								(pwr->Availability() > dst->Availability() &&
									pwr->Charge() > dst->Charge()))
								dst = pwr;
						}
					}

					if (dst) {
						while (src->Clients().size() > 0) {
							SimSystem* s = src->Clients().at(0);
							src->RemoveClient(s);
							dst->AddClient(s);
						}
					}
				}

				ListIter<SimComponent> comp = sys->GetComponents();
				while (++comp) {
					SimComponent* c = comp.value();

					if (c->GetStatus() < SYSTEM_STATUS::NOMINAL && c->Availability() < 75) {
						if (c->SpareCount() &&
							c->ReplaceTime() <= 300 &&
							(c->Availability() < 50 ||
								c->ReplaceTime() < c->RepairTime())) {

							c->Replace();
							started_repairs = true;
						}
						else if (c->Availability() >= 50 || c->NumJerried() < 5) {
							c->Repair();
							started_repairs = true;
						}
					}
				}

				if (started_repairs)
					RepairSystem(sys);
			}
		}
	}

	if (repair_queue.size() > 0 && RepairTeams() > 0) {
		int team = 0;
		ListIter<SimSystem> iter = repair_queue;
		while (++iter && team < RepairTeams()) {
			SimSystem* sys = iter.value();

			sys->ExecMaintFrame(seconds * RepairSpeed());
			team++;

			if (sys->GetStatus() != SYSTEM_STATUS::MAINT) {
				iter.removeItem();

				// emergency power routing (restore):
				if (sys->GetType() == SYSTEM_CATEGORY::POWER_SOURCE &&
					sys->GetStatus() == SYSTEM_STATUS::NOMINAL) {
					PowerSource* src = (PowerSource*)sys;
					const int isrc = reactors.index(src);

					for (int i = 0; i < reactors.size(); i++) {
						PowerSource* pwr = reactors[i];

						if (pwr != src) {
							List<SimSystem> xfer;

							for (int j = 0; j < pwr->Clients().size(); j++) {
								SimSystem* s = pwr->Clients().at(j);

								if (s->GetSourceIndex() == isrc) {
									xfer.append(s);
								}
							}

							for (int j = 0; j < xfer.size(); j++) {
								SimSystem* s = xfer.at(j);
								pwr->RemoveClient(s);
								src->AddClient(s);
							}
						}
					}
				}
			}
		}
	}
}

// +--------------------------------------------------------------------+

void
Ship::SetNetworkControl(SimDirector* net)
{
	net_control = net;

	delete dir;
	dir = 0;

	if (!net_control && GetIFF() < 100) {
		if (IsStatic())
			dir = 0;
		else if (IsStarship())
			dir = SteerAI::Create(this, SteerAI::STARSHIP);
		else
			dir = SteerAI::Create(this, SteerAI::FIGHTER);
	}
}

void
Ship::SetControls(MotionController* m)
{
	if (IsDropping() || IsAttaining()) {
		if (dir && dir->Type() != DropShipAI::DIR_TYPE) {
			delete dir;
			dir = new  DropShipAI(this);
		}

		return;
	}

	else if (IsSkipping()) {
		if (navsys && sim->GetPlayerShip() == this)
			navsys->EngageAutoNav();
	}

	else if (IsDying() || IsDead()) {
		if (dir) {
			delete dir;
			dir = 0;
		}

		if (navsys && navsys->AutoNavEngaged()) {
			navsys->DisengageAutoNav();
		}

		return;
	}

	else if (life == 0) {
		if (dir || navsys) {
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Warning: dying ship '%s' still has not been destroyed!"),
				ANSI_TO_TCHAR(name)
			);
			delete dir;
			dir = 0;

			if (navsys && navsys->AutoNavEngaged())
				navsys->DisengageAutoNav();
		}

		return;
	}

	if (navsys && navsys->AutoNavEngaged()) {
		NavAI* nav = 0;

		if (dir) {
			if (dir->Type() != NavAI::DIR_TYPE) {
				delete dir;
				dir = 0;
			}
			else {
				nav = (NavAI*)dir;
			}
		}

		if (!nav) {
			nav = new NavAI(this);
			dir = nav;
			return;
		}

		else if (!nav->Complete()) {
			return;
		}
	}

	if (dir) {
		delete dir;
		dir = 0;
	}

	if (m) {
		Keyboard::FlushKeys();
		m->Acquire();
		dir = new ShipManager(this, m);
		director_info = Game::GetText("flcs.auto");
	}
	else if (GetIFF() < 100) {
		if (IsStatic())
			dir = SteerAI::Create(this, SteerAI::GROUND);

		else if (IsStarship() && !IsAirborne())
			dir = SteerAI::Create(this, SteerAI::STARSHIP);

		else
			dir = SteerAI::Create(this, SteerAI::FIGHTER);
	}
}

// +--------------------------------------------------------------------+

FColor
Ship::IFFColor(int iff)
{
	FColor c;

	switch (iff) {
	case 0:  // NEUTRAL, NON-COMBAT
		c = FColor(192, 192, 192);
		break;

	case 1:  // TERELLIAN ALLIANCE
		c = FColor(70, 70, 220);
		break;

	case 2:  // MARAKAN HEGEMONY
		c = FColor(220, 20, 20);
		break;

	case 3:  // BROTHERHOOD OF IRON
		c = FColor(200, 180, 20);
		break;

	case 4:  // ZOLON EMPIRE
		c = FColor(20, 200, 20);
		break;

	case 5:
		c = FColor(128, 0, 128);
		break;

	case 6:
		c = FColor(40, 192, 192);
		break;

	default:
		c = FColor(128, 128, 128);
		break;
	}

	return c;
}

FColor
Ship::MarkerColor() const
{
	return IFFColor(IFF_code);
}

// +--------------------------------------------------------------------+

void
Ship::SetIFF(int iff)
{
	IFF_code = iff;

	if (hangar)
		hangar->SetAllIFF(iff);

	DropTarget();

	if (dir && dir->Type() >= 1000) {
		SteerAI* ai = (SteerAI*)dir;
		ai->DropTarget();
	}
}

// +--------------------------------------------------------------------+

void
Ship::SetRogue(bool r)
{
	const bool was_rogue = IsRogue();

	ff_count = r ? 1000 : 0;

	if (!was_rogue && IsRogue()) {
		UE_LOG(LogTemp, Log, TEXT("Ship '%s' has been made rogue"), *FString(Name()));
	}
	else if (was_rogue && !IsRogue()) {
		UE_LOG(LogTemp, Log, TEXT("Ship '%s' is no longer rogue"), *FString(Name()));
	}
}

void
Ship::SetFriendlyFire(int f)
{
	const bool was_rogue = IsRogue();

	ff_count = f;

	if (!was_rogue && IsRogue()) {
		UE_LOG(LogTemp, Log, TEXT("Ship '%s' has been made rogue with ff_count = %d"), *FString(Name()), ff_count);
	}
	else if (was_rogue && !IsRogue()) {
		UE_LOG(LogTemp, Log, TEXT("Ship '%s' is no longer rogue"), *FString(Name()));
	}
}

void
Ship::IncFriendlyFire(int f)
{
	if (f > 0) {
		const bool was_rogue = IsRogue();

		ff_count += f;

		if (!was_rogue && IsRogue()) {
			UE_LOG(LogTemp, Log, TEXT("Ship '%s' has been made rogue with ff_count = %d"), *FString(Name()), ff_count);
		}
	}
}

// +--------------------------------------------------------------------+

void
Ship::SetEMCON(int e, bool from_net)
{
	if (e < 1)        emcon = 1;
	else if (e > 3)   emcon = 3;
	else              emcon = (BYTE)e;

	//if (emcon != old_emcon && !from_net && NetGame::GetInstance())
	//	NetUtil::SendObjEmcon(this);
}

double
Ship::PCS() const
{
	const double e_factor = design->e_factor[emcon - 1];

	if (IsAirborne() && !IsGroundUnit()) {
		if (AltitudeAGL() < 40)
			return 0;

		if (AltitudeAGL() < 200) {
			const double clutter = AltitudeAGL() / 200;
			return clutter * e_factor;
		}
	}

	return e_factor * pcs;
}

double
Ship::ACS() const
{
	if (IsAirborne() && !IsGroundUnit()) {
		if (AltitudeAGL() < 40)
			return 0;

		if (AltitudeAGL() < 200) {
			const double clutter = AltitudeAGL() / 200;
			return clutter * acs;
		}
	}

	return acs;
}

DWORD
Ship::MissionClock() const
{
	if (launch_time > 0)
		return Game::GameTime() + 1 - launch_time;

	return 0;
}

// +----------------------------------------------------------------------+

Instruction*
Ship::GetRadioOrders() const
{
	return radio_orders;
}

void
Ship::ClearRadioOrders()
{
	if (radio_orders) {
		radio_orders->SetAction(INSTRUCTION_ACTION::NONE);
		radio_orders->ClearTarget();
		radio_orders->SetLocation(FVector::ZeroVector);
	}
}

void
Ship::HandleRadioMessage(RadioMessage* msg)
{
	if (!msg) return;

	static RadioHandler rh;

	if (rh.ProcessMessage(msg, this))
		rh.AcknowledgeMessage(msg, this);
}

// +----------------------------------------------------------------------+

int
Ship::Value() const
{
	return Value(design->type);
}

// +----------------------------------------------------------------------+

int
Ship::Value(int type)
{
	int value = 0;

	switch (type) {
	case (int)CLASSIFICATION::DRONE: 
		value = 10;
		break;
	case (int)CLASSIFICATION::FIGHTER:
		value = 20;
		break;
	case (int)CLASSIFICATION::ATTACK: 
		value = 40;
		break;
	case (int)CLASSIFICATION::LCA:
		value = 50; break;

	case (int)CLASSIFICATION::COURIER:  
		value = 100; 
		break;
	case (int)CLASSIFICATION::CARGO:   
		value = 100;
		break;
	case (int)CLASSIFICATION::CORVETTE:   
		value = 100;
		break;
	case (int)CLASSIFICATION::FREIGHTER:   
		value = 250;
		break;
	case (int)CLASSIFICATION::FRIGATE:
		value = 200; 
		break;
	case (int)CLASSIFICATION::DESTROYER:   
		value = 500;
		break;
	case (int)CLASSIFICATION::CRUISER: 
		value = 800;
		break;
	case (int)CLASSIFICATION::BATTLESHIP: 
		value = 1000;
		break;
	case (int)CLASSIFICATION::CARRIER: 
		value = 1500; 
		break;
	case (int)CLASSIFICATION::SWACS:
		value = 500;
		break;
	case (int)CLASSIFICATION::DREADNAUGHT: 
		value = 1500;
		break;

	case (int)CLASSIFICATION::STATION:
		value = 2500;
		break;
	case (int)CLASSIFICATION::FARCASTER:  
		value = 5000;
		break;

	case (int)CLASSIFICATION::MINE:        
		value = 20;
		break;
	case (int)CLASSIFICATION::COMSAT: 
		value = 200;
		break;
	case (int)CLASSIFICATION::DEFSAT:    
		value = 300;
		break;

	case (int)CLASSIFICATION::BUILDING:  
		value = 100;
		break;
	case (int)CLASSIFICATION::FACTORY:    
		value = 250;
		break;
	case (int)CLASSIFICATION::SAM:    
		value = 100; 
		break;
	case (int)CLASSIFICATION::EWR:       
		value = 200;
		break;
	case (int)CLASSIFICATION::C3I:   
		value = 500; 
		break;
	case (int)CLASSIFICATION::STARBASE:  
		value = 2000;
		break;

	default: 
		value = 100; 
		break;
	}

	return value;
}

// +----------------------------------------------------------------------+

double
Ship::AIValue() const
{
	double value = 0;

	for (int i = 0; i < reactors.size(); i++) {
		const PowerSource* r = reactors[i];
		value += r->Value();
	}

	for (int i = 0; i < drives.size(); i++) {
		const Drive* d = drives[i];
		value += d->Value();
	}

	for (int i = 0; i < weapons.size(); i++) {
		const WeaponGroup* w = weapons[i];
		value += w->Value();
	}

	return value;
}

