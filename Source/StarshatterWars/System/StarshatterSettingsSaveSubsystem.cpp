/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe (Unreal Port)
    FILE:         StarshatterSettingsSaveSubsystem.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UStarshatterSettingsSaveSubsystem

    Persistence-only GameInstance subsystem:
      - Loads/creates UStarshatterSettingsSaveGame
      - Runs version migration hooks
      - Caches settings for UI + subsystems
      - Saves on demand and optionally on Deinitialize
*/

#include "StarshatterSettingsSaveSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "StarshatterSettingsSaveGame.h"
#include "StarshatterAudioSettings.h"
#include "StarshatterVideoSettings.h"
#include "StarshatterControlsSettings.h"
#include "StarshatterKeyboardSettings.h"
#include "SSWRuntimeSubsystem.h"
#include "PlayerCharacter.h"
#include "Ship.h"
#include "HUDView.h"
#include "KeyMap.h"
#include "Engine/GameInstance.h"


void UStarshatterSettingsSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (bAutoLoadOnInitialize)
    {
        LoadOrCreateSettings();
    }
}

void UStarshatterSettingsSaveSubsystem::Deinitialize()
{
    if (bSaveOnDeinitialize && bDirty)
    {
        SaveSettings();
    }

    CachedSettings = nullptr;
    Super::Deinitialize();
}

bool UStarshatterSettingsSaveSubsystem::LoadOrCreateSettings()
{
    if (CachedSettings) return true;
    const bool Loaded=LoadInternal(true);
    if (Loaded) RestoreConfigModels();
    return Loaded;
}

bool UStarshatterSettingsSaveSubsystem::Reload()
{
    CachedSettings = nullptr;
    bDirty = false;
    return LoadOrCreateSettings();
}

bool UStarshatterSettingsSaveSubsystem::SaveSettings()
{
    if (!CachedSettings)
        return false;

    // Always sanitize before saving:
    CachedSettings->Sanitize();

    // Keep save version at current:
    CachedSettings->SaveVersion = UStarshatterSettingsSaveGame::CurrentVersion;

    const FString Slot = GetSlotName();
    const int32 UserIndex = GetUserIndex();

    const bool bOK = UGameplayStatics::SaveGameToSlot(CachedSettings, Slot, UserIndex);
    if (bOK)
        bDirty = false;

    return bOK;
}

FString UStarshatterSettingsSaveSubsystem::GetSlotName() const
{
    return UStarshatterSettingsSaveGame::GetDefaultSlotName();
}

int32 UStarshatterSettingsSaveSubsystem::GetUserIndex() const
{
    return UStarshatterSettingsSaveGame::GetDefaultUserIndex();
}

bool UStarshatterSettingsSaveSubsystem::LoadInternal(bool bCreateIfMissing)
{
    const FString Slot = GetSlotName();
    const int32 UserIndex = GetUserIndex();

    // Migrate the previous settings slot only when options.sav does not exist.
    // Keep the old file so a failed migration never destroys the user's settings.
    if (!UGameplayStatics::DoesSaveGameExist(Slot,UserIndex) &&
        UGameplayStatics::DoesSaveGameExist(TEXT("StarshatterSettings"),UserIndex))
    {
        CachedSettings=Cast<UStarshatterSettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("StarshatterSettings"),UserIndex));
        if (CachedSettings) { MigrateAndSanitize(); bDirty=true; return SaveSettings(); }
    }

    // -----------------------------
    // Load existing if present
    // -----------------------------
    if (UGameplayStatics::DoesSaveGameExist(Slot, UserIndex))
    {
        USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(Slot, UserIndex);
        CachedSettings = Cast<UStarshatterSettingsSaveGame>(Loaded);

        if (CachedSettings)
        {
            MigrateAndSanitize();
            return true;
        }

        // Wrong class / cast failed:
        CachedSettings = nullptr;
    }

    if (!bCreateIfMissing)
        return false;

    // -----------------------------
    // Create new defaults (auto slot creation)
    // -----------------------------
    CachedSettings = Cast<UStarshatterSettingsSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UStarshatterSettingsSaveGame::StaticClass())
    );

    if (!CachedSettings)
        return false;

    // Seed a new slot from the existing config/profile, preserving prior preferences.
    UStarshatterAudioSettings::Get()->Load();
    UStarshatterVideoSettings::Get()->Load();
    UStarshatterControlsSettings::Get()->Load();
    UStarshatterKeyboardSettings::Get()->Load();
    CaptureCurrentOptions();
    // Force version + sanitize:
    CachedSettings->SaveVersion = UStarshatterSettingsSaveGame::CurrentVersion;
    CachedSettings->Sanitize();

    bDirty = true;

    // Create slot immediately (you asked for automatic slot creation):
    SaveSettings();

    return true;
}

void UStarshatterSettingsSaveSubsystem::MigrateAndSanitize()
{
    if (!CachedSettings)
        return;

    // If the save is from an older version, migrate it:
    const bool bMigrated = CachedSettings->MigrateToCurrentVersion();

    // Always sanitize after migration:
    CachedSettings->Sanitize();

    // If migration changed anything, mark dirty (and optionally autosave):
    if (bMigrated)
        bDirty = true;
}

