/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         Sim.cpp
	AUTHOR:       Carlos Bott
	ORIGINAL:     John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Simulation Universe and Region classes
*/

#include "Sim.h"
#include "SimEvent.h"
#include "SimObject.h"
#include "SimRegion.h"
#include "Starshatter.h"
#include "StarSystem.h"
#include "SimContact.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "SimElement.h"
#include "Instruction.h"
#include "RadioTraffic.h"
#include "SimShot.h"
#include "Drone.h"
#include "Explosion.h"
#include "Debris.h"
#include "Asteroid.h"
#include "Drive.h"
#include "QuantumDrive.h"
#include "Sensor.h"
#include "NavLight.h"
#include "Shield.h"
#include "Weapon.h"
#include "WeaponGroup.h"
#include "Hangar.h"
#include "FlightDeck.h"
#include "Sky.h"
#include "Grid.h"
#include "MFDView.h"
#include "AudioConfig.h"
#include "Mission.h"
#include "MissionEvent.h"
#include "CameraManager.h"
#include "MusicManager.h"
#include "Combatant.h"
#include "CombatGroup.h"
#include "CombatUnit.h"
#include "HUDView.h"
#include "SeekerAI.h"
#include "ShipAI.h"
#include "Power.h"
#include "Callsign.h"
#include "GameScreen.h"
#include "Terrain.h"
#include "TerrainPatch.h"
#include "SimScene.h"

#include "Game.h"
#include "Sound.h"
#include "Bolt.h"
#include "Solid.h"
#include "Sprite.h"
#include "SimLight.h"
#include "DataLoader.h"
#include "ParseUtil.h"
#include "MouseController.h"
#include "PlayerCharacter.h"
#include "Random.h"
#include "SimModel.h"
#include "Video.h"
#include "Graphic.h"
#include "GameStructs.h"

// Minimal Unreal includes (logging + FVector):
#include "CoreMinimal.h"
#include "Logging/LogMacros.h"
#include "Math/Vector.h"
#include "Math/RandomStream.h"
#include "HAL/PlatformString.h"
#include "Containers/StringConv.h"

// NOTE: Render assets like Bitmap are handled as Unreal assets elsewhere (e.g., UTexture2D*).
// This translation unit does notxinclude Bitmap.h.

// --------------------------------------------------------------------
// FVector helpers
// --------------------------------------------------------------------

static FORCEINLINE double VecLen(const FVector& V)
{
	return V.Size();
}

// Normalizes V in-place; returns original length (legacy Point::Normalize semantics)
static FORCEINLINE double NormalizeRetLen(FVector& V)
{
	const double Len = V.Size();
	if (Len > SMALL_NUMBER)
	{
		V /= Len;
	}
	else
	{
		V = FVector::ZeroVector;
	}
	return Len;
}

const char* FormatGameTime();

// +--------------------------------------------------------------------+

class SimHyper
{
public:
	SimHyper(Ship* o, SimRegion* r, const FVector& l, int t, bool h, Ship* fc1, Ship* fc2)
		: ship(o), rgn(r), loc(l), type(t), hyperdrive(h), fc_src(fc1), fc_dst(fc2) {
	}

	Ship* ship;
	SimRegion* rgn;
	FVector     loc;
	int         type;
	bool        hyperdrive;
	Ship* fc_src;
	Ship* fc_dst;
};

// +--------------------------------------------------------------------+

class SimSplash
{
public:
	SimSplash(SimRegion* r, const FVector& l, double d, double n)
		: rgn(r), loc(l), damage(d), range(n),
		owner_name("Collateral Damage"), missile(false) {
	}

	Text        owner_name;
	FVector     loc;
	double      damage;
	double      range;
	SimRegion* rgn;
	bool        missile;
};

// +--------------------------------------------------------------------+

static bool first_frame = true;
Sim* Sim::sim = 0;

Sim::Sim(MotionController* InCtrl)
	: ctrl(InCtrl)
	, test_mode(false)
	, grid_shown(false)
	, dust(nullptr)
	, star_system(nullptr)
	, active_region(nullptr)
	, mission(nullptr)
	, start_time(0)
	, cam_dir(nullptr)
{
	Drive::Initialize();
	Explosion::Initialize();
	FlightDeck::Initialize();
	NavLight::Initialize();
	SimShot::Initialize();
	MFDView::Initialize();
	Asteroid::Initialize();

	// Singleton hookup:
	if (!sim)
	{
		sim = this;
		UE_LOG(LogTemp, Log, TEXT("Sim singleton set (Sim::sim = this)."));
	}
	else if (sim != this)
	{
		UE_LOG(LogTemp, Warning, TEXT("Sim singleton already set; new Sim instance created but not assigned."));
	}

	cam_dir = CameraManager::GetInstance();
	if (!cam_dir)
	{
		UE_LOG(LogTemp, Warning, TEXT("CameraManager::GetInstance() returned null."));
	}
}

Sim::~Sim()
{
	UnloadMission();

	SimShot::Close();
	FlightDeck::Close();
	NavLight::Close();
	Token::close();
	Asteroid::Close();

	if (sim == this)
		sim = 0;
}

// +--------------------------------------------------------------------+

void
Sim::CommitMission()
{
	for (int i = 0; i < regions.size(); i++)
		regions[i]->CommitMission();

	if (ShipStats::NumStats() > 0) {
		UE_LOG(LogTemp, Log,
			TEXT("\n\nFINAL SCORE '%s'"),
			ANSI_TO_TCHAR(mission->GetName()));

		UE_LOG(LogTemp, Log,
			TEXT("Name              Kill1  Kill2  Died   Colls  Points  Cmd Pts"));

		UE_LOG(LogTemp, Log,
			TEXT("----------------  -----  -----  -----  -----  ------  ------"));

		int tk1 = 0;
		int tk2 = 0;
		int td = 0;
		int tc = 0;

		for (int i = 0; i < ShipStats::NumStats(); i++) {
			ShipStats* s = ShipStats::GetStats(i);
			s->Summarize();

			UE_LOG(LogTemp, Log,
				TEXT("%-16s  %5d  %5d  %5d  %5d  %6d  %6d"),
				ANSI_TO_TCHAR(s->GetName()),
				s->GetGunKills(),
				s->GetMissileKills(),
				s->GetDeaths(),
				s->GetColls(),
				s->GetPoints(),
				s->GetCommandPoints());

			tk1 += s->GetGunKills();
			tk2 += s->GetMissileKills();
			td += s->GetDeaths();
			tc += s->GetColls();

			CombatGroup* group = s->GetCombatGroup();

			if (group) {
				Combatant* c = group->GetCombatant();

				if (c)
					c->AddScore(s->GetPoints());

				if (s->GetElementIndex() == 1)
					group->SetSorties(group->GetSorties() + 1);

				group->SetKills(group->GetKills() + s->GetGunKills() + s->GetMissileKills());
				group->SetPoints(group->GetPoints() + s->GetPoints());
			}

			if (s->IsPlayer()) {
				PlayerCharacter* p = PlayerCharacter::GetCurrentPlayer();
				p->ProcessStats(s, start_time);

				if (mission && mission->GetType() == (int)EMISSIONTYPE::TRAINING &&
					s->GetDeaths() == 0 && s->GetColls() == 0)
					p->SetTrained(mission->GetIdentity());

				PlayerCharacter::Save(); // save training state right now before we forget!
			}
		}

		UE_LOG(LogTemp, Verbose, TEXT("--------------------------------------------"));

		UE_LOG(LogTemp, Verbose,
			TEXT("TOTAL             %5d  %5d  %5d  %5d"),
			tk1, tk2, td, tc);

		ShipStats::Initialize();
	}
}

// +--------------------------------------------------------------------+

void
Sim::UnloadMission()
{
	HUDView* hud = HUDView::GetInstance();
	if (hud)
		hud->HideAll();

	ShipStats::Initialize();

	events.destroy();
	mission_elements.destroy();
	elements.destroy();
	finished.destroy();

	if (active_region)
		active_region->Deactivate();

	if (star_system)
		star_system->Deactivate();

	if (mission) {
		mission->SetActive(false);
		mission->SetComplete(true);
	}

	regions.destroy();
	scene->Collect();

	GRAPHIC_DESTROY(dust);

	star_system = 0;
	active_region = 0;
	mission = 0;

	// reclaim memory used by radio traffic:
	RadioTraffic::DiscardMessages();

	// release texture memory for 2D screens:
	Starshatter* stars = Starshatter::GetInstance();
	if (stars)
		stars->InvalidateTextureCache();

	cam_dir = CameraManager::GetInstance();
	if (cam_dir)
		cam_dir->SetShip(0);

	AudioConfig::SetTraining(false);
}

