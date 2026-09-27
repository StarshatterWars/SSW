/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         NavLightComponent.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    NavLightComponent provides a lightweight Unreal
    point light component for ship running lights.

    This component does not tick itself.

    The owning ship is responsible for:
        - creating the component
        - applying authored definitions
        - advancing blink / sequence state
        - optional visible emitter meshes

    This design avoids recursive visibility and nested
    component lifecycle problems.
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/PointLightComponent.h"
#include "GameStructs_System.h"
#include "NavLightComponent.generated.h"

USTRUCT(BlueprintType)
struct FShipNavLightDef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|NavLights")
    FVector LocalOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|NavLights")
    FRotator LocalRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|NavLights")
    FLinearColor Color = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|NavLights")
    float Intensity = 3000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|NavLights")
    float Radius = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|NavLights")
    EShipNavLightMode Mode = EShipNavLightMode::Steady;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|NavLights")
    float BlinkInterval = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|NavLights")
    float PhaseOffset = 0.0f;
};

UCLASS(ClassGroup = (Ship), meta = (BlueprintSpawnableComponent))
class STARSHATTERWARS_API UNavLightComponent : public UPointLightComponent
{
    GENERATED_BODY()

public:
    UNavLightComponent();

    UFUNCTION(BlueprintCallable, Category = "Ship|NavLights")
    void ApplyDefinition(const FShipNavLightDef& InDef);

    UFUNCTION(BlueprintCallable, Category = "Ship|NavLights")
    void SetGlobalMultipliers(float InIntensityMultiplier, float InRadiusMultiplier);

    UFUNCTION(BlueprintCallable, Category = "Ship|NavLights")
    void AdvanceLight(float DeltaTime, bool bSequenceActive);

    UFUNCTION(BlueprintCallable, Category = "Ship|NavLights")
    void ResetRuntime();

    UFUNCTION(BlueprintCallable, Category = "Ship|NavLights")
    void ForceVisible(bool bInVisible);

    UFUNCTION(BlueprintPure, Category = "Ship|NavLights")
    const FShipNavLightDef& GetDefinition() const { return LightDef; }

    UFUNCTION(BlueprintPure, Category = "Ship|NavLights")
    bool IsRenderedVisible() const { return bRenderedVisible; }

    void SetRenderedFromLegacy(bool bInVisible);
protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|NavLights")
    FShipNavLightDef LightDef;

    UPROPERTY(Transient)
    float IntensityMultiplier;

    UPROPERTY(Transient)
    float RadiusMultiplier;

    UPROPERTY(Transient)
    float TimeAccumulator;

    UPROPERTY(Transient)
    bool bRenderedVisible;

protected:
    void ApplyVisualSettings();
    void ApplyRenderedVisibility(bool bVisible);
};