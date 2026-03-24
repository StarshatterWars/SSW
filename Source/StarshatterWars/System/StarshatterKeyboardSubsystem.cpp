/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         StarshatterKeyboardSubsystem.cpp
    AUTHOR:       Carlos Bott
*/

#include "StarshatterKeyboardSubsystem.h"

#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

#include "StarshatterKeyboardSettings.h"
#include "StarshatterSettingsSaveGame.h" // MUST define KeyboardConfig inside this SaveGame

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterKeyboardSubsystem, Log, All);

UStarshatterKeyboardSubsystem* UStarshatterKeyboardSubsystem::Get(UObject* WorldContextObject)
{
    if (!WorldContextObject)
        return nullptr;

    UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject);
    if (!GI)
        return nullptr;

    return GI->GetSubsystem<UStarshatterKeyboardSubsystem>();
}

void UStarshatterKeyboardSubsystem::LoadFromSaveGame(const UStarshatterSettingsSaveGame* SaveGame)
{
    if (!SaveGame)
        return;

    UStarshatterKeyboardSettings* Settings = UStarshatterKeyboardSettings::Get();
    if (!Settings)
        return;

    // IMPORTANT:
    // This requires UStarshatterSettingsSaveGame to contain:
    // UPROPERTY() FStarshatterKeyboardConfig KeyboardConfig;
    Settings->Load();
    Settings->SetKeyboardConfig(SaveGame->KeyboardConfig);
    Settings->Sanitize();
    Settings->Save();

    UE_LOG(LogStarshatterKeyboardSubsystem, Log, TEXT("Loaded keyboard config from SaveGame"));
}

void UStarshatterKeyboardSubsystem::ApplySettingsToRuntime(UObject* WorldContextObject)
{
    UStarshatterKeyboardSettings* Settings = UStarshatterKeyboardSettings::Get();
    if (!Settings)
        return;

    Settings->ApplyToRuntimeKeyboard(WorldContextObject);
}

void UStarshatterKeyboardSubsystem::SaveToSaveGame(UStarshatterSettingsSaveGame* SaveGame) const
{
    if (!SaveGame)
        return;

    const UStarshatterKeyboardSettings* Settings = UStarshatterKeyboardSettings::Get();
    if (!Settings)
        return;

    SaveGame->KeyboardConfig = Settings->GetKeyboardConfig();
}