bool
Sim::IsActive() const
{
	return mission && mission->IsActive();
}

bool
Sim::IsComplete() const
{
	return mission && mission->IsComplete();
}

// +--------------------------------------------------------------------+

void
Sim::LoadMission(Mission* m, bool preload_textures)
{
	cam_dir = CameraManager::GetInstance();

	if (!mission) {
		mission = m;
		mission->SetActive(true);

		if (preload_textures) {
			Video* video = Game::GetVideo();
			List<SimModel>    all_models;
			// Textures are Unreal assets now (e.g., UTexture2D*), so legacy Bitmap preload is disabled.

			ListIter<MissionElement> elem_iter = mission->GetElements();
			while (++elem_iter) {
				MissionElement* elem = elem_iter.value();
				const ShipDesign* design = elem->GetDesign();

				if (design) {
					for (int i = 0; i < 4; i++) {
						List<SimModel>& models = (List<SimModel>&) design->models[i]; // cast-away const

						ListIter<SimModel> model_iter = models;
						while (++model_iter) {
							SimModel* model = model_iter.value();
							if (!all_models.contains(model)) {
								all_models.append(model);

								ListIter<Surface> surf_iter = model->GetSurfaces();
								while (++surf_iter) {
									Surface* surface = surf_iter.value();
									video->PreloadSurface(surface);
								}
							}
						}
					}
				}
			}
		}
	}
}

void
Sim::ExecMission()
{
	cam_dir = CameraManager::GetInstance();
	if (!mission) {
		UE_LOG(LogTemp, Warning,
			TEXT("Sim::ExecMission() - No mission to execute."));
		return;
	}

	if (elements.size() || finished.size()) {
		UE_LOG(LogTemp, Warning,
			TEXT("Sim::ExecMission(%s) mission is already executing."),
			ANSI_TO_TCHAR(mission->GetName()));
		return;
	}

	UE_LOG(LogTemp, Log,
		TEXT("Exec Mission: '%s'"),
		ANSI_TO_TCHAR(mission->GetName()));

	if (cam_dir)
		cam_dir->Reset();

	if (mission->GetStardate() > 0)
		StarSystem::SetBaseTime(mission->GetStardate(), true);

	star_system = mission->GetStarSystem();
	star_system->Activate(*scene);

	int dust_factor = 0;

	if (Starshatter::GetInstance())
		dust_factor = Starshatter::GetInstance()->Dust();

	if (star_system->NumDust() * dust_factor) {
		dust = new Dust(star_system->NumDust() * 2 * (dust_factor + 1), dust_factor > 1);
		scene->AddGraphic(dust);
	}

	CreateRegions();
	BuildLinks();
	CreateElements();
	CopyEvents();

	first_frame = true;
	start_time = Game::GameTime();

	AudioConfig::SetTraining(mission->GetType() == (int)EMISSIONTYPE::TRAINING);
}

// +--------------------------------------------------------------------+

void
Sim::CreateRegions()
{
	const char* active_region_name = 0;

	if (mission)
		active_region_name = mission->GetRegion();
	else return;

	ListIter<StarSystem> iter = mission->GetSystemList();
	while (++iter) {
		StarSystem* sys = iter.value();

		// insert objects from star system:
		ListIter<OrbitalBody> star = sys->Bodies();
		while (++star) {
			ListIter<OrbitalBody> planet = star->Satellites();
			while (++planet) {
				ListIter<OrbitalBody> moon = planet->Satellites();
				while (++moon) {
					ListIter<OrbitalRegion> rgn = moon->Regions();
					while (++rgn) {
						SimRegion* sim_region = new SimRegion(this, rgn.value());
						regions.append(sim_region);
						if (!strcmp(active_region_name, sim_region->GetName())) {
							ActivateRegion(sim_region);
						}
					}
				}

				ListIter<OrbitalRegion> rgn = planet->Regions();
				while (++rgn) {
					SimRegion* sim_region = new SimRegion(this, rgn.value());
					regions.append(sim_region);
					if (!strcmp(active_region_name, sim_region->GetName())) {
						ActivateRegion(sim_region);
					}
				}
			}

			ListIter<OrbitalRegion> rgn = star->Regions();
			while (++rgn) {
				SimRegion* sim_region = new SimRegion(this, rgn.value());
				regions.append(sim_region);
				if (!strcmp(active_region_name, sim_region->GetName())) {
					ActivateRegion(sim_region);
				}
			}
		}
	}
}

// +--------------------------------------------------------------------+

void
Sim::BuildLinks()
{
	ListIter<SimRegion> iter = regions;
	while (++iter) {
		SimRegion* rgn = iter.value();
		OrbitalRegion* orb = rgn->GetOrbitalRegion();

		if (orb) {
			ListIter<Text> lnk_iter = orb->Links();
			while (++lnk_iter) {
				Text* t = lnk_iter.value();

				SimRegion* tgt = FindRegion(*t);

				if (tgt && !rgn->GetLinks().contains(tgt))
					rgn->GetLinks().append(tgt);
			}
		}
	}
}

