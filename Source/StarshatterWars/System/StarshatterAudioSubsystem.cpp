/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe (Unreal Port)
    FILE:         StarshatterAudioSubsystem.cpp
    AUTHOR:       Carlos Bott
*/

#include "StarshatterAudioSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

#include "StarshatterAudioSettings.h"

// Unified settings save:
#include "StarshatterSettingsSaveGame.h"
#include "GameStructs.h"

void UStarshatterAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // Boot orchestrated by StarshatterBootSubsystem (intentionally not auto-booting here)
}

void UStarshatterAudioSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void UStarshatterAudioSubsystem::Boot()
{
    LoadAudioConfig();
    ApplySettingsToRuntime();
}

UStarshatterAudioSubsystem* UStarshatterAudioSubsystem::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
        return nullptr;

    const UWorld* World = WorldContextObject->GetWorld();
    if (!World)
        return nullptr;

    UGameInstance* GI = World->GetGameInstance();
    return Get(GI);
}

UStarshatterAudioSubsystem* UStarshatterAudioSubsystem::Get(UGameInstance* GameInstance)
{
    return GameInstance ? GameInstance->GetSubsystem<UStarshatterAudioSubsystem>() : nullptr;
}

UStarshatterAudioSettings* UStarshatterAudioSubsystem::GetSettings() const
{
    return UStarshatterAudioSettings::Get();
}

void UStarshatterAudioSubsystem::LoadAudioConfig()
{
    if (UStarshatterAudioSettings* Settings = GetSettings())
    {
        Settings->Load();   // your CDO helper: ReloadConfig + Sanitize
    }
}

void UStarshatterAudioSubsystem::SaveAudioConfig()
{
    if (UStarshatterAudioSettings* Settings = GetSettings())
    {
        Settings->Sanitize();
        Settings->Save();   // your CDO helper: SaveConfig
    }
}

void UStarshatterAudioSubsystem::ApplySettingsToRuntime()
{
    if (UStarshatterAudioSettings* Settings = GetSettings())
    {
        UObject* WorldContext = GetGameInstance();
        if (!WorldContext)
            WorldContext = this;

        Settings->ApplyToRuntimeAudio(WorldContext);
    }
}

// ---------------------------------------------------------------------
// SaveGame bridging
// ---------------------------------------------------------------------

void UStarshatterAudioSubsystem::LoadFromSaveGame(const UStarshatterSettingsSaveGame* SaveGame)
{
    if (!SaveGame)
        return;

    UStarshatterAudioSettings* Settings = GetSettings();
    if (!Settings)
        return;

    // Copy SaveGame struct -> Settings CDO:
    const FStarshatterAudioConfig& A = SaveGame->Audio;

    Settings->SetMasterVolume(A.MasterVolume);
    Settings->SetMusicVolume(A.MusicVolume);
    Settings->SetEffectsVolume(A.EffectsVolume);
    Settings->SetVoiceVolume(A.VoiceVolume);
    Settings->SetSoundQuality(A.SoundQuality);

    Settings->Sanitize();
}

void UStarshatterAudioSubsystem::WriteToSaveGame(UStarshatterSettingsSaveGame* SaveGame) const
{
    if (!SaveGame)
        return;

    const UStarshatterAudioSettings* Settings = GetSettings();
    if (!Settings)
        return;

    FStarshatterAudioConfig& A = SaveGame->Audio;

    A.MasterVolume = Settings->GetMasterVolume();
    A.MusicVolume = Settings->GetMusicVolume();
    A.EffectsVolume = Settings->GetEffectsVolume();
    A.VoiceVolume = Settings->GetVoiceVolume();
    A.SoundQuality = Settings->GetSoundQuality();
}
