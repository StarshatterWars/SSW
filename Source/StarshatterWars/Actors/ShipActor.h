/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         ShipActor.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    ShipActor provides a base Unreal Engine actor for
    rendering ships in the system scene.

    Supports Blueprint-based visual actors for full
    cinematic rendering, with fallback support for
    static meshes.

    Provides common scene points for:
        - focus / bridge / chase camera framing
        - engine exhaust locations
        - thruster locations
        - weapon mount locations
        - turret base locations
        - docking and landing locations
        - optional system marker points
        - navigation light definitions and components

    Most systems are structural only at this stage and
    may not yet have gameplay functionality.
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/PointLightComponent.h"
#include "ShipActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;

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
    bool bBlink = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|NavLights")
    float BlinkInterval = 1.0f;
};

UCLASS()
class STARSHATTERWARS_API AShipActor : public AActor
{
    GENERATED_BODY()

public:
    AShipActor();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

public:

    /*
     * Core hierarchy
     */

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
    USceneComponent* ShipRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
    USceneComponent* VisualRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
    UStaticMeshComponent* HullMesh;

    /*
     * Optional spawned Blueprint visual actor
     */

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship")
    TSubclassOf<AActor> ShipVisualBP;

    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
    AActor* VisualActor;

    /*
     * Camera / cutscene points
     */

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Points")
    USceneComponent* FocusPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Points")
    USceneComponent* BridgePoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Points")
    USceneComponent* ChasePoint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    FVector FocusPointOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    FVector BridgePointOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    FVector ChasePointOffset;

    /*
     * Drive / movement points
     */

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Points")
    USceneComponent* DriveCenterPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Points")
    TArray<USceneComponent*> MainEnginePoints;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Points")
    TArray<USceneComponent*> ThrusterPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    int32 NumMainEnginePoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    int32 NumThrusterPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    float MainEngineSpread;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    float ThrusterSpread;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    float MainEngineX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    float ThrusterX;

    /*
     * Weapons / docking / landing structural points
     */

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Points")
    TArray<USceneComponent*> WeaponMountPoints;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Points")
    TArray<USceneComponent*> TurretBasePoints;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Points")
    TArray<USceneComponent*> DockPoints;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Points")
    TArray<USceneComponent*> LandingPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    int32 NumWeaponMountPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    int32 NumTurretBasePoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    int32 NumDockPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    int32 NumLandingPoints;

    /*
     * Optional system marker points
     */

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Points")
    USceneComponent* QuantumPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Points")
    USceneComponent* ShieldPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Points")
    USceneComponent* SensorPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Points")
    USceneComponent* NavPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Points")
    USceneComponent* ComputerPointA;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Points")
    USceneComponent* ComputerPointB;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Points")
    USceneComponent* ReactorPoint;

    /*
     * Nav lights
     */

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|NavLights")
    TArray<FShipNavLightDef> NavLightDefs;

    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ship|NavLights")
    TArray<UPointLightComponent*> NavLightComponents;

public:

    /*
     * Setup
     */

    UFUNCTION(BlueprintCallable, Category = "Ship")
    void ConfigureForCutscene();

    UFUNCTION(BlueprintCallable, Category = "Ship")
    void SpawnVisualActor();

    UFUNCTION(BlueprintCallable, Category = "Ship")
    void DestroyVisualActor();

    UFUNCTION(BlueprintCallable, Category = "Ship")
    void SetHullMesh(UStaticMesh* Mesh);

    UFUNCTION(BlueprintCallable, Category = "Ship")
    void RebuildAllGeneratedComponents();

    UFUNCTION(BlueprintCallable, Category = "Ship")
    void RebuildMainEnginePoints();

    UFUNCTION(BlueprintCallable, Category = "Ship")
    void RebuildThrusterPoints();

    UFUNCTION(BlueprintCallable, Category = "Ship")
    void RebuildWeaponMountPoints();

    UFUNCTION(BlueprintCallable, Category = "Ship")
    void RebuildTurretBasePoints();

    UFUNCTION(BlueprintCallable, Category = "Ship")
    void RebuildDockPoints();

    UFUNCTION(BlueprintCallable, Category = "Ship")
    void RebuildLandingPoints();

    UFUNCTION(BlueprintCallable, Category = "Ship")
    void RebuildNavLights();

    /*
     * Legacy transform helpers
     */

    UFUNCTION(BlueprintCallable, Category = "Ship|Legacy")
    void ApplyLegacyTransform(const FVector& Loc, const FVector& Rot);

    UFUNCTION(BlueprintPure, Category = "Ship|Legacy")
    static FVector ConvertLegacyLocation(const FVector& V);

    UFUNCTION(BlueprintPure, Category = "Ship|Legacy")
    static FRotator ConvertLegacyRotation(const FVector& V);

protected:

    /*
     * Internal helpers
     */

    void UpdateDerivedPointsFromHull();

    void CreateMainEnginePoints();
    void CreateThrusterPoints();
    void CreateWeaponMountPoints();
    void CreateTurretBasePoints();
    void CreateDockPoints();
    void CreateLandingPoints();

    void ClearSceneComponentArray(TArray<USceneComponent*>& Components);
    void ClearNavLightArray(TArray<UPointLightComponent*>& Components);
};