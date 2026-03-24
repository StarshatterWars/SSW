/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         StarshatterControlsSettings.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UStarshatterControlsSettings
    - UE-native controls settings model (config-backed).
    - Dialogs read/write ONLY this model.
    - Runtime apply is performed HERE (Audio/Video style):
        ApplyToRuntimeControls(WorldContextObject)

    CONTROL MODEL
    ============
    Uses three Enhanced Input Mapping Contexts (IMC):
    - Arcade
    - FlightSim
    - Hybrid
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

// Canonical enum + struct live here:
#include "GameStructs.h"

#include "StarshatterControlsSettings.generated.h"

class UObject;
class UInputMappingContext;
class UEnhancedInputLocalPlayerSubsystem;

UCLASS(Config = Game, DefaultConfig)
class STARSHATTERWARS_API UStarshatterControlsSettings : public UObject
{
    GENERATED_BODY()

public:
    static UStarshatterControlsSettings* Get();

    void Load();
    void Save() const;
    void Sanitize();

    // Audio/Video style runtime apply entry point:
    void ApplyToRuntimeControls(UObject* WorldContextObject);

public:
    const FStarshatterControlsConfig& GetControlsConfig() const { return Controls; }
    void SetControlsConfig(const FStarshatterControlsConfig& In) { Controls = In; Sanitize(); }

    // Mapping contexts (soft, config-backed)
    TSoftObjectPtr<UInputMappingContext> GetArcadeContext() const { return ArcadeContext; }
    TSoftObjectPtr<UInputMappingContext> GetFlightSimContext() const { return FlightSimContext; }
    TSoftObjectPtr<UInputMappingContext> GetHybridContext() const { return HybridContext; }

    void SetArcadeContext(TSoftObjectPtr<UInputMappingContext> In) { ArcadeContext = In; }
    void SetFlightSimContext(TSoftObjectPtr<UInputMappingContext> In) { FlightSimContext = In; }
    void SetHybridContext(TSoftObjectPtr<UInputMappingContext> In) { HybridContext = In; }

    int32 GetMappingPriority() const { return MappingPriority; }
    void  SetMappingPriority(int32 In) { MappingPriority = In; }

    // Convenience setters (clamped)
    void SetControlModelEnum(EStarshatterControlModel In);

    void SetJoystickIndex(int32 V);
    void SetThrottleAxis(int32 V);
    void SetRudderAxis(int32 V);
    void SetJoystickSensitivity(int32 V);

    void SetMouseSensitivity(int32 V);
    void SetMouseInvert(bool bV);

private:
    void ApplyToEnhancedInput(UObject* WorldContextObject);

    UInputMappingContext* ResolveContextForModel() const;
    void InstallOnlySelectedContext(UEnhancedInputLocalPlayerSubsystem* Subsystem, UInputMappingContext* Selected) const;

private:
    UPROPERTY(Config, EditAnywhere, Category = "Controls")
    FStarshatterControlsConfig Controls;

    UPROPERTY(Config, EditAnywhere, Category = "Controls|EnhancedInput")
    TSoftObjectPtr<UInputMappingContext> ArcadeContext;

    UPROPERTY(Config, EditAnywhere, Category = "Controls|EnhancedInput")
    TSoftObjectPtr<UInputMappingContext> FlightSimContext;

    UPROPERTY(Config, EditAnywhere, Category = "Controls|EnhancedInput")
    TSoftObjectPtr<UInputMappingContext> HybridContext;

    UPROPERTY(Config, EditAnywhere, Category = "Controls|EnhancedInput", meta = (ClampMin = "0", ClampMax = "10"))
    int32 MappingPriority = 1;
};
