#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NavLightComponent.h"

#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"
#include "ShipActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UPointLightComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class Ship;
class Drive;

USTRUCT(BlueprintType)
struct FShipPointDef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    FVector LocalOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    FRotator LocalRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    FName PointName = NAME_None;
};

//-------------------------------------------------------------
// Runtime Thruster FX
//-------------------------------------------------------------

USTRUCT()
struct FRuntimeThrusterFX
{
    GENERATED_BODY()

    UPROPERTY()
    FName PointName = NAME_None;

    UPROPERTY()
    EThrusterPortDir Direction = EThrusterPortDir::AFT;

    UPROPERTY()
    UNiagaraComponent* Flare = nullptr;

    UPROPERTY()
    UNiagaraComponent* Trail = nullptr;

    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;

    float PortScale = 1.0f;
    float AudioMultiplier = 1.0f;
};

UCLASS()
class STARSHATTERWARS_API AShipActor : public AActor
{
    GENERATED_BODY()

public:
    AShipActor();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

    void BindRuntimeShip(Ship* InShip);
    void UpdateFromRuntimeShip(float DeltaTime);

    void BuildNavLightsFromRuntime();
    bool HasRuntimeShip() const { return RuntimeShip != nullptr; }


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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
    TObjectPtr<USceneComponent> VFXRoot;

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

    FVector InitialRuntimeLocationLegacy = FVector::ZeroVector;
    FVector InitialActorLocationUE = FVector::ZeroVector;
    
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    FVector DriveCenterPointOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    FVector QuantumPointOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    FVector ShieldPointOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    FVector SensorPointOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    FVector NavPointOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    FVector ComputerPointAOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    FVector ComputerPointBOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Points")
    FVector ReactorPointOffset;

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
    
    UPROPERTY(Transient)
    float MainEngineVisualPower = 0.0f;

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
    TArray<UNavLightComponent*> NavLights;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|NavLights")
    UStaticMesh* NavLightBulbMesh = nullptr;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UStaticMeshComponent>> NavLightBulbMeshes;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UPointLightComponent>> NavLightPointLights;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UPointLightComponent>> MainEngineLights;

    UPROPERTY(Transient)
    float NavLightSequenceTimer = 0.0f;

    UPROPERTY(Transient)
    int32 NavLightSequenceIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|NavLights")
    UMaterialInterface* NavLightBulbMaterial = nullptr;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> NavLightBulbMIDs;


    /*
     * Engine Audio
     */

    UPROPERTY(EditAnywhere, Category = "Ship|Main Engines")
    float MainEngineIntensityScale = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Ship|Engine Audio")
    TObjectPtr<USoundCue> EngineSoundCue;

    UPROPERTY(EditAnywhere, Category = "Ship|Engine Audio")
    TObjectPtr<USoundCue> BurnerSoundCue;

    UPROPERTY(EditAnywhere, Category = "Ship|Engine Audio")
    TObjectPtr<USoundCue> RumbleSoundCue;

    UPROPERTY(Transient)
    TArray<TObjectPtr<USceneComponent>> RuntimeMainEnginePoints;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UNiagaraComponent>> RuntimeMainEngineEmitters;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> EngineAudioComponent;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> BurnerAudioComponent;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> RumbleAudioComponent;

    void BuildMainEnginesFromRuntime();
    void ClearRuntimeMainEngines();
    void UpdateMainEnginesFromRuntime(float DeltaTime);

    void CreateEngineAudioComponents();
    void UpdateEngineAudioFromRuntime(float DeltaTime);
    
    //-------------------------------------------------------------
    // Runtime Thruster Niagara
    //-------------------------------------------------------------

    UPROPERTY(EditAnywhere, Category = "Thrusters")
    UNiagaraSystem* ThrusterFlareSystem = nullptr;

    UPROPERTY(EditAnywhere, Category = "Thrusters")
    UNiagaraSystem* ThrusterTrailSystem = nullptr;

    UPROPERTY()
    TArray<FRuntimeThrusterFX> RuntimeThrusterFX;