void
Sim::CreateElements()
{
	ListIter<MissionElement> ElementIter = mission->GetElements();
	while (++ElementIter) {
		MissionElement* MissionElem = ElementIter.value();

		// add element to a carrier?
		if (MissionElem->IsSquadron()) {
			Ship* Carrier = FindShip(MissionElem->GetCarrier());
			if (Carrier) {
				Hangar* HangarPtr = Carrier->GetHangar();

				if (HangarPtr) {
					int32* DefaultLoadout = nullptr;

					if (MissionElem->Loadouts().size()) {
						MissionLoad* MissionLoadPtr = MissionElem->Loadouts().at(0);

						if (MissionLoadPtr->GetName().length()) {
							ShipDesign* ShipDesignPtr = (ShipDesign*)MissionElem->GetDesign();
							ListIter<ShipLoad> ShipLoadIter = ShipDesignPtr->loadouts;
							while (++ShipLoadIter) {
								ShipLoad* ShipLoadPtr = ShipLoadIter.value();

								if (MissionLoadPtr->GetName() == ShipLoadPtr->name)
									DefaultLoadout = ShipLoadPtr->load;
							}
						}

						if (!DefaultLoadout) {
							DefaultLoadout = MissionLoadPtr->GetStations();
						}
					}

					HangarPtr->CreateSquadron(MissionElem->GetName(), MissionElem->GetCombatGroup(),
						MissionElem->GetDesign(), MissionElem->Count(),
						MissionElem->GetIFF(),
						DefaultLoadout, MissionElem->MaintCount(), MissionElem->DeadCount());

					SimElement* Element = CreateElement(MissionElem->GetName(),
						MissionElem->GetIFF(),
						MissionElem->MissionRole());

					Element->SetCarrier(Carrier);
					Element->SetCombatGroup(MissionElem->GetCombatGroup());
					Element->SetCombatUnit(MissionElem->GetCombatUnit());
					Element->SetCount(MissionElem->Count());
					Element->SetRogue(false);
					Element->SetPlayable(false);
					Element->SetLoadout(DefaultLoadout);
				}
			}
		}

		// create the element in space:
		else {
			Ship* Carrier = nullptr;
			Hangar* HangarPtr = nullptr;
			int32 SquadronIndex = -1;
			int32 SlotIndex = 0;

			// first create the package element:
			SimElement* Element = CreateElement(MissionElem->GetName(),
				MissionElem->GetIFF(),
				MissionElem->MissionRole());

			Element->SetPlayer(MissionElem->IsPlayer());
			Element->SetCombatGroup(MissionElem->GetCombatGroup());
			Element->SetCombatUnit(MissionElem->GetCombatUnit());
			Element->SetCommandAILevel(MissionElem->GetCommandAI());
			Element->SetHoldTime(MissionElem->HoldTime());
			Element->SetZoneLock(MissionElem->GetZoneLock() ? true : false);
			Element->SetRogue(MissionElem->IsRogue());
			Element->SetPlayable(MissionElem->IsPlayable());
			Element->SetIntelLevel(MissionElem->IntelLevel());

			// if this is the player's element, make sure to activate the region:
			if (MissionElem->IsPlayer()) {
				SimRegion* Region = FindRegion(MissionElem->GetRegion());

				if (Region && Region != active_region)
					ActivateRegion(Region);
			}

			// if element belongs to a squadron,
			// find the carrier, squadron, flight deck, etc.:
			if (MissionElem->GetSquadron().length() > 0) {
				MissionElement* SquadronElem = mission->FindElement(MissionElem->GetSquadron());

				if (SquadronElem) {
					Element->SetSquadron(MissionElem->GetSquadron());

					SimElement* Commander = FindElement(SquadronElem->GetCarrier());

					if (Commander) {
						Element->SetCommander(Commander);
						Carrier = Commander->GetShip(1);

						if (Carrier) {
							Element->SetCarrier(Carrier);
							HangarPtr = Carrier->GetHangar();

							for (int32 s = 0; s < HangarPtr->NumSquadrons(); s++) {
								if (HangarPtr->SquadronName(s) == MissionElem->GetSquadron()) {
									SquadronIndex = s;
									break;
								}
							}
						}
					}
				}
			}

			else if (MissionElem->GetCommander().length() > 0) {
				SimElement* Commander = FindElement(MissionElem->GetCommander());

				if (Commander) {
					Element->SetCommander(Commander);
				}
			}

			ListIter<Instruction> ObjectiveIter = MissionElem->Objectives();
			while (++ObjectiveIter) {
				Instruction* Objective = ObjectiveIter.value();
				Instruction* NewInstruction = nullptr;

				NewInstruction = new Instruction(*Objective);

				Element->AddObjective(NewInstruction);
			}

			if (MissionElem->Instructions().size() > 0) {
				ListIter<Text> InstructionIter = MissionElem->Instructions();
				while (++InstructionIter) {
					Element->AddInstruction(*InstructionIter);
				}
			}

			ListIter<Instruction> NavIter = MissionElem->NavList();
			while (++NavIter) {
				SimRegion* Region = FindRegion(NavIter->RegionName());

				if (!Region)
					Region = FindRegion(MissionElem->GetRegion());

				if (Region) {
					Instruction* NavPoint = new
						Instruction(Region, OtherHand(NavIter->Location()), NavIter->GetAction());

					NavPoint->SetStatus(NavIter->GetStatus());
					NavPoint->SetEMCON(NavIter->EMCON());
					NavPoint->SetFormation(NavIter->GetFormation());
					NavPoint->SetSpeed(NavIter->Speed());
					NavPoint->SetTarget(NavIter->TargetName());
					NavPoint->SetHoldTime(NavIter->HoldTime());
					NavPoint->SetFarcast(NavIter->Farcast());

					Element->AddNavPoint(NavPoint);
				}
			}

			bool bAlertPrep = false;
			int32* Loadout = nullptr;
			int32 Respawns = MissionElem->RespawnCount();

			// if ships are to start on alert,
			// spot them onto the appropriate launch deck:
			if (HangarPtr && Element && MissionElem->Count() > 0 && MissionElem->IsAlert()) {
				FlightDeck* Deck = nullptr;
				int32 Queue = 1000;
				const ShipDesign* ShipDesignPtr = MissionElem->GetDesign();

				if (ShipDesignPtr) {
					for (int32 i = 0; i < Carrier->NumFlightDecks(); i++) {
						FlightDeck* FlightDeckPtr = Carrier->GetFlightDeck(i);
						int32 DeckQueue = HangarPtr->PreflightQueue(FlightDeckPtr);

						if (FlightDeckPtr && FlightDeckPtr->IsLaunchDeck() && FlightDeckPtr->SpaceLeft(ShipDesignPtr->type) && DeckQueue < Queue) {
							Queue = DeckQueue;
							Deck = FlightDeckPtr;
						}
					}
				}

				if (Deck) {
					bAlertPrep = true;

					// choose best loadout:
					if (MissionElem->Loadouts().size()) {
						MissionLoad* MissionLoadPtr = MissionElem->Loadouts().at(0);
						if (MissionLoadPtr->GetName().length()) {
							ListIter<ShipLoad> ShipLoadIter = ((ShipDesign*)ShipDesignPtr)->loadouts;
							while (++ShipLoadIter) {
								if (!_stricmp(ShipLoadIter->name, MissionLoadPtr->GetName()))
									Loadout = ShipLoadIter->load;
							}
						}

						else {
							Loadout = MissionLoadPtr->GetStations();
						}
					}

					Element->SetLoadout(Loadout);

					for (int32 i = 0; i < MissionElem->Count(); i++) {
						int32 SquadronLocal = -1;
						int32 SlotLocal = -1;

						if (HangarPtr->FindAvailSlot(MissionElem->GetDesign(), SquadronLocal, SlotLocal)) {
							bAlertPrep = bAlertPrep &&
								HangarPtr->GotoAlert(SquadronLocal,
									SlotLocal,
									Deck,
									Element,
									Loadout,
									true,    // package for launch
									true);   // expedite

							HangarSlot* HangarSlotPtr = (HangarSlot*)HangarPtr->GetSlot(SquadronLocal, SlotLocal);
							Ship* AlertShip = HangarPtr->GetShip(HangarSlotPtr);

							if (AlertShip) {
								AlertShip->SetRespawnCount(Respawns);

								if (MissionElem->IsPlayer()) {
									if (AlertShip->GetRegion()) {
										AlertShip->GetRegion()->SetPlayerShip(AlertShip);
									}
									else {
										UE_LOG(LogTemp, Warning,
											TEXT("WARNING: alert ship '%s' region is null"),
											ANSI_TO_TCHAR(AlertShip->Name()));
									}
								}
							}
						}
					}
				}
			}

			if (!bAlertPrep) {
				// then, create the ships:
				for (int32 i = 0; i < MissionElem->Count(); i++) {
					MissionShip* MissionShipPtr = nullptr;
					Text ShipName = MissionElem->GetShipName(i);
					Text RegistryNum = MissionElem->GetRegistry(i);
					Text RegionName = MissionElem->GetRegion();

					if (MissionElem->Ships().size() > i) {
						MissionShipPtr = MissionElem->Ships()[i];
						ShipName = MissionShipPtr->Name();
						RegistryNum = MissionShipPtr->RegNum();
						RegionName = MissionShipPtr->Region();
					}

					FVector SpawnLocation = OtherHand(MissionElem->GetLocation());

					if (MissionShipPtr && fabs(MissionShipPtr->Location().X) < 1e9) {
						SpawnLocation = OtherHand(MissionShipPtr->Location());
					}
					else if (i) {
						FVector Offset = OtherHand(FVector(
							FMath::FRandRange(-1.f, 1.f),
							FMath::FRandRange(-1.f, 1.f),
							0.f
						));

						Offset.Z = FMath::FRandRange(-1000.f, 1000.f);

						if (MissionElem->Count() < 5)
							Offset *= 0.3;

						SpawnLocation += Offset;
					}

					// choose best loadout:
					ListIter<MissionLoad> LoadIter = MissionElem->Loadouts();
					while (++LoadIter) {
						if ((LoadIter->GetShip() == i) || (LoadIter->GetShip() < 0 && Loadout == nullptr)) {
							if (LoadIter->GetName().length()) {
								ListIter<ShipLoad> ShipLoadIter = ((ShipDesign*)MissionElem->GetDesign())->loadouts;
								while (++ShipLoadIter) {
									if (!_stricmp(ShipLoadIter->name, LoadIter->GetName()))
										Loadout = ShipLoadIter->load;
								}
							}

							else {
								Loadout = LoadIter->GetStations();
							}
						}
					}

					Element->SetLoadout(Loadout);

					Ship* NewShip = CreateShip(ShipName, RegistryNum,
						(ShipDesign*)MissionElem->GetDesign(),
						RegionName, SpawnLocation,
						MissionElem->GetIFF(),
						MissionElem->GetCommandAI(),
						Loadout);

					if (NewShip) {
						double Heading = MissionElem->GetHeading();
						const Skin* SkinPtr = MissionElem->GetSkin();

						if (MissionShipPtr) {
							Heading = MissionShipPtr->Heading();

							if (MissionShipPtr->GetSkin())
								SkinPtr = MissionShipPtr->GetSkin();
						}

						NewShip->SetRogue(MissionElem->IsRogue());
						NewShip->SetInvulnerable(MissionElem->IsInvulnerable());
						NewShip->SetHeading(0, 0, Heading + PI);
						NewShip->SetRespawnCount(Respawns);
						NewShip->UseSkin(SkinPtr);

						const int32 SeedValue = 123456;
						FRandomStream RandomStream(SeedValue);
						NewShip->SetRespawnLoc(OtherHand(RandomStream.VRand()) * 2.f);

						if (NewShip->IsStarship())
							NewShip->SetHelmHeading(Heading);

						else if (NewShip->IsAirborne() && NewShip->AltitudeAGL() > 25)
							NewShip->SetVelocity(OtherHand(NewShip->Heading()) * 250);

						if (Element)
							Element->AddShip(NewShip);

						if (HangarPtr)
							HangarPtr->FindSlot(NewShip, SquadronIndex, SlotIndex, Hangar::ACTIVE);

						if (NewShip->GetRegion() && MissionElem->IsPlayer())
							NewShip->GetRegion()->SetPlayerShip(NewShip);

						if (NewShip->NumFlightDecks()) {
							for (int32 FlightDeckIndex = 0; FlightDeckIndex < NewShip->NumFlightDecks(); FlightDeckIndex++) {
								FlightDeck* ShipDeck = NewShip->GetFlightDeck(FlightDeckIndex);
								if (ShipDeck)
									ShipDeck->Orient(NewShip);
							}
						}

						if (MissionShipPtr) {
							NewShip->SetVelocity(OtherHand(MissionShipPtr->Velocity()));
							NewShip->SetIntegrity((float)MissionShipPtr->Integrity());
							NewShip->SetRespawnCount(MissionShipPtr->Respawns());

							if (MissionShipPtr->Ammo()[0] > -10) {
								for (int32 AmmoIndex = 0; AmmoIndex < 64; AmmoIndex++) {
									Weapon* WeaponPtr = NewShip->GetWeaponByIndex(AmmoIndex + 1);
									if (WeaponPtr)
										WeaponPtr->SetAmmo(MissionShipPtr->Ammo()[AmmoIndex]);
									else
										break;
								}
							}

							if (MissionShipPtr->Fuel()[0] > -10) {
								for (int32 ReactorIndex = 0; ReactorIndex < 4; ReactorIndex++) {
									if (NewShip->Reactors().size() > ReactorIndex) {
										PowerSource* PowerSourcePtr = NewShip->Reactors()[ReactorIndex];
										PowerSourcePtr->SetCapacity(MissionShipPtr->Fuel()[ReactorIndex]);
									}
								}
							}

							if (MissionShipPtr->Decoys() > -10) {
								Weapon* DecoyWeapon = NewShip->GetDecoy();
								if (DecoyWeapon)
									DecoyWeapon->SetAmmo(MissionShipPtr->Decoys());
							}

							if (MissionShipPtr->Probes() > -10) {
								Weapon* ProbeWeapon = NewShip->GetProbeLauncher();
								if (ProbeWeapon)
									ProbeWeapon->SetAmmo(MissionShipPtr->Probes());
							}
						}

						Shield* ShieldPtr = NewShip->GetShield();

						if (ShieldPtr) {
							ShieldPtr->SetPowerLevel(50);
						}

						if (NewShip->Class() > CLASSIFICATION::FRIGATE) {
							ListIter<WeaponGroup> WeaponGroupIter = NewShip->GetWeapons();
							while (++WeaponGroupIter) {
								WeaponGroup* WeaponGroupPtr = WeaponGroupIter.value();

								// anti-air weapon?
								if (WeaponGroupPtr->GetDesign()->target_type & (int) CLASSIFICATION::DRONE) {
									WeaponGroupPtr->SetFiringOrders(WeaponsOrders::POINT_DEFENSE);
								}
								else {
									WeaponGroupPtr->SetFiringOrders(WeaponsOrders::MANUAL);
								}
							}
						}

						if (NewShip->Class() > CLASSIFICATION::DRONE && NewShip->Class() < CLASSIFICATION::STATION) {
							ShipStats* Stats = ShipStats::Find(ShipName);
							if (Stats) {
								char DesignName[64];
								sprintf_s(DesignName, "%s %s", NewShip->Abbreviation(), NewShip->Design()->display_name);
								Stats->SetType(DesignName);
								Stats->SetShipClass((int) NewShip->Class());
								Stats->SetRole(Mission::GetRoleName(MissionElem->MissionRole()));
								Stats->SetIFF(NewShip->GetIFF());
								Stats->SetRegion(MissionElem->GetRegion());
								Stats->SetCombatGroup(MissionElem->GetCombatGroup());
								Stats->SetCombatUnit(MissionElem->GetCombatUnit());
								Stats->SetPlayer(MissionElem->IsPlayer());
								Stats->SetElementIndex(NewShip->GetElementIndex());
							}
						}
					}  // ship
				}     // count
			}
		}
	}
}

void
Sim::CopyEvents()
{
	events.destroy();

	if (mission) {
		ListIter<MissionEvent> iter = mission->GetEvents();
		while (++iter) {
			MissionEvent* orig = iter.value();
			MissionEvent* event = new MissionEvent(*orig);
			events.append(event);
		}
	}
}

// +--------------------------------------------------------------------+

const char*
Sim::FindAvailCallsign(int IFF)
{
	const char* call = "Unidentified";

	for (int i = 0; i < 32; i++) {
		call = Callsign::GetCallsign(IFF);

		if (!FindElement(call))
			break;
	}

	return call;
}

SimElement*
Sim::CreateElement(const char* callsign, int IFF, int type)
{
	SimElement* elem = new SimElement(callsign, IFF, type);
	elements.append(elem);
	return elem;
}

void
Sim::DestroyElement(SimElement* elem)
{
	if (elements.contains(elem))
		elements.remove(elem);

	delete elem;
}

SimElement*
Sim::FindElement(const char* name)
{
	ListIter<SimElement> iter = elements;

	while (++iter) {
		SimElement* elem = iter.value();
		Text     ename = elem->Name();

		if (ename == name)
			return elem;
	}

	return 0;
}

// +--------------------------------------------------------------------+

int
Sim::GetAssignedElements(SimElement* elem, List<SimElement>& assigned)
{
	assigned.clear();

	if (elem) {
		for (int i = 0; i < elements.size(); i++) {
			SimElement* e = elements.at(i);
			if (!e->IsSquadron() && e->GetAssignment() == elem)
				assigned.append(e);
		}
	}

	return assigned.size();
}

// +--------------------------------------------------------------------+

Ship*
Sim::CreateShip(const char* name, const char* reg_num, ShipDesign* design, const char* rgn_name, const FVector& loc, int IFF, int cmd_ai, const int* loadout)
{
	if (!design) {
		UE_LOG(LogTemp, Warning,
			TEXT("WARNING: CreateShip(%s): invalid design"),
			ANSI_TO_TCHAR(name));
		return 0;
	}

	SimRegion* rgn = FindRegion(rgn_name);

	if (!rgn) {
		return 0;
	}

	Ship* ship = new Ship(name, reg_num, design, IFF, cmd_ai, loadout);
	ship->MoveTo(OtherHand(loc));

	if (rgn) {
		UE_LOG(LogTemp, Log,
			TEXT("Inserting Ship(%s) into Region(%s) (%s)"),
			ANSI_TO_TCHAR(ship->Name()),
			ANSI_TO_TCHAR(rgn->GetName()),
			ANSI_TO_TCHAR(FormatGameTime()));

		rgn->InsertObject(ship);

		if (ship->IsAirborne() && ship->AltitudeAGL() > 25)
			ship->SetVelocity(OtherHand(ship->Heading()) * 250);
	}

	return ship;
}

Ship*
Sim::FindShip(const char* name, const char* rgn_name)
{
	Ship* ship = 0;

	if (rgn_name) {
		SimRegion* rgn = FindRegion(rgn_name);
		if (rgn)
			ship = rgn->FindShip(name);
	}

	if (!ship) {
		ListIter<SimRegion> rgn = regions;
		while (++rgn && !ship)
			ship = rgn->FindShip(name);
	}

	return ship;
}

void
Sim::DestroyShip(Ship* ship)
{
	SimRegion* rgn = ship->GetRegion();
	if (rgn)
		rgn->DestroyShip(ship);
}

Ship*
Sim::FindShipByObjID(uint32 objid)
{
	Ship* ship = 0;

	ListIter<SimRegion> rgn = regions;
	while (++rgn && !ship)
		ship = rgn->FindShipByObjID(objid);

	return ship;
}

