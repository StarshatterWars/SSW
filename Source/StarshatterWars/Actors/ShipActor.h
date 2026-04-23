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

    This version uses editable data arrays with fallback
    generated components. Blueprint children can edit the
    data arrays directly or disable auto rebuild and manage
    authoring manually.

    Nav lights are handled by UNavLightComponent and are
    advanced by the owning ship tick.
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NavLightComponent.h"
#include "ShipActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;

USTRUCT(BlueprintType)
struct FShipPointDef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    FVector LocalOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    FRotator LocalRotation = FRotator::ZeroRotator;
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
    USceneComponent* PointRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
    USceneComponent* LightRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
    UStaticMeshComponent* HullMesh;

    /*
     * Optional spawned Blueprint visual actor
     */

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Visual")
    TSubclassOf<AActor> ShipVisualBP;

    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Visual")
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
     * Fixed system marker points
     */

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Points")
    USceneComponent* DriveCenterPoint;

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
     * Build control
     */

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Build")
    bool bAutoRebuildGeneratedComponents;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Build")
    bool bRebuildOnConstruction;

    /*
     * Legacy fallback generation controls
     */

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Generated")
    int32 NumMainEnginePoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Generated")
    int32 NumThrusterPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Generated")
    int32 NumWeaponMountPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Generated")
    int32 NumTurretBasePoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Generated")
    int32 NumDockPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Generated")
    int32 NumLandingPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Generated")
    float MainEngineSpread;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Generated")
    float ThrusterSpread;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Generated")
    float MainEngineX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Generated")
    float ThrusterX;

    /*
     * Editable point definition arrays
     * If these arrays are populated, they take priority over
     * the count/spread fallback generation logic.
     */

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Generated")
    TArray<FShipPointDef> MainEnginePointDefs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Generated")
    TArray<FShipPointDef> ThrusterPointDefs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Generated")
    TArray<FShipPointDef> WeaponMountPointDefs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Generated")
    TArray<FShipPointDef> TurretBasePointDefs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Generated")
    TArray<FShipPointDef> DockPointDefs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Generated")
    TArray<FShipPointDef> LandingPointDefs;

    /*
     * Generated component arrays
     */

    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Generated")
    TArray<USceneComponent*> MainEnginePoints;

    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Generated")
    TArray<USceneComponent*> ThrusterPoints;

    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Generated")
    TArray<USceneComponent*> WeaponMountPoints;

    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Generated")
    TArray<USceneComponent*> TurretBasePoints;

    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Generated")
    TArray<USceneComponent*> DockPoints;

    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ship|Generated")
    TArray<USceneComponent*> LandingPoints;

    /*
     * Main engine emitters
     */

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|FX")
    bool bEnableMainEngineEmitters = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|FX")
    TObjectPtr<UNiagaraSystem> MainEngineEmitterSystem = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|FX")
    FVector MainEngineEmitterRelativeScale = FVector(1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|FX")
    FRotator MainEngineEmitterRelativeRotation = FRotator::ZeroRotator;

    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ship|FX")
    TArray<TObjectPtr<UNiagaraComponent>> MainEngineEmitters;


    /*
    * Thruster emitters
    */

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|FX")
    bool bEnableThrusterEmitters = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|FX")
    TObjectPtr<UNiagaraSystem> ThrusterEmitterSystem = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|FX")
    FVector ThrusterEmitterRelativeScale = FVector(1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|FX")
    FRotator ThrusterEmitterRelativeRotation = FRotator::ZeroRotator;

    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ship|FX")
    TArray<TObjectPtr<UNiagaraComponent>> ThrusterEmitters;
    
    /*
     * Nav lights
     */

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|NavLights")
    TArray<FShipNavLightDef> NavLightDefs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|NavLights")
    bool bEnableNavLights = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|NavLights")
    float NavLightIntensityMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|NavLights")
    float NavLightRadiusMultiplier = 1.0f;

    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Ship|NavLights")
    TArray<UNavLightComponent*> NavLights;\

    UPROPERTY(Transient)
    TArray<TObjectPtr<UPointLightComponent>> MainEngineLights;

    UPROPERTY(Transient)
    float NavLightSequenceTimer = 0.0f;

    UPROPERTY(Transient)
    int32 NavLightSequenceIndex = 0;

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

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Ship|Build")
    void RebuildAllGeneratedComponents();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Ship|Build")
    void RebuildMainEnginePoints();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Ship|Build")
    void RebuildThrusterPoints();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Ship|Build")
    void RebuildWeaponMountPoints();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Ship|Build")
    void RebuildTurretBasePoints();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Ship|Build")
    void RebuildDockPoints();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Ship|Build")
    void RebuildLandingPoints();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Ship|FX")
    void RebuildMainEngineEmitters();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Ship|NavLights")
    void RefreshNavLights();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Ship|NavLights")
    void RebuildNavLights();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Ship|Build")
    void RebuildThrusterEmitters();



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
    void UpdateNavLights(float DeltaTime);

    void CreateMainEnginePoints();
    void CreateThrusterPoints();
    void CreateWeaponMountPoints();
    void CreateTurretBasePoints();
    void CreateDockPoints();
    void CreateLandingPoints();

    USceneComponent* CreateGeneratedPoint(
        const FString& BaseName,
        int32 Index,
        const FVector& LocalOffset,
        const FRotator& LocalRotation);

    void ClearSceneComponentArray(TArray<USceneComponent*>& Components);
    void ClearNavLightArray(TArray<UNavLightComponent*>& Components);
};