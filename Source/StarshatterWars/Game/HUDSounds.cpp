/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         HUDSounds.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	HUDSounds singleton class utility implementation
*/

#include "HUDSounds.h"

#include "AudioConfig.h"
#include "Sound.h"
#include "DataLoader.h"

#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogHUDSounds, Log, All);

// +--------------------------------------------------------------------+

static USound* mfd_mode = 0;
static USound* nav_mode = 0;
static USound* wep_mode = 0;
static USound* wep_disp = 0;
static USound* hud_mode = 0;
static USound* hud_widget = 0;
static USound* shield_level = 0;
static USound* red_alert = 0;
static USound* tac_accept = 0;
static USound* tac_reject = 0;

// +--------------------------------------------------------------------+

static void LoadInterfaceSound(DataLoader* loader, const char* wave, USound*& s)
{
	if (!loader || !wave || !*wave) {
		UE_LOG(LogHUDSounds, Warning, TEXT("LoadInterfaceSound: Invalid loader or wave name."));
		return;
	}

	loader->LoadSound(wave, s, 0, true);   // optional sound effect

	if (!s) {
		UE_LOG(LogHUDSounds, Verbose, TEXT("Optional HUD sound not found: %s"), ANSI_TO_TCHAR(wave));
	}
}

void
HUDSounds::Initialize()
{
	DataLoader* loader = DataLoader::GetLoader();
	if (!loader) {
		UE_LOG(LogHUDSounds, Error, TEXT("HUDSounds::Initialize: DataLoader is null."));
		return;
	}

	loader->SetDataPath("Sounds/");

	LoadInterfaceSound(loader, "mfd_mode.wav", mfd_mode);
	LoadInterfaceSound(loader, "nav_mode.wav", nav_mode);
	LoadInterfaceSound(loader, "wep_mode.wav", wep_mode);
	LoadInterfaceSound(loader, "wep_disp.wav", wep_disp);
	LoadInterfaceSound(loader, "hud_mode.wav", hud_mode);
	LoadInterfaceSound(loader, "hud_widget.wav", hud_widget);
	LoadInterfaceSound(loader, "shield_level.wav", shield_level);
	LoadInterfaceSound(loader, "alarm.wav", red_alert);
	LoadInterfaceSound(loader, "tac_accept.wav", tac_accept);
	LoadInterfaceSound(loader, "tac_reject.wav", tac_reject);

	if (red_alert) {
		red_alert->SetFlags(USound::AMBIENT | USound::LOOP | USound::LOCKED);
	}

	loader->SetDataPath("");
}

// +--------------------------------------------------------------------+

void
HUDSounds::Close()
{
	delete mfd_mode;      mfd_mode = 0;
	delete nav_mode;      nav_mode = 0;
	delete wep_mode;      wep_mode = 0;
	delete wep_disp;      wep_disp = 0;
	delete hud_mode;      hud_mode = 0;
	delete hud_widget;    hud_widget = 0;
	delete shield_level;  shield_level = 0;
	delete red_alert;     red_alert = 0;
	delete tac_accept;    tac_accept = 0;
	delete tac_reject;    tac_reject = 0;
}

void
HUDSounds::PlaySound(int n)
{
	USound* sound = nullptr;

	switch (n) {
	default:
	case SND_MFD_MODE:      if (mfd_mode)     sound = mfd_mode->Duplicate();      break;
	case SND_NAV_MODE:      if (nav_mode)     sound = nav_mode->Duplicate();      break;
	case SND_WEP_MODE:      if (wep_mode)     sound = wep_mode->Duplicate();      break;
	case SND_WEP_DISP:      if (wep_disp)     sound = wep_disp->Duplicate();      break;
	case SND_HUD_MODE:      if (hud_mode)     sound = hud_mode->Duplicate();      break;
	case SND_HUD_WIDGET:    if (hud_widget)   sound = hud_widget->Duplicate();    break;
	case SND_SHIELD_LEVEL:  if (shield_level) sound = shield_level->Duplicate();  break;
	case SND_TAC_ACCEPT:    if (tac_accept)   sound = tac_accept->Duplicate();    break;
	case SND_TAC_REJECT:    if (tac_reject)   sound = tac_reject->Duplicate();    break;

		// RED ALERT IS A SPECIAL CASE:
	case SND_RED_ALERT:
		if (red_alert) {
			sound = red_alert;
		}
		break;
	}

	if (sound && !sound->IsPlaying()) {
		const int gui_volume = AudioConfig::GuiVolume();
		sound->SetVolume(gui_volume);
		sound->Play();
	}
}

void
HUDSounds::StopSound(int n)
{
	if (n == SND_RED_ALERT && red_alert) {
		red_alert->Stop();
	}
}

