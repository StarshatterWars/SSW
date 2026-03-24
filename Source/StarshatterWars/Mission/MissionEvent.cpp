/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         MissionEvent.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR: John DiCamillo
	ORIGINAL STUDIO: Destroyer Studios LLC

	OVERVIEW
	========
	Events for mission scripting
*/

#include "MissionEvent.h"

// NOTE: This file is legacy Starshatter core code being ported.
// Keep includes as-is unless they cause Unreal build issues; remove unsupported debug headers.
#include "Mission.h"
#include "StarSystem.h"
#include "Galaxy.h"
#include "Starshatter.h"
#include "StarServer.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "SimElement.h"
#include "DisplayView.h"
#include "HUDView.h"
#include "Instruction.h"
#include "QuantumDrive.h"
#include "Sim.h"
#include "AudioConfig.h"
#include "CameraManager.h"
#include "RadioMessage.h"
#include "RadioTraffic.h"
#include "Weapon.h"
#include "WeaponGroup.h"
#include "PlayerCharacter.h"
#include "Campaign.h"
#include "CombatGroup.h"

#include "Game.h"
#include "DataLoader.h"
#include "SystemFont.h"
#include "FontManager.h"
#include "Sound.h"
#include "ParseUtil.h"
#include "FormatUtil.h"
#include "Random.h"
#include "Video.h"
#include "GameStructs.h"

// Minimal Unreal logging support:
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterWarsMissionEvent, Log, All);

const char* FormatGameTime();

// +--------------------------------------------------------------------+

MissionEvent::MissionEvent()
	: id(0),
	status(INSTRUCTION_STATUS::PENDING),
	time(0),
	delay(0),
	event(0),
	event_nparams(0),
	event_chance(100),
	trigger(0),
	trigger_nparams(0),
	sound(0)
{
	ZeroMemory(event_param, sizeof(event_param));
	ZeroMemory(trigger_param, sizeof(trigger_param));
}

MissionEvent::~MissionEvent()
{
	if (sound) {
		sound->Stop();
		sound->Release();
	}
}

// +--------------------------------------------------------------------+

void
MissionEvent::ExecFrame(double seconds)
{
	Sim* sim = Sim::GetSim();

	if (!sim) {
		status = INSTRUCTION_STATUS::PENDING;
		return;
	}

	if (status == INSTRUCTION_STATUS::PENDING)
		CheckTrigger();

	if (status == INSTRUCTION_STATUS::ACTIVE) {
		if (delay > 0)
			delay -= seconds;
		else
			Execute();
	}
}

// +--------------------------------------------------------------------+

void
MissionEvent::Activate()
{
	if (status == INSTRUCTION_STATUS::PENDING) {
		if (event_chance > 0 && event_chance < 100) {
			const int32 Roll = FMath::RandRange(0, 100);
			if (Roll < event_chance)
				status = INSTRUCTION_STATUS::ACTIVE;
			else
				status = INSTRUCTION_STATUS::SKIPPED;
		}
		else {
			status = INSTRUCTION_STATUS::ACTIVE;
		}

		if (status == INSTRUCTION_STATUS::SKIPPED) {
			Sim::GetSim()->ProcessEventTrigger(TRIGGER_SKIPPED, id);
		}
	}
}

void
MissionEvent::Skip()
{
	if (status == INSTRUCTION_STATUS::PENDING) {
		status = INSTRUCTION_STATUS::SKIPPED;
	}
}

// +--------------------------------------------------------------------+

