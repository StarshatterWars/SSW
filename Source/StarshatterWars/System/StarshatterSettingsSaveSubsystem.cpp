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
    return LoadInternal(true);
}

bool UStarshatterSettingsSaveSubsystem::Reload()
{
    CachedSettings = nullptr;
    bDirty = false;
    return LoadInternal(true);
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