SimShot*
Sim::FindShotByObjID(uint32 objid)
{
	SimShot* shot = 0;

	ListIter<SimRegion> rgn = regions;
	while (++rgn && !shot)
		shot = rgn->FindShotByObjID(objid);

	return shot;
}

// +--------------------------------------------------------------------+

Orbital*
Sim::FindOrbitalBody(const char* name)
{
	Orbital* body = 0;

	if (mission) {
		ListIter<StarSystem> iter = mission->GetSystemList();
		while (++iter && !body) {
			StarSystem* sys = iter.value();
			body = sys->FindOrbital(name);
		}
	}

	return body;
}


// +--------------------------------------------------------------------+

SimShot*
Sim::CreateShot(const FVector& pos, const Camera& shot_cam, WeaponDesign* design, const Ship* ship, SimRegion* rgn)
{
	SimShot* shot = 0;

	if (design->drone)
		shot = new Drone(pos, shot_cam, design, ship);
	else
		shot = new SimShot(pos, shot_cam, design, ship);

	if (rgn)
		rgn->InsertObject(shot);

	else if (active_region)
		active_region->InsertObject(shot);

	return shot;
}

// +--------------------------------------------------------------------+

Explosion*
Sim::CreateExplosion(const FVector& pos, const FVector& vel, int type, float exp_scale, float part_scale, SimRegion* rgn, SimObject* source, SimSystem* sys)
{
	// don't bother creating explosions that can't be seen:
	if (!rgn || !active_region || rgn != active_region)
		return 0;

	Explosion* exp = new Explosion(type, pos, vel, exp_scale, part_scale, rgn, source);

	if (rgn)
		rgn->InsertObject(exp);

	else if (active_region)
		active_region->InsertObject(exp);

	return exp;
}

// +--------------------------------------------------------------------+

Debris*
Sim::CreateDebris(const FVector& pos, const FVector& vel, SimModel* model, double mass, SimRegion* rgn)
{
	Debris* debris = new Debris(model, pos, vel, mass);

	if (rgn)
		rgn->InsertObject(debris);

	else if (active_region)
		active_region->InsertObject(debris);

	return debris;
}

// +--------------------------------------------------------------------+

Asteroid*
Sim::CreateAsteroid(const FVector& pos, int t, double mass, SimRegion* rgn)
{
	Asteroid* asteroid = new Asteroid(t, pos, mass);

	if (rgn)
		rgn->InsertObject(asteroid);

	else if (active_region)
		active_region->InsertObject(asteroid);

	return asteroid;
}

// +--------------------------------------------------------------------+

void
Sim::CreateSplashDamage(Ship* ship)
{
	if (ship && ship->GetRegion() && ship->Design()->splash_radius > 1) {
		SimSplash* splash = new
			SimSplash(ship->GetRegion(),
				OtherHand(ship->Location()),
				ship->Design()->integrity / 4,
				ship->Design()->splash_radius);

		splash->owner_name = ship->Name();
		splashlist.append(splash);
	}
}

// +--------------------------------------------------------------------+

void
Sim::CreateSplashDamage(SimShot* shot)
{
	if (shot && shot->GetRegion()) {
		double damage = shot->Damage();
		if (damage < shot->Design()->damage)
			damage = shot->Design()->damage;

		SimSplash* splash = new
			SimSplash(shot->GetRegion(),
				OtherHand(shot->Location()),
				damage,
				shot->Design()->lethal_radius);

		if (shot->Owner())
			splash->owner_name = shot->Owner()->Name();

		splash->missile = shot->IsMissile();

		splashlist.append(splash);
		CreateExplosion(OtherHand(shot->Location()), FVector::ZeroVector, Explosion::SHOT_BLAST, 20.0f, 1.0f, shot->GetRegion());
	}
}

// +--------------------------------------------------------------------+

void
Sim::ShowGrid(int show)
{
	PlayerCharacter* player = PlayerCharacter::GetCurrentPlayer();

	if (player && player->GridMode() == 0) {
		show = 0;
		grid_shown = false;
	}

	ListIter<SimRegion> rgn = regions;
	while (++rgn) {
		rgn->ShowGrid(show);
	}

	grid_shown = show ? true : false;
}

bool
Sim::GridShown() const
{
	return grid_shown;
}

// +--------------------------------------------------------------------+

List<StarSystem>&
Sim::GetSystemList()
{
	if (mission)
		return mission->GetSystemList();

	static List<StarSystem> dummy_system_list;
	return dummy_system_list;
}

// +--------------------------------------------------------------------+

void
Sim::NextView()
{
	if (active_region)
		active_region->NextView();
}

Ship*
Sim::GetPlayerShip()
{
	if (active_region)
		return active_region->GetPlayerShip();

	Starshatter* stars = Starshatter::GetInstance();
	if (stars && stars->InCutscene()) {
		Ship* player = 0;

		ListIter<SimRegion> rgn = regions;
		while (++rgn && !player) {
			player = rgn->GetPlayerShip();
		}

		return player;
	}

	return 0;
}

SimElement*
Sim::GetPlayerElement()
{
	SimElement* elem = 0;

	for (int i = 0; i < elements.size(); i++) {
		SimElement* e = elements[i];

		if (e->GetPlayer() > 0)
			elem = e;
	}

	return elem;
}

bool
Sim::IsSelected(Ship* s)
{
	if (active_region)
		return active_region->IsSelected(s);

	return false;
}

ListIter<Ship>
Sim::GetSelection()
{
	if (active_region)
		return active_region->GetSelection();

	static List<Ship> empty;
	return empty;
}

void
Sim::ClearSelection()
{
	if (active_region)
		active_region->ClearSelection();
}

void
Sim::AddSelection(Ship* s)
{
	if (active_region)
		active_region->AddSelection(s);
}

void
Sim::SetSelection(Ship* newsel)
{
	if (active_region)
		active_region->SetSelection(newsel);
}

// +--------------------------------------------------------------------+

void
Sim::SetTestMode(bool t)
{
	test_mode = t;
	Ship* pship = GetPlayerShip();

	if (pship)
		if (IsTestMode())
			pship->SetControls(0);
		else
			pship->SetControls(ctrl);
}

// +--------------------------------------------------------------------+

SimRegion*
Sim::FindRegion(const char* name)
{
	ListIter<SimRegion> rgn = regions;
	while (++rgn)
		if (rgn->name == name)
			return rgn.value();

	return 0;
}

SimRegion* Sim::FindRegion(const FString& Name)
{
	return FindRegion(TCHAR_TO_ANSI(*Name));
}

SimRegion*
Sim::FindRegion(OrbitalRegion* orgn)
{
	ListIter<SimRegion> rgn = regions;
	while (++rgn)
		if (rgn->orbital_region == orgn)
			return rgn.value();

	return 0;
}

// +--------------------------------------------------------------------+

SimRegion*
Sim::FindNearestSpaceRegion(SimObject* object)
{
	return FindNearestRegion(object, REAL_SPACE);
}

SimRegion*
Sim::FindNearestTerrainRegion(SimObject* object)
{
	return FindNearestRegion(object, AIR_SPACE);
}

SimRegion*
Sim::FindNearestRegion(SimObject* object, int type)
{
	if (!object) return 0;

	SimRegion* result = 0;
	double      distance = 1.0e40;
	FVector     objloc = OtherHand(object->Location());

	if (object->GetRegion())
		objloc += OtherHand(object->GetRegion()->GetLocation());

	ListIter<SimRegion> rgn = regions;
	while (++rgn) {
		if (rgn->GetType() == type) {
			OrbitalRegion* orgn = rgn->GetOrbitalRegion();
			if (orgn) {
				const double test = FMath::Abs(VecLen(OtherHand(orgn->Location()) - objloc));
				if (test < distance) {
					result = rgn.value();
					distance = test;
				}
			}
		}
	}

	return result;
}

SimRegion* Sim::FindNearestSpaceRegion(const Orbital* orb)
{
	if (!orb)
		return nullptr;

	return FindNearestSpaceRegionAt(orb->Location());
}

SimRegion* Sim::FindNearestSpaceRegionAt(const FVector& loc)
{
	SimRegion* result = nullptr;
	double     distance = 1.0e40;

	const FVector objloc = OtherHand(loc);

	ListIter<SimRegion> rgn = regions;
	while (++rgn) {
		OrbitalRegion* orgn = rgn->GetOrbitalRegion();
		if (!orgn)
			continue;

		const double test = FMath::Abs(VecLen(OtherHand(orgn->Location()) - objloc));
		if (test < distance) {
			result = rgn.value();
			distance = test;
		}
	}

	return result;
}