    void BuildThrustersFromRuntime();
    void ClearRuntimeThrusters();
    void UpdateThrusterVFXFromRuntime();

    UPROPERTY()
    TArray<TObjectPtr<UNiagaraComponent>> RuntimeThrusterEmitters;

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
     * Accessors
     */

    UFUNCTION(BlueprintPure, Category = "Ship|Points")
    USceneComponent* GetMainEnginePoint(int32 Index) const;

    UFUNCTION(BlueprintPure, Category = "Ship|Points")
    USceneComponent* GetThrusterPoint(int32 Index) const;

    UFUNCTION(BlueprintPure, Category = "Ship|Points")
    USceneComponent* GetWeaponMountPoint(int32 Index) const;

    UFUNCTION(BlueprintPure, Category = "Ship|Points")
    USceneComponent* GetTurretBasePoint(int32 Index) const;

    UFUNCTION(BlueprintPure, Category = "Ship|Points")
    USceneComponent* GetDockPoint(int32 Index) const;

    UFUNCTION(BlueprintPure, Category = "Ship|Points")
    USceneComponent* GetLandingPoint(int32 Index) const;

    UFUNCTION(BlueprintPure, Category = "Ship|Points")
    USceneComponent* FindWeaponMountPointByName(FName PointName) const;

    UFUNCTION(BlueprintPure, Category = "Ship|Points")
    USceneComponent* FindTurretBasePointByName(FName PointName) const;

 public:
    UFUNCTION(BlueprintCallable, Category = "Ship|FX")
    void SetMainEnginesActive(bool bActive);

    UFUNCTION(BlueprintPure, Category = "Ship|FX")
    bool AreMainEnginesActive() const;

    UFUNCTION(BlueprintCallable, Category = "Ship|FX")
    void SetThrustersActive(bool bActive);

    UFUNCTION(BlueprintPure, Category = "Ship|FX")
    bool AreThrustersActive() const;

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
    void ClearNavLightVisuals();

protected:
    void ClearRuntimeNavLights();
    void AddRuntimeNavLightComponent(const FShipNavLightDef& Def);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|FX")
    bool bMainEnginesActive = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|FX")
    bool bThrustersActive = false;

    public:
        UFUNCTION(BlueprintCallable, Category = "Ship|Cutscene")
        void SetCutsceneNavMovement(
            const FVector& InStartLegacy,
            const FVector& InTargetLegacy,
            float InSpeed);

        UFUNCTION(BlueprintCallable, Category = "Ship|Cutscene")
        void SetCutsceneLocalMovement(
            const FVector& InStartLocal,
            const FVector& InTargetLocal,
            float InSpeed);

        UFUNCTION(BlueprintCallable, Category = "Ship|Legacy")
        FVector ConvertLegacyRegionLocToUELocal(const FVector& LegacyLoc) const;

private:
    void UpdateCutsceneNavMovement(float DeltaTime);

private:
    UPROPERTY(Transient)
    bool bUseCutsceneNavMovement = false;

    UPROPERTY(Transient)
    FVector CutsceneStartLocal = FVector::ZeroVector;

    UPROPERTY(Transient)
    FVector CutsceneTargetLocal = FVector::ZeroVector;

    UPROPERTY(Transient)
    float CutsceneMoveSpeed = 0.0f;

    protected:
        UPROPERTY(Transient)
        bool bUseRuntimeShipTransform = true;

private:
    Ship* RuntimeShip = nullptr;

    protected:
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Runtime")
        float RuntimeLocationInterpSpeed = 8.0f;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Runtime")
        float RuntimeRotationInterpSpeed = 6.0f;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Runtime")
        float RuntimeVelocityVisibleThreshold = 5.0f;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Runtime")
        bool bRuntimeUseVelocityForYaw = true;

private:
    FVector LastRuntimeLocation = FVector::ZeroVector;
    FVector LastRuntimeVelocity = FVector::ZeroVector;
    bool bHasRuntimeTransform = false;
};