bool
MissionEvent::CheckTrigger()
{
	Sim* sim = Sim::GetSim();

	if (time > 0 && time > sim->MissionClock())
		return false;

	switch (trigger) {
	case TRIGGER_TIME: {
		if (time <= sim->MissionClock())
			Activate();
	}
					 break;

	case TRIGGER_DAMAGE: {
		Ship* ship = sim->FindShip(trigger_ship);
		if (ship) {
			double damage = 100.0 * (ship->Design()->integrity - ship->Integrity()) /
				(ship->Design()->integrity);

			if (damage >= trigger_param[0])
				Activate();
		}
	}
					   break;

	case TRIGGER_DETECT: {
		Ship* ship = sim->FindShip(trigger_ship);
		Ship* tgt = sim->FindShip(trigger_target);

		if (ship && tgt) {
			if (ship->FindContact(tgt))
				Activate();
		}
		else {
			Skip();
		}
	}
					   break;

	case TRIGGER_RANGE: {
		Ship* ship = sim->FindShip(trigger_ship);
		Ship* tgt = sim->FindShip(trigger_target);

		if (ship && tgt) {
			double range = (ship->Location() - tgt->Location()).Length();
			double min_range = 0;
			double max_range = 1e12;

			if (trigger_param[0] > 0)
				min_range = trigger_param[0];
			else
				max_range = -trigger_param[0];

			if (range < min_range || range > max_range)
				Activate();
		}
		else {
			Skip();
		}
	}
					  break;

	case TRIGGER_SHIPS_LEFT: {
		int alive = 0;
		int count = 0;
		int iff = -1;
		int nparams = NumTriggerParams();

		if (nparams > 0) count = TriggerParam(0);
		if (nparams > 1) iff = TriggerParam(1);

		ListIter<SimRegion> iter = sim->GetRegions();
		while (++iter) {
			SimRegion* rgn = iter.value();

			ListIter<Ship> s_iter = rgn->GetShips();
			while (++s_iter) {
				Ship* ship = s_iter.value();

				if (ship->Type() >= (int)CLASSIFICATION::STATION)
					continue;

				if (ship->Life() == 0 && ship->RespawnCount() < 1)
					continue;

				if (iff < 0 || ship->GetIFF() == iff)
					alive++;
			}
		}

		if (alive <= count)
			Activate();
	}
						   break;

	case TRIGGER_EVENT_ALL: {
		bool  all = true;
		int   nparams = NumTriggerParams();
		for (int i = 0; all && i < nparams; i++) {
			int trigger_id = TriggerParam(i);

			ListIter<MissionEvent> iter = sim->GetEvents();
			while (++iter) {
				MissionEvent* e = iter.value();
				if (e->EventID() == trigger_id) {
					if (e->GetStatus() != INSTRUCTION_STATUS::COMPLETE)
						all = false;
					break;
				}
				else if (e->EventID() == -trigger_id) {
					if (e->GetStatus() == INSTRUCTION_STATUS::COMPLETE)
						all = false;
					break;
				}
			}
		}

		if (all)
			Activate();
	}
						  break;

	case TRIGGER_EVENT_ANY: {
		bool  any = false;
		int   nparams = NumTriggerParams();
		for (int i = 0; !any && i < nparams; i++) {
			int trigger_id = TriggerParam(i);

			ListIter<MissionEvent> iter = sim->GetEvents();
			while (++iter) {
				MissionEvent* e = iter.value();
				if (e->EventID() == trigger_id) {
					if (e->GetStatus() == INSTRUCTION_STATUS::COMPLETE)
						any = true;
					break;
				}
			}
		}

		if (any)
			Activate();
	}
						  break;
	}

	return status == INSTRUCTION_STATUS::ACTIVE;
}

// +--------------------------------------------------------------------+