// +--------------------------------------------------------------------+

bool
Sim::ActivateRegion(SimRegion* rgn)
{
	if (rgn && active_region != rgn && regions.contains(rgn)) {
		if (active_region)
			active_region->Deactivate();

		if (!active_region || active_region->GetSystem() != rgn->GetSystem()) {
			if (active_region)
				active_region->GetSystem()->Deactivate();
			rgn->GetSystem()->Activate(*scene);
		}

		active_region = rgn;
		star_system = active_region->GetSystem();

		if (star_system) {
			star_system->SetActiveRegion(active_region->orbital_region);
		}
		else {
			UE_LOG(LogTemp, Warning,
				TEXT("Sim::ActivateRegion() No star system found for rgn '%s'"),
				ANSI_TO_TCHAR(rgn->GetName()));
		}

		active_region->Activate();
		return true;
	}

	return false;
}

// +--------------------------------------------------------------------+

void
Sim::RequestHyperJump(Ship* obj, SimRegion* rgn, const FVector& loc,
	int type, Ship* fc1, Ship* fc2)
{
	bool hyperdrive = false;

	if (obj->GetQuantumDrive() && obj->GetQuantumDrive()->Subtype() == QuantumDrive::HYPER)
		hyperdrive = true;

	jumplist.append(new SimHyper(obj, rgn, loc, type, hyperdrive, fc1, fc2));
}

// +--------------------------------------------------------------------+

void
Sim::ExecFrame(double DeltaSeconds)
{
	if (first_frame) {
		first_frame = false;
		//netgame = NetGame::Create();
	}

	//if (netgame)
	//	netgame->ExecFrame();

	if (regions.isEmpty()) {
		active_region = nullptr;
		rgn_queue.clear();
		jumplist.destroy();
		scene->Collect();
		return;
	}

	ListIter<SimElement> ElementIter = elements;
	while (++ElementIter)
		if (!ElementIter->IsSquadron())
			ElementIter->ExecFrame(DeltaSeconds);

	ListIter<SimRegion> RegionIter = regions;
	while (++RegionIter)
		if (RegionIter.value() != active_region && RegionIter->GetNumShips() && !rgn_queue.contains(RegionIter.value()))
			rgn_queue.append(RegionIter.value());

	// execframe for one inactive sim region:
	if (rgn_queue.size()) {
		SimRegion* ExecRegion = rgn_queue.removeIndex(0);

		while (ExecRegion && (ExecRegion->GetNumShips() == 0 || ExecRegion == active_region))
			if (rgn_queue.size())
				ExecRegion = rgn_queue.removeIndex(0);
			else
				ExecRegion = nullptr;

		if (ExecRegion)
			ExecRegion->ExecFrame(DeltaSeconds);
	}

	if (active_region)
		active_region->ExecFrame(DeltaSeconds);

	ExecEvents(DeltaSeconds);
	ResolveHyperList();
	ResolveSplashList();

	// GC all the dead objects:
	scene->Collect();

	if (!IsTestMode()) {
		ListIter<SimElement> FinishedIter = elements;
		while (++FinishedIter) {
			SimElement* Element = FinishedIter.value();
			if (!Element->IsSquadron() && Element->IsFinished()) {
				finished.append(FinishedIter.removeItem());
			}
		}
	}

	// setup music
	if (!MusicManager::IsNoMusic()) {
		Starshatter* Stars = Starshatter::GetInstance();
		if (Stars && Stars->GetGameMode() == EGameMode::PLAY) {
			Ship* PlayerShip = GetPlayerShip();
			if (PlayerShip) {
				const int32 Phase = PlayerShip->GetFlightPhase();

				if (Phase < Ship::ACTIVE) {
					MusicManager::SetMode(MusicManager::LAUNCH);
				}

				else if (Phase > Ship::ACTIVE) {
					MusicManager::SetMode(MusicManager::RECOVERY);
				}

				else {
					if (PlayerShip->IsInCombat()) {
						MusicManager::SetMode(MusicManager::COMBAT);
					}

					else {
						MusicManager::SetMode(MusicManager::FLIGHT);
					}
				}
			}
		}
	}
}


void
Sim::ExecEvents(double seconds)
{
	ListIter<MissionEvent> iter = events;
	while (++iter) {
		MissionEvent* event = iter.value();
		event->ExecFrame(seconds);
	}
}

void
Sim::ResolveHyperList()
{
	// resolve the hyper space transitions:
	if (jumplist.size()) {
		Ship* pship = GetPlayerShip();

		ListIter<SimHyper> j_iter = jumplist;
		while (++j_iter) {
			SimHyper* jump = j_iter.value();
			Ship* jumpship = jump->ship;

			if (jumpship) {
				SimRegion* dest = jump->rgn;

				if (!dest)
					dest = FindNearestSpaceRegion(jumpship);

				if (dest) {
					// bring along fighters on deck:
					ListIter<FlightDeck> deck = jumpship->FlightDecks();
					while (++deck) {
						for (int i = 0; i < deck->NumSlots(); i++) {
							Ship* s = deck->GetShip(i);

							if (s) {
								dest->InsertObject(s);
								s->ClearTrack();
							}
						}
					}

					if (jump->type == 0 && !jump->hyperdrive) {
						// bring along nearby ships:
						// have to do it in two parts, because inserting the ships
						// into the destination corrupts the iter over the current
						// region's list of ships...

						// part one: gather the ships that will be jumping:
						List<Ship> riders;
						ListIter<Ship> neighbor = jumpship->GetRegion()->GetShips();
						while (++neighbor) {
							if (neighbor->IsDropship()) {
								Ship* s = neighbor.value();
								if (s == jumpship) continue;

								const FVector Delta = s->Location() - jumpship->Location();

								if (Delta.Size() < 5e3) {
									riders.append(s);
								}
							}
						}

						// part two: now transfer the list to the destination:
						for (int i = 0; i < riders.size(); i++) {
							Ship* s = riders[i];
							const FVector Delta = s->Location() - jumpship->Location();

							dest->InsertObject(s);
							s->MoveTo(OtherHand(jump->loc) + Delta);
							s->ClearTrack();

							if (jump->fc_dst) {
								const double r = jump->fc_dst->Roll();
								const double p = jump->fc_dst->Pitch();
								const double w = jump->fc_dst->Yaw();

								s->SetAbsoluteOrientation(r, p, w);
								s->SetVelocity(jump->fc_dst->Heading() * 500.0);
							}

							ProcessEventTrigger(MissionEvent::TRIGGER_JUMP, 0, s->Name());
						}
					}

					// now it is safe to move the main jump ship:
					dest->InsertObject(jumpship);
					jumpship->MoveTo(OtherHand(jump->loc));
					jumpship->ClearTrack();

					ProcessEventTrigger(MissionEvent::TRIGGER_JUMP, 0, jumpship->Name());
					//NetUtil::SendObjHyper(jumpship, dest->Name(), jump->loc, jump->fc_src, jump->fc_dst, jump->type);

					// if using farcaster:
					if (jump->fc_src) {
						UE_LOG(LogTemp, Log, TEXT("Ship '%s' farcast to '%s'"),
							UTF8_TO_TCHAR(jumpship->Name()),
							UTF8_TO_TCHAR(dest->GetName())
						);
						CreateExplosion(jumpship->Location(), FVector::ZeroVector, Explosion::QUANTUM_FLASH, 1.0f, 0.0f, dest);

						if (jump->fc_dst) {
							const double r = jump->fc_dst->Roll();
							const double p = jump->fc_dst->Pitch();
							const double w = jump->fc_dst->Yaw();

							jumpship->SetAbsoluteOrientation(r, p, w);
							jumpship->SetVelocity(jump->fc_dst->Heading() * 500.0);
						}

						jumpship->SetHelmHeading(jumpship->CompassHeading());
						jumpship->SetHelmPitch(0);
					}

					// break orbit:
					else if (jump->type == Ship::TRANSITION_DROP_ORBIT) {
						
						UE_LOG(LogTemp, Log,
							TEXT("Ship '%s' broke orbit to '%s'"),
							ANSI_TO_TCHAR(jumpship->Name()),
							ANSI_TO_TCHAR(dest->GetName()));

						jumpship->SetAbsoluteOrientation(0, PI / 4, 0);
						jumpship->SetVelocity(jumpship->Heading() * 1.0e3);
					}

					// make orbit:
					else if (jump->type == Ship::TRANSITION_MAKE_ORBIT) {
						
						UE_LOG(LogTemp, Log,
							TEXT("Ship '%s' achieved orbit '%s'"),
							ANSI_TO_TCHAR(jumpship->Name()),
							ANSI_TO_TCHAR(dest->GetName()));

						jumpship->LookAt(FVector::ZeroVector);
						jumpship->SetVelocity(jumpship->Heading() * 500.0);
					}

					// hyper jump:
					else {
						UE_LOG(LogTemp, Log,
							TEXT("Ship '%s' quantum to '%s'"),
							ANSI_TO_TCHAR(jumpship->Name()),
							ANSI_TO_TCHAR(dest->GetName()));

						if (jump->hyperdrive)
							CreateExplosion(jumpship->Location(), FVector::ZeroVector, Explosion::HYPER_FLASH, 1.0f, 1.0f, dest);
						else
							CreateExplosion(jumpship->Location(), FVector::ZeroVector, Explosion::QUANTUM_FLASH, 1.0f, 0.0f, dest);

						jumpship->LookAt(FVector::ZeroVector);
						jumpship->SetVelocity(jumpship->Heading() * 500.0);
						jumpship->SetHelmHeading(jumpship->CompassHeading());
						jumpship->SetHelmPitch(0);
					}
				}

				else if (regions.size() > 1) {
					
					UE_LOG(LogTemp, Warning,
						TEXT("Warning: Unusual jump request for ship '%s'"),
						ANSI_TO_TCHAR(jumpship->Name()));

					regions[1]->InsertObject(jumpship);
				}

				Sensor* sensor = jumpship->GetSensor();
				if (sensor)
					sensor->ClearAllContacts();
			}
		}

		jumplist.destroy();

		if (pship && pship->GetRegion()) {
			if (active_region != pship->GetRegion()) {
				pship->GetRegion()->SetPlayerShip(pship);
			}
		}
	}
}