namespace
{
    const int32 JoystickActions[] = { KEY_AXIS_YAW, KEY_AXIS_PITCH, KEY_AXIS_ROLL, KEY_AXIS_THROTTLE,
        KEY_AXIS_YAW_INVERT, KEY_AXIS_PITCH_INVERT, KEY_AXIS_ROLL_INVERT, KEY_AXIS_THROTTLE_INVERT };
}
void UStarshatterSettingsSaveSubsystem::CaptureCurrentOptions()
{
    if (!CachedSettings) return;
    auto* Audio=UStarshatterAudioSettings::Get();
    CachedSettings->Audio.MasterVolume=Audio->GetMasterVolume();
    CachedSettings->Audio.MusicVolume=Audio->GetMusicVolume();
    CachedSettings->Audio.EffectsVolume=Audio->GetEffectsVolume();
    CachedSettings->Audio.VoiceVolume=Audio->GetVoiceVolume();
    CachedSettings->Audio.SoundQuality=Audio->GetSoundQuality();
    CachedSettings->Video=UStarshatterVideoSettings::Get()->GetConfig();
    CachedSettings->Controls=UStarshatterControlsSettings::Get()->GetControlsConfig();
    CachedSettings->KeyboardConfig=UStarshatterKeyboardSettings::Get()->GetKeyboardConfig();
    if (PlayerCharacter* Player=PlayerCharacter::GetCurrentPlayer())
    {
        auto& Game=CachedSettings->GameOptions;
        Game.FlightModel=Player->FlightModel(); Game.LandingModel=Player->LandingModel();
        Game.FlyingStart=Player->FlyingStart(); Game.AILevel=Player->AILevel();
        Game.HUDMode=Player->HUDMode(); Game.HUDColor=Player->HUDColor();
        Game.FriendlyFire=Player->FriendlyFire(); Game.GridMode=Player->GridMode(); Game.Gunsight=Player->Gunsight();
        CachedSettings->bHasGameOptions=true;
    }
    if (GetGameInstance())
        if (auto* Runtime=GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>())
        {
            KeyMap& Mapping=Runtime->GetKeyMap();
            for (int32 Action : JoystickActions)
            {
                const int32 Index=Mapping.FindMapIndex(Action);
                if (Index>=0 && Index<Mapping.GetNumKeys()) CachedSettings->JoystickBindings.Add(Action,Mapping.GetKeyMap(Index)->key);
            }
        }
    bDirty=true;
}
void UStarshatterSettingsSaveSubsystem::RestoreConfigModels()
{
    if (!CachedSettings) return;
    auto* Audio=UStarshatterAudioSettings::Get();
    Audio->SetMasterVolume(CachedSettings->Audio.MasterVolume);Audio->SetMusicVolume(CachedSettings->Audio.MusicVolume);
    Audio->SetEffectsVolume(CachedSettings->Audio.EffectsVolume);Audio->SetVoiceVolume(CachedSettings->Audio.VoiceVolume);Audio->SetSoundQuality(CachedSettings->Audio.SoundQuality);Audio->Save();
    auto* Video=UStarshatterVideoSettings::Get();Video->SetConfig(CachedSettings->Video);Video->Save();
    auto* Controls=UStarshatterControlsSettings::Get();Controls->SetControlsConfig(CachedSettings->Controls);Controls->Save();
    auto* Keyboard=UStarshatterKeyboardSettings::Get();Keyboard->SetKeyboardConfig(CachedSettings->KeyboardConfig);Keyboard->Save();
}
void UStarshatterSettingsSaveSubsystem::RestoreGameOptions()
{
    if (!CachedSettings || !CachedSettings->bHasGameOptions) return;
    const auto& Game=CachedSettings->GameOptions;
    if (PlayerCharacter* Player=PlayerCharacter::GetCurrentPlayer())
    {
        Player->SetFlightModel(Game.FlightModel);Player->SetLandingModel(Game.LandingModel);
        Player->SetFlyingStart(Game.FlyingStart);Player->SetAILevel(Game.AILevel);
        Player->SetHUDMode(Game.HUDMode);Player->SetHUDColor(Game.HUDColor);
        Player->SetFriendlyFire(Game.FriendlyFire);Player->SetGridMode(Game.GridMode);Player->SetGunsight(Game.Gunsight);
    }
    Ship::SetFlightModel(Game.FlightModel);Ship::SetLandingModel(Game.LandingModel);
    Ship::SetFriendlyFireLevel(Game.FriendlyFire/4.f);
    HUDView::SetArcade(Game.HUDMode>0);HUDView::SetDefaultColorSet(Game.HUDColor);
}
void UStarshatterSettingsSaveSubsystem::RestoreJoystickBindings(KeyMap& Mapping)
{
    if (!CachedSettings) return;
    for (int32 Action : JoystickActions)
        if (const int32* Value=CachedSettings->JoystickBindings.Find(Action)) Mapping.Bind(Action,*Value,0);
}