void
MissionEvent::Execute(bool silent)
{
	Starshatter* stars = Starshatter::GetInstance();
	HUDView* hud = HUDView::GetInstance();
	Sim* sim = Sim::GetSim();
	Ship* player = sim->GetPlayerShip();
	Ship* ship = 0;
	Ship* src = 0;
	Ship* tgt = 0;
	SimElement* elem = 0;
	int            pan = 0;
	bool           end_mission = false;

	if (event_ship.length())
		ship = sim->FindShip(event_ship);
	else
		ship = player;

	if (event_source.length())
		src = sim->FindShip(event_source);

	if (event_target.length())
		tgt = sim->FindShip(event_target);

	if (ship)
		elem = ship->GetElement();
	else if (event_ship.length()) {
		elem = sim->FindElement(event_ship);

		if (elem)
			ship = elem->GetShip(1);
	}

	// expire the delay, if any remains
	delay = 0;

	// fire the event action
	switch (event) {
	case MESSAGE:
		if (event_message.length() > 0) {
			if (ship) {
				const auto Action = static_cast<RadioMessageAction>(event_param[0]);
				RadioMessage* msg = new RadioMessage(ship, src, Action);

				msg->SetInfo(event_message);
				msg->SetChannel(ship->GetIFF());
				if (tgt)
					msg->AddTarget(tgt);
				RadioTraffic::Transmit(msg);
			}
			else if (elem) {
				const auto Action = static_cast<RadioMessageAction>(event_param[0]);
				RadioMessage* msg = new RadioMessage(ship, src, Action);

				msg->SetInfo(event_message);
				msg->SetChannel(elem->GetIFF());
				if (tgt)
					msg->AddTarget(tgt);
				RadioTraffic::Transmit(msg);
			}
		}

		if (event_sound.length() > 0) {
			pan = event_param[0];
		}
		break;

	case OBJECTIVE:
	{
		if (!elem)
			break;

		// Clear existing objectives if requested
		if (event_param[0]) {
			elem->ClearInstructions();
			elem->ClearObjectives();
		}

		// Create new objective instruction
		Instruction* obj = new Instruction(
			static_cast<INSTRUCTION_ACTION>(event_param[0]),
			nullptr
		);

		// Set target name (UTF-8 safe)
		if (!event_target) {
			obj->SetTarget(UTF8_TO_TCHAR(event_target.data()));
		}

		elem->AddObjective(obj);

		// If this affects the player, update HUD
		if (elem->Contains(player)) {
			if (HUDView* hud_local = HUDView::GetInstance()) {
				hud_local->ShowHUDInst();
			}
		}
	}
	break;

	case INSTRUCTION:
		if (elem) {
			if (event_param[0])
				elem->ClearInstructions();

			elem->AddInstruction(event_message);

			if (elem->Contains(player) && event_message.length() > 0) {
				HUDView* hud_local = HUDView::GetInstance();
				if (hud_local)
					hud_local->ShowHUDInst();
			}
		}
		break;

	case IFF:
		if (elem) {
			elem->SetIFF(event_param[0]);
		}
		else if (ship) {
			ship->SetIFF(event_param[0]);
		}
		break;

	case DAMAGE:
		if (ship) {
			ship->InflictDamage(event_param[0]);

			if (ship->Integrity() < 1) {
				//NetUtil::SendObjKill(ship, 0, NetObjKill::KILL_MISC);
				ship->DeathSpiral();

				UE_LOG(LogStarshatterWarsMissionEvent, Log,
					TEXT("Ship '%hs' killed by scripted event %d (%hs)"),
					(const char*)ship->Name(), id, FormatGameTime());
			}
		}
		else {
			UE_LOG(LogStarshatterWarsMissionEvent, Warning,
				TEXT("EVENT %d: Could not apply damage to ship '%hs' (not found)."),
				id, (const char*)event_ship);
		}
		break;

	case JUMP:
		if (ship) {
			SimRegion* rgn = sim->FindRegion(event_target);

			if (rgn && ship->GetRegion() != rgn) {
				if (rgn->IsOrbital()) {
					QuantumDrive* quantum_drive = ship->GetQuantumDrive();
					if (quantum_drive) {
						quantum_drive->SetDestination(rgn, Point(0, 0, 0));
						quantum_drive->Engage(true); // request immediate jump
					}
					else if (ship->IsAirborne()) {
						ship->MakeOrbit();
					}
				}
				else {
					ship->DropOrbit();
				}
			}
		}
		break;

	case HOLD:
		if (elem)
			elem->SetHoldTime(event_param[0]);
		break;

	case SKIP: {
		for (int i = 0; i < event_nparams; i++) {
			int skip_id = event_param[i];

			ListIter<MissionEvent> iter = sim->GetEvents();
			while (++iter) {
				MissionEvent* e = iter.value();
				if (e->EventID() == skip_id) {
					if (e->status != INSTRUCTION_STATUS::COMPLETE)
						e->status = INSTRUCTION_STATUS::SKIPPED;
				}
			}
		}
	}
			 break;

	case END_MISSION:
		UE_LOG(LogStarshatterWarsMissionEvent, Log,
			TEXT("END MISSION by scripted event %d (%hs)"),
			id, FormatGameTime());
		end_mission = true;
		break;

		//
		// NOTE: CUTSCENE EVENTS DO NOT APPLY IN MULTIPLAYER
		//
	case BEGIN_SCENE:
		UE_LOG(LogStarshatterWarsMissionEvent, Log, TEXT("------------------------------------"));
		UE_LOG(LogStarshatterWarsMissionEvent, Log, TEXT("Begin Cutscene '%hs'"), event_message.data());
		stars->BeginCutscene();
		break;

	case END_SCENE:
		UE_LOG(LogStarshatterWarsMissionEvent, Log, TEXT("End Cutscene '%hs'"), event_message.data());
		UE_LOG(LogStarshatterWarsMissionEvent, Log, TEXT("------------------------------------"));
		stars->EndCutscene();
		break;

	case CAMERA:
		if (stars->InCutscene()) {
			CameraManager* cam_dir = CameraManager::GetInstance();

			if (!cam_dir->GetShip())
				cam_dir->SetShip(player);

			switch (event_param[0]) {
			case 1:
				if (cam_dir->GetMode() != CameraManager::MODE_COCKPIT)
					cam_dir->SetMode(CameraManager::MODE_COCKPIT, event_rect.x);
				break;

			case 2:
				if (cam_dir->GetMode() != CameraManager::MODE_CHASE)
					cam_dir->SetMode(CameraManager::MODE_CHASE, event_rect.x);
				break;

			case 3:
				if (cam_dir->GetMode() != CameraManager::MODE_ORBIT)
					cam_dir->SetMode(CameraManager::MODE_ORBIT, event_rect.x);
				break;

			case 4:
				if (cam_dir->GetMode() != CameraManager::MODE_TARGET)
					cam_dir->SetMode(CameraManager::MODE_TARGET, event_rect.x);
				break;
			}

			if (event_target.length()) {
				UE_LOG(LogStarshatterWarsMissionEvent, Verbose,
					TEXT("Mission Event %d: setting camera target to %hs"),
					id, (const char*)event_target);

				Ship* s_tgt = 0;

				if (event_target.indexOf("body:") < 0)
					s_tgt = sim->FindShip(event_target);

				if (s_tgt) {
					UE_LOG(LogStarshatterWarsMissionEvent, Verbose,
						TEXT("   found ship %hs"), s_tgt->Name());

					cam_dir->SetViewOrbital(0);

					if (cam_dir->GetViewObject() != s_tgt) {

						if (event_param[0] == 6) {
							s_tgt->DropCam(event_param[1], event_param[2]);
							cam_dir->SetShip(s_tgt);
							cam_dir->SetMode(CameraManager::MODE_DROP, 0);
						}
						else {
							Ship* cam_ship = cam_dir->GetShip();

							if (cam_ship && cam_ship->IsDropCam()) {
								cam_ship->CompleteTransition();
							}

							if (cam_dir->GetShip() != sim->GetPlayerShip())
								cam_dir->SetShip(sim->GetPlayerShip());
							cam_dir->SetViewObject(s_tgt, true); // immediate, no transition
						}
					}
				}
				else {
					const char* body_name = event_target.data();

					if (!strncmp(body_name, "body:", 5))
						body_name += 5;

					Orbital* orb = sim->FindOrbitalBody(body_name);

					if (orb) {
						UE_LOG(LogStarshatterWarsMissionEvent, Verbose,
							TEXT("   found body %hs"), orb->Name());
						cam_dir->SetViewOrbital(orb);
					}
				}
			}

			if (event_param[0] == 3) {
				cam_dir->SetOrbitPoint(event_point.X, event_point.Y, event_point.Z);
			}
			else if (event_param[0] == 5) {
				cam_dir->SetOrbitRates(event_point.X, event_point.Y, event_point.Z);
			}
		}
		break;

	case VOLUME:
		if (stars->InCutscene()) {
			AudioConfig* audio_cfg = AudioConfig::GetInstance();

			audio_cfg->SetEfxVolume(event_param[0]);
			audio_cfg->SetWrnVolume(event_param[0]);
		}
		break;

	case DISPLAY:
		if (stars->InCutscene()) {
			DisplayView* disp_view = DisplayView::GetInstance();

			if (disp_view) {
				FColor color;
				if (disp_view) {
					color = FColor(event_param[0]);
				}

				if (event_message.length() && event_source.length()) {

					if (event_message.contains('$')) {
						Campaign* campaign = Campaign::GetCampaign();
						PlayerCharacter* user = PlayerCharacter::GetCurrentPlayer();
						CombatGroup* group = campaign->GetPlayerGroup();

						if (user) {
							event_message = FormatTextReplace(
								event_message,
								"$NAME",
								TCHAR_TO_UTF8(*user->Name())
							);
							event_message = FormatTextReplace(event_message, "$RANK", PlayerCharacter::RankName(user->GetRank()));
						}

						if (group) {
							event_message = FormatTextReplace(event_message, "$GROUP", group->GetDescription());
						}

						if (event_message.contains("$TIME")) {
							char timestr[32];
							FormatDayTime(timestr, campaign->GetTime(), true);
							event_message = FormatTextReplace(event_message, "$TIME", timestr);
						}
					}

					disp_view->AddText(event_message,
						FontManager::Find(event_source),
						color,
						event_rect,
						event_point.Y,
						event_point.X,
						event_point.Z);

				}
				else if (event_target.length()) {
					DataLoader* loader = DataLoader::GetLoader();

					// Legacy bitmap loading path retained; image now is a UTexture2D* in the header.
					// The DataLoader interface must be ported accordingly elsewhere.
					if (loader) {
						loader->SetDataPath(0);
						loader->LoadGameBitmap(event_target, *image, 0, true);
					}

					if (image) {
						disp_view->AddImage(image,
							color,
							Video::BLEND_ALPHA,
							event_rect,
							event_point.Y,
							event_point.X,
							event_point.Z);
					}
				}
			}
		}
		break;

	case FIRE_WEAPON:
		if (ship) {
			// fire single weapon:
			if (event_param[0] >= 0) {
				ship->FireWeapon(event_param[0]);
			}
			// fire all weapons:
			else {
				ListIter<WeaponGroup> g_iter = ship->GetWeapons();
				while (++g_iter) {
					ListIter<Weapon> w_iter = g_iter->GetWeapons();
					while (++w_iter) {
						Weapon* w = w_iter.value();
						w->Fire();
					}
				}
			}
		}
		break;

	default:
		break;
	}

	sim->ProcessEventTrigger(TRIGGER_EVENT, id);

	if (!silent && !sound && event_sound.length()) {
		DataLoader* loader = DataLoader::GetLoader();
		bool        use_fs = loader->IsFileSystemEnabled();
		DWORD       flags = pan ? USound::LOCKED | USound::LOCALIZED :
			USound::LOCKED | USound::AMBIENT;

		loader->UseFileSystem(true);
		loader->SetDataPath("Sounds/");
		loader->LoadSound(event_sound, sound, flags);
		loader->SetDataPath(0);

		if (!sound) {
			loader->SetDataPath("Mods/Sounds/");
			loader->LoadSound(event_sound, sound, flags);
			loader->SetDataPath(0);
		}

		if (!sound) {
			loader->LoadSound(event_sound, sound, flags);
		}

		loader->UseFileSystem(use_fs);

		// fire and forget:
		if (sound) {
			if (sound->GetFlags() & USound::STREAMED) {
				sound->SetFlags(flags | sound->GetFlags());
				sound->SetVolume(AudioConfig::VoxVolume());
				sound->Play();
			}
			else {
				sound->SetFlags(flags);
				sound->SetVolume(AudioConfig::VoxVolume());
				sound->SetPan(pan);
				sound->SetFilename(event_sound);
				sound->AddToSoundCard();
				sound->Play();
			}
		}
	}

	status = INSTRUCTION_STATUS::COMPLETE;

	if (end_mission) {
		StarServer* server = StarServer::GetInstance();

		if (stars) {
			stars->EndMission();
		}
		else if (server) {
			// end mission event uses event_target member
			// to forward server to next mission in the chain:
			if (event_target.length())
				server->SetNextMission(event_target);

			server->SetGameMode(EGameMode::MENU);
		}
	}
}