void
Sim::ResolveSplashList()
{
	if (splashlist.size()) {
		ListIter<SimSplash> iter = splashlist;
		while (++iter) {
			SimSplash* splash = iter.value();

			if (!splash->rgn)
				continue;

			// damage ships:
			ListIter<Ship> s_iter = splash->rgn->GetShips();
			while (++s_iter) {
				Ship* ship = s_iter.value();

				const double distance = (ship->Location() - splash->loc).Size();

				if (distance > 1 && distance < splash->range) {
					const double damage = splash->damage * (1 - distance / splash->range);
					//if (!NetGame::IsNetGameClient()) {
						ship->InflictDamage(damage);
					//}

					const int ship_destroyed = (!ship->InTransition() && ship->Integrity() < 1.0f);

					// then delete the ship:
					if (ship_destroyed) {
						const FString KillerName = UTF8_TO_TCHAR(splash->owner_name);
						const FString ShipName = ship->Name();
						const FString TimeStr = FormatGameTime();

						UE_LOG(LogTemp, Log,
							TEXT("    %s Killed %s (%s)"),
							*KillerName,
							*ShipName,
							*TimeStr
						);
						// record the kill
						ShipStats* killer = ShipStats::Find(splash->owner_name);
						if (killer) {
							if (splash->missile)
								killer->AddEvent(SimEvent::MISSILE_KILL, ship->Name());
							else
								killer->AddEvent(SimEvent::GUNS_KILL, ship->Name());
						}

						Ship* owner = FindShip(splash->owner_name, splash->rgn->GetName());
						if (owner && owner->GetIFF() != ship->GetIFF()) {
							if (ship->GetIFF() > 0 || owner->GetIFF() > 1) {
								killer->AddPoints(ship->Value());

								SimElement* elem = owner->GetElement();
								if (elem) {
									if (owner->GetElementIndex() > 1) {
										Ship* s = elem->GetShip(1);

										if (s) {
											ShipStats* cmdr_stats = ShipStats::Find(s->Name());
											if (cmdr_stats) {
												cmdr_stats->AddCommandPoints(ship->Value() / 2);
											}
										}
									}

									SimElement* cmdr = elem->GetCommander();
									if (cmdr) {
										Ship* s = cmdr->GetShip(1);

										if (s) {
											ShipStats* cmdr_stats = ShipStats::Find(s->Name());
											if (cmdr_stats) {
												cmdr_stats->AddCommandPoints(ship->Value() / 2);
											}
										}
									}
								}
							}
						}

						ShipStats* killee = ShipStats::Find(ship->Name());
						if (killee)
							killee->AddEvent(SimEvent::DESTROYED, splash->owner_name);

						ship->DeathSpiral();
					}
				}
			}

			// damage drones:
			ListIter<Drone> drone_iter = splash->rgn->GetDrones();
			while (++drone_iter) {
				Drone* drone = drone_iter.value();

				const double distance = (drone->Location() - splash->loc).Size();

				if (distance > 1 && distance < splash->range) {
					const double damage = splash->damage * (1 - distance / splash->range);
					drone->InflictDamage(damage);

					const int destroyed = (drone->Integrity() < 1.0f);

					// then mark the drone for deletion:
					if (destroyed) {
						//NetUtil::SendWepDestroy(drone);
						sim->CreateExplosion(drone->Location(), drone->Velocity(), 21 /* was LARGE_EXP */, 1.0f, 1.0f, splash->rgn);
						drone->SetLife(0);
					}
				}
			}
		}

		splashlist.destroy();
	}
}

// +--------------------------------------------------------------------+

void
Sim::ProcessEventTrigger(int type, int event_id, const char* ship, int param)
{
	Text ship_name = ship;

	ListIter<MissionEvent> iter = events;
	while (++iter) {
		MissionEvent* event = iter.value();

		if (event->IsPending() && event->Trigger() == type) {
			switch (type) {
			case MissionEvent::TRIGGER_DAMAGE:
			case MissionEvent::TRIGGER_DESTROYED:
			case MissionEvent::TRIGGER_JUMP:
			case MissionEvent::TRIGGER_LAUNCH:
			case MissionEvent::TRIGGER_DOCK:
			case MissionEvent::TRIGGER_TARGET:
				if (event->TriggerParam() <= param) {
					if (ship_name.indexOf(event->TriggerShip()) == 0)
						event->Activate();
				}
				break;

			case MissionEvent::TRIGGER_NAVPT:
				if (event->TriggerParam() == param) {
					if (ship_name.indexOf(event->TriggerShip()) == 0)
						event->Activate();
				}
				break;

			case MissionEvent::TRIGGER_EVENT:
			case MissionEvent::TRIGGER_SKIPPED:
				if (event->TriggerParam() == event_id)
					event->Activate();
				break;
			}
		}
	}
}

double
Sim::MissionClock() const
{
	return (Game::GameTime() - start_time) / 1000.0;
}

// +--------------------------------------------------------------------+

void
Sim::SkipCutscene()
{
	Starshatter* stars = Starshatter::GetInstance();
	if (stars && stars->InCutscene()) {
		ListIter<MissionEvent>  iter = events;
		bool                    end = false;
		double                  end_time = 0;

		while (++iter && !end) {
			MissionEvent* event = iter.value();

			if (event->IsPending() || event->IsActive()) {
				if (event->Event() == MissionEvent::END_SCENE ||
					event->Event() == MissionEvent::END_MISSION) {
					end = true;
					end_time = event->Time();
				}

				if (event->Event() == MissionEvent::FIRE_WEAPON) {
					event->Skip();
				}
				else {
					event->Activate();
					event->Execute(true);
				}
			}
		}

		const double skip_time = end_time - MissionClock();
		if (skip_time > 0) {
			Game::SkipGameTime(skip_time);
		}
	}
}

// +--------------------------------------------------------------------+

void
Sim::ResolveTimeSkip(double seconds)
{
	double skipped = 0;

	// allow elements to process hold time, and release as needed:
	ListIter<SimElement> elem = elements;
	while (++elem)
		elem->ExecFrame(seconds);

	// step through the skip, ten seconds at a time:
	if (active_region) {
		double total_skip = seconds;
		double frame_skip = 10;

		while (total_skip > frame_skip) {
			if (active_region->CanTimeSkip()) {
				active_region->ResolveTimeSkip(frame_skip);
				total_skip -= frame_skip;
				skipped += frame_skip;
			}
			// break out early if player runs into bad guys...
			else {
				total_skip = 0;
			}
		}

		if (total_skip > 0)
			active_region->ResolveTimeSkip(total_skip);

		skipped += total_skip;
	}

	// give player control after time skip:
	Ship* player_ship = GetPlayerShip();
	if (player_ship) {
		player_ship->SetAutoNav(false);
		player_ship->SetThrottle(75);

		HUDView* hud = HUDView::GetInstance();
		if (hud)
			hud->SetHUDMode(EHUDMode::Tactical);

		if (IsTestMode())
			player_ship->SetControls(0);
	}

	Game::SkipGameTime(skipped);
	CameraManager::SetCameraMode(CameraManager::MODE_COCKPIT);
}

