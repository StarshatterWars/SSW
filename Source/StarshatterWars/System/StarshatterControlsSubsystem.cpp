/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         StarshatterControlsSubsystem.cpp
    AUTHOR:       Carlos Bott
*/

#include "StarshatterControlsSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

#include "StarshatterControlsSettings.h"
#include "GameStructs.h"

#include "StarshatterSettingsSaveGame.h"

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterControls, Log, All);

UStarshatterControlsSubsystem* UStarshatterControlsSubsystem::Get(UObject* WorldContextObject)
{
    if (!WorldContextObject)
        return nullptr;

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
        return nullptr;

    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
        return nullptr;

    return GI->GetSubsystem<UStarshatterControlsSubsystem>();
}

void UStarshatterControlsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UStarshatterControlsSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void UStarshatterControlsSubsystem::ApplySettingsToRuntime(UObject* WorldContextObject)
{
    if (!WorldContextObject)
        return;

    UStarshatterControlsSettings* Settings = UStarshatterControlsSettings::Get();
    if (!Settings)
        return;

    Settings->ApplyToRuntimeControls(WorldContextObject);
}

void UStarshatterControlsSubsystem::ApplySettingsToRuntime()
{
    // Use GameInstance as safe context
    UObject* Context = GetGameInstance();
    ApplySettingsToRuntime(Context);
}

void UStarshatterControlsSubsystem::LoadFromSaveGame(UStarshatterSettingsSaveGame* SaveGame)
{
    if (!SaveGame)
    {
        UE_LOG(LogStarshatterControls, Warning, TEXT("LoadFromSaveGame: SaveGame is null"));
        return;
    }

    UStarshatterControlsSettings* Settings = UStarshatterControlsSettings::Get();
    if (!Settings)
        return;

    // Find a struct property of type FStarshatterControlsConfig on the SaveGame object.
    // This avoids hard-coding the member name.
    const UStruct* TargetStruct = FStarshatterControlsConfig::StaticStruct();
    FStarshatterControlsConfig FoundConfig;
    bool bFound = false;

    UClass* SGClass = SaveGame->GetClass();
    if (!SGClass)
        return;

    for (TFieldIterator<FProperty> It(SGClass); It; ++It)
    {
        FProperty* Prop = *It;
        FStructProperty* StructProp = CastField<FStructProperty>(Prop);
        if (!StructProp)
            continue;

        if (StructProp->Struct == TargetStruct)
        {
            void* ValuePtr = StructProp->ContainerPtrToValuePtr<void>(SaveGame);
            if (ValuePtr)
            {
                const FStarshatterControlsConfig* Src = (const FStarshatterControlsConfig*)ValuePtr;
                FoundConfig = *Src;
                bFound = true;

                UE_LOG(LogStarshatterControls, Log, TEXT("LoadFromSaveGame: Imported controls from SaveGame property '%s'"), *StructProp->GetName());
                break;
            }
        }
    }

    if (!bFound)
    {
        UE_LOG(LogStarshatterControls, Warning, TEXT("LoadFromSaveGame: No FStarshatterControlsConfig property found on SaveGame class '%s'"), *SGClass->GetName());
        return;
    }

    // Import into config-backed settings model
    Settings->SetControlsConfig(FoundConfig);
    Settings->Sanitize();
    Settings->Save();
}