// +--------------------------------------------------------------------+

Text
MissionEvent::TriggerParamStr() const
{
	Text result;
	char buffer[8];

	if (trigger_param[0] == 0) {
		// nothing
	}
	else if (trigger_param[1] == 0) {
		sprintf_s(buffer, "%d", trigger_param[0]);
		result = buffer;
	}
	else {
		result = "(";

		for (int i = 0; i < 8; i++) {
			if (trigger_param[i] == 0)
				break;

			if (i < 7 && trigger_param[i + 1] != 0)
				sprintf_s(buffer, "%d, ", trigger_param[i]);
			else
				sprintf_s(buffer, "%d", trigger_param[i]);

			result += buffer;
		}

		result += ")";
	}

	return result;
}

// +--------------------------------------------------------------------+

int
MissionEvent::EventParam(int index) const
{
	if (index >= 0 && index < NumEventParams())
		return event_param[index];

	return 0;
}

int
MissionEvent::NumEventParams() const
{
	return event_nparams;
}

// +--------------------------------------------------------------------+

int
MissionEvent::TriggerParam(int index) const
{
	if (index >= 0 && index < NumTriggerParams())
		return trigger_param[index];

	return 0;
}

int
MissionEvent::NumTriggerParams() const
{
	return trigger_nparams;
}