// +--------------------------------------------------------------------+

ListIter<MissionElement>
Sim::GetMissionElements()
{
	mission_elements.destroy();

	ListIter<SimElement> iter = elements;
	while (++iter) {
		SimElement* elem = iter.value();

		int num_live_ships = 0;

		for (int i = 0; i < elem->NumShips(); i++) {
			Ship* s = elem->GetShip(i + 1);

			if (s && !s->IsDying() && !s->IsDead())
				num_live_ships++;
		}

		if (elem->IsSquadron() || num_live_ships > 0) {
			MissionElement* msn_elem = CreateMissionElement(elem);

			if (msn_elem)
				mission_elements.append(msn_elem);
		}
	}

	return mission_elements;
}

MissionElement*
Sim::CreateMissionElement(SimElement* elem)
{
	MissionElement* msn_elem = 0;

	if (elem->IsSquadron()) {
		if (!elem->GetCarrier() || elem->GetCarrier()->Integrity() < 1)
			return msn_elem;
	}

	if (elem && !elem->IsNetObserver()) {
		msn_elem = new MissionElement;

		msn_elem->SetName(elem->Name());
		msn_elem->SetIFF(elem->GetIFF());
		msn_elem->SetMissionRole(elem->Type());

		if (elem->IsSquadron() && elem->GetCarrier()) {
			Ship* carrier = elem->GetCarrier();

			msn_elem->SetCarrier(carrier->Name());
			msn_elem->SetCount(elem->GetCount());
			msn_elem->SetLocation(OtherHand(carrier->Location()));

			if (carrier->GetRegion())
				msn_elem->SetRegion(carrier->GetRegion()->GetName());

			int     squadron_index = 0;
			Hangar* hangar = FindSquadron(elem->Name(), squadron_index);

			if (hangar) {
				msn_elem->SetDeadCount(hangar->NumShipsDead(squadron_index));
				msn_elem->SetMaintCount(hangar->NumShipsMaint(squadron_index));

				const ShipDesign* design = hangar->SquadronDesign(squadron_index);
				msn_elem->SetDesign(design);

				Text design_path = design->path_name;
				design_path.setSensitive(false);

				if (design_path.indexOf("/Mods/Ships") == 0) {
					design_path = design_path.substring(11, 1000);
					msn_elem->SetPath(design_path);
				}
			}
		}
		else {
			msn_elem->SetSquadron(elem->GetSquadron());
			msn_elem->SetCount(elem->NumShips());
		}

		if (elem->GetCommander())
			msn_elem->SetCommander(elem->GetCommander()->Name());

		msn_elem->SetCombatGroup(elem->GetCombatGroup());
		msn_elem->SetCombatUnit(elem->GetCombatUnit());

		Ship* ship = elem->GetShip(1);
		if (ship) {
			if (ship->GetRegion())
				msn_elem->SetRegion(ship->GetRegion()->GetName());

			msn_elem->SetLocation(OtherHand(ship->Location()));
			msn_elem->SetDesign(ship->Design());

			msn_elem->SetPlayer(elem->GetPlayer());
			msn_elem->SetCommandAI(elem->GetCommandAILevel());
			msn_elem->SetHoldTime((int)elem->GetHoldTime());
			msn_elem->SetZoneLock(elem->GetZoneLock());
			msn_elem->SetHeading(ship->CompassHeading());

			msn_elem->SetPlayable(elem->IsPlayable());
			msn_elem->SetRogue(elem->IsRogue());
			msn_elem->SetIntelLevel(elem->IntelLevel());

			Text design_path = ship->Design()->path_name;
			design_path.setSensitive(false);

			if (design_path.indexOf("/Mods/Ships") == 0) {
				design_path = design_path.substring(11, 1000);
				msn_elem->SetPath(design_path);
			}

			msn_elem->SetRespawnCount(ship->RespawnCount());
		}

		MissionLoad* loadout = new MissionLoad;
		FMemory::Memcpy(loadout->GetStations(), elem->Loadout(), 16 * sizeof(int));
		msn_elem->Loadouts().append(loadout);

		const int num_obj = elem->NumObjectives();
		for (int i = 0; i < num_obj; i++) {
			Instruction* o = elem->GetObjective(i);
			Instruction* instr = new Instruction(*o);
			msn_elem->AddObjective(instr);
		}

		const int num_inst = elem->NumInstructions();
		for (int i = 0; i < num_inst; i++) {
			Text instr = elem->GetInstruction(i);
			msn_elem->AddInstruction(instr);
		}

		ListIter<Instruction> nav_iter = elem->GetFlightPlan();
		while (++nav_iter) {
			Instruction* nav = nav_iter.value();
			Instruction* npt = new Instruction(nav->RegionName(), nav->Location(), nav->GetAction());

			npt->SetFormation(nav->GetFormation());
			npt->SetSpeed(nav->Speed());
			npt->SetTarget(nav->TargetName());
			npt->SetHoldTime(nav->HoldTime());
			npt->SetFarcast(nav->Farcast());
			npt->SetStatus(nav->GetStatus());

			msn_elem->AddNavPoint(npt);
		}

		for (int i = 0; i < elem->NumShips(); i++) {
			ship = elem->GetShip(i + 1);

			if (ship) {
				MissionShip* s = new MissionShip;

				s->SetName(ship->Name());
				s->SetRegNum(ship->Registry());
				s->SetRegion(ship->GetRegion()->GetName());
				s->SetLocation(OtherHand(ship->Location()));
				s->SetVelocity(OtherHand(ship->Velocity()));

				s->SetRespawns(ship->RespawnCount());
				s->SetHeading(ship->CompassHeading());
				s->SetIntegrity(ship->Integrity());

				if (ship->GetDecoy())
					s->SetDecoys(ship->GetDecoy()->Ammo());

				if (ship->GetProbeLauncher())
					s->SetProbes(ship->GetProbeLauncher()->Ammo());

				int n;
				int ammo[16];
				int fuel[4];

				for (n = 0; n < 16; n++) {
					Weapon* w = ship->GetWeaponByIndex(n + 1);

					if (w)
						ammo[n] = w->Ammo();
					else
						ammo[n] = -10;
				}

				for (n = 0; n < 4; n++) {
					if (ship->Reactors().size() > n)
						fuel[n] = ship->Reactors()[n]->Charge();
					else
						fuel[n] = -10;
				}

				s->SetAmmo(ammo);
				s->SetFuel(fuel);

				msn_elem->Ships().append(s);
			}
		}
	}

	return msn_elem;
}

Hangar*
Sim::FindSquadron(const char* name, int& index)
{
	Hangar* hangar = 0;

	ListIter<SimRegion> iter = regions;
	while (++iter && !hangar) {
		SimRegion* rgn = iter.value();

		ListIter<Ship> s_iter = rgn->GetCarriers();
		while (++s_iter && !hangar) {
			Ship* carrier = s_iter.value();
			Hangar* h = carrier->GetHangar();

			for (int i = 0; i < h->NumSquadrons() && !hangar; i++) {
				if (h->SquadronName(i) == name) {
					hangar = h;
					index = i;
				}
			}
		}
	}

	return hangar;
}

const char* FormatGameTime()
{
	static char TextBuffer[64];

	const int32 TimeMs = Game::GameTime();

	const int32 Hours = (TimeMs / 3600000);
	const int32 Minutes = ((TimeMs - Hours * 3600000) / 60000);
	const int32 Seconds = ((TimeMs - Hours * 3600000 - Minutes * 60000) / 1000);
	const int32 Milliseconds = (TimeMs - Hours * 3600000 - Minutes * 60000 - Seconds * 1000);

	if (Hours > 0)
		sprintf_s(TextBuffer, "%02d:%02d:%02d.%03d", Hours, Minutes, Seconds, Milliseconds);
	else
		sprintf_s(TextBuffer, "%02d:%02d.%03d", Minutes, Seconds, Milliseconds);

	return TextBuffer;
}