// +--------------------------------------------------------------------+

static const char* event_names[] = {
	"Message",
	"Objective",
	"Instruction",
	"IFF",
	"Damage",
	"Jump",
	"Hold",
	"Skip",
	"Exit",

	"BeginScene",
	"Camera",
	"Volume",
	"Display",
	"Fire",
	"EndScene"
};

static const char* trigger_names[] = {
	"Time",
	"Damage",
	"Destroyed",
	"Jump",
	"Launch",
	"Dock",
	"Navpoint",
	"Event",
	"Skipped",
	"Target",
	"Ships Left",
	"Detect",
	"Range",
	"Event (ALL)",
	"Event (ANY)"
};

const char*
MissionEvent::EventName() const
{
	return event_names[event];
}

const char*
MissionEvent::EventName(int n)
{
	return event_names[n];
}

int
MissionEvent::EventForName(const char* n)
{
	for (int i = 0; i < NUM_EVENTS; i++)
		if (!_stricmp(n, event_names[i]))
			return i;

	return 0;
}

const char*
MissionEvent::TriggerName() const
{
	return trigger_names[trigger];
}

const char*
MissionEvent::TriggerName(int n)
{
	return trigger_names[n];
}

int
MissionEvent::TriggerForName(const char* n)
{
	for (int i = 0; i < NUM_TRIGGERS; i++)
		if (!_stricmp(n, trigger_names[i]))
			return i;

	return 0;
}
