/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         ShipActor.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Implementation of ShipActor with Blueprint visual
    support and common ship attachment points.

    Most components are structural placeholders for now
    and may not yet drive gameplay systems.
*/

#include "ShipActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"

#include "UObject/ConstructorHelpers.h"

#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"

#include "Drive.h"
#include "Ship.h"
#include "Thruster.h"

#include "Engine/StaticMesh.h"
#include "Engine/World.h"

#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "GameStructs_System.h"

#include "NavLight.h"
#include "ShipUtils.h"


template<typename T>
static void ClearComponentArray(TArray<TObjectPtr<T>>& Components)
{
    for (TObjectPtr<T>& Comp : Components)
    {
        if (Comp)
        {
            Comp->DestroyComponent();
        }
    }

    Components.Empty();
}

template<typename T>
static void ClearComponentArray(TArray<T*>& Components)
{
    for (T* Comp : Components)
    {
        if (Comp)
        {
            Comp->DestroyComponent();
        }
    }

    Components.Empty();
}

static void ClearPointLights(TArray<TObjectPtr<UPointLightComponent>>& Lights)
{
    for (TObjectPtr<UPointLightComponent>& L : Lights)
    {
        if (L)
        {
            L->DestroyComponent();
        }
    }
    Lights.Empty();
}


AShipActor::AShipActor()
{
    PrimaryActorTick.bCanEverTick = true;


    static ConstructorHelpers::FObjectFinder<USoundCue> EngineCueObj(
        TEXT("/Game/Audio/Sounds/SC_Engine.SC_Engine"));

    if (EngineCueObj.Succeeded())
    {
        EngineSoundCue = EngineCueObj.Object;
    }

    static ConstructorHelpers::FObjectFinder<USoundCue> BurnerCueObj(
        TEXT("/Game/Audio/Sounds/SC_Burner.SC_Burner"));

    if (BurnerCueObj.Succeeded())
    {
        BurnerSoundCue = BurnerCueObj.Object;
    }

    static ConstructorHelpers::FObjectFinder<USoundCue> RumbleCueObj(
        TEXT("/Game/Audio/Sounds/SC_Ramble.SC_Ramble"));

    if (RumbleCueObj.Succeeded())
    {
        RumbleSoundCue = RumbleCueObj.Object;
    }

    ShipRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ShipRoot"));
    SetRootComponent(ShipRoot);

    VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
    VisualRoot->SetupAttachment(ShipRoot);

    PointRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PointRoot"));
    PointRoot->SetupAttachment(ShipRoot);

    LightRoot = CreateDefaultSubobject<USceneComponent>(TEXT("LightRoot"));
    LightRoot->SetupAttachment(ShipRoot);

    HullMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HullMesh"));
    HullMesh->SetupAttachment(VisualRoot);
    HullMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HullMesh->SetGenerateOverlapEvents(false);
    HullMesh->SetMobility(EComponentMobility::Movable);

    FocusPoint = CreateDefaultSubobject<USceneComponent>(TEXT("FocusPoint"));
    FocusPoint->SetupAttachment(PointRoot);

    BridgePoint = CreateDefaultSubobject<USceneComponent>(TEXT("BridgePoint"));
    BridgePoint->SetupAttachment(PointRoot);

    ChasePoint = CreateDefaultSubobject<USceneComponent>(TEXT("ChasePoint"));
    ChasePoint->SetupAttachment(PointRoot);

    DriveCenterPoint = CreateDefaultSubobject<USceneComponent>(TEXT("DriveCenterPoint"));
    DriveCenterPoint->SetupAttachment(PointRoot);

    QuantumPoint = CreateDefaultSubobject<USceneComponent>(TEXT("QuantumPoint"));
    QuantumPoint->SetupAttachment(PointRoot);

    ShieldPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ShieldPoint"));
    ShieldPoint->SetupAttachment(PointRoot);

    SensorPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SensorPoint"));
    SensorPoint->SetupAttachment(PointRoot);

    NavPoint = CreateDefaultSubobject<USceneComponent>(TEXT("NavPoint"));
    NavPoint->SetupAttachment(PointRoot);

    ComputerPointA = CreateDefaultSubobject<USceneComponent>(TEXT("ComputerPointA"));
    ComputerPointA->SetupAttachment(PointRoot);

    ComputerPointB = CreateDefaultSubobject<USceneComponent>(TEXT("ComputerPointB"));
    ComputerPointB->SetupAttachment(PointRoot);

    ReactorPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ReactorPoint"));
    ReactorPoint->SetupAttachment(PointRoot);

    VisualActor = nullptr;

    FocusPointOffset = FVector(0.0f, 0.0f, 50.0f);
    BridgePointOffset = FVector(0.0f, 200.0f, 30.0f);
    ChasePointOffset = FVector(0.0f, -1000.0f, 200.0f);

    DriveCenterPointOffset = FVector::ZeroVector;
    QuantumPointOffset = FVector::ZeroVector;
    ShieldPointOffset = FVector::ZeroVector;
    SensorPointOffset = FVector::ZeroVector;
    NavPointOffset = FVector::ZeroVector;
    ComputerPointAOffset = FVector::ZeroVector;
    ComputerPointBOffset = FVector::ZeroVector;
    ReactorPointOffset = FVector::ZeroVector;

    NumMainEnginePoints = 2;
    NumThrusterPoints = 2;
    NumWeaponMountPoints = 0;
    NumTurretBasePoints = 0;
    NumDockPoints = 0;
    NumLandingPoints = 0;

    MainEngineSpread = 120.0f;
    ThrusterSpread = 80.0f;

    MainEngineX = -250.0f;
    ThrusterX = -180.0f;

    bAutoRebuildGeneratedComponents = true;
    bRebuildOnConstruction = true;

    bEnableNavLights = true;
    NavLightIntensityMultiplier = 1.0f;
    NavLightRadiusMultiplier = 1.0f;

    FocusPoint->SetRelativeLocation(FocusPointOffset);
    BridgePoint->SetRelativeLocation(BridgePointOffset);
    ChasePoint->SetRelativeLocation(ChasePointOffset);

    DriveCenterPoint->SetRelativeLocation(DriveCenterPointOffset);
    QuantumPoint->SetRelativeLocation(QuantumPointOffset);
    ShieldPoint->SetRelativeLocation(ShieldPointOffset);
    SensorPoint->SetRelativeLocation(SensorPointOffset);
    NavPoint->SetRelativeLocation(NavPointOffset);
    ComputerPointA->SetRelativeLocation(ComputerPointAOffset);
    ComputerPointB->SetRelativeLocation(ComputerPointBOffset);
    ReactorPoint->SetRelativeLocation(ReactorPointOffset);

    /*
     * Default fallback nav lights.
     * These are generic and can be overridden per ship.
     */
    {
        FShipNavLightDef Port;
        Port.LocalOffset = FVector(0.0f, -100.0f, 20.0f);
        Port.LocalRotation = FRotator::ZeroRotator;
        Port.Color = FLinearColor::Red;
        Port.Intensity = 3000.0f;
        Port.Radius = 300.0f;
        Port.Mode = EShipNavLightMode::Steady;
        Port.BlinkInterval = 1.0f;
        Port.PhaseOffset = 0.0f;
        NavLightDefs.Add(Port);

        FShipNavLightDef Starboard;
        Starboard.LocalOffset = FVector(0.0f, 100.0f, 20.0f);
        Starboard.LocalRotation = FRotator::ZeroRotator;
        Starboard.Color = FLinearColor::Green;
        Starboard.Intensity = 3000.0f;
        Starboard.Radius = 300.0f;
        Starboard.Mode = EShipNavLightMode::Steady;
        Starboard.BlinkInterval = 1.0f;
        Starboard.PhaseOffset = 0.0f;
        NavLightDefs.Add(Starboard);

        FShipNavLightDef Dorsal;
        Dorsal.LocalOffset = FVector(-40.0f, 0.0f, 60.0f);
        Dorsal.LocalRotation = FRotator::ZeroRotator;
        Dorsal.Color = FLinearColor::White;
        Dorsal.Intensity = 2500.0f;
        Dorsal.Radius = 250.0f;
        Dorsal.Mode = EShipNavLightMode::Steady;
        Dorsal.BlinkInterval = 1.0f;
        Dorsal.PhaseOffset = 0.0f;
        NavLightDefs.Add(Dorsal);

        FShipNavLightDef Ventral;
        Ventral.LocalOffset = FVector(-40.0f, 0.0f, -60.0f);
        Ventral.LocalRotation = FRotator::ZeroRotator;
        Ventral.Color = FLinearColor(0.6f, 0.6f, 1.0f);
        Ventral.Intensity = 2000.0f;
        Ventral.Radius = 250.0f;
        Ventral.Mode = EShipNavLightMode::Steady;
        Ventral.BlinkInterval = 1.0f;
        Ventral.PhaseOffset = 0.0f;
        NavLightDefs.Add(Ventral);
    }

    SetThrustersActive(false);
}

void AShipActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    UpdateDerivedPointsFromHull();

    if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
    {
        return;
    }

    if (bAutoRebuildGeneratedComponents && bRebuildOnConstruction)
    {
        RebuildMainEnginePoints();
        RebuildMainEngineEmitters();

        RebuildThrusterPoints();
        RebuildThrusterEmitters();

        RebuildWeaponMountPoints();
        RebuildTurretBasePoints();
        RebuildDockPoints();
        RebuildLandingPoints();

        if (!RuntimeShip)
        {
            RebuildNavLights();
        }
    }
}

void AShipActor::BeginPlay()
{
    Super::BeginPlay();
}

void AShipActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearRuntimeThrusters();
    ClearRuntimeMainEngines();

    Super::EndPlay(EndPlayReason);
}

void AShipActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (RuntimeShip)
    {
        RuntimeShip->ExecFrame(DeltaTime);

        UpdateFromRuntimeShip(DeltaTime);
        UpdateNavLights(DeltaTime);
        UpdateMainEnginesFromRuntime(DeltaTime);
        UpdateEngineAudioFromRuntime(DeltaTime);
        UpdateThrustersFromRuntime(DeltaTime);
        return;
    }

    UpdateNavLights(DeltaTime);

    if (bUseCutsceneNavMovement)
    {
        UpdateCutsceneNavMovement(DeltaTime);
        return;
    }
}

void AShipActor::ConfigureForCutscene()
{
    SpawnVisualActor();

    if (VisualActor && HullMesh)
    {
        HullMesh->SetVisibility(false);
    }
    else if (HullMesh)
    {
        HullMesh->SetVisibility(true);
        HullMesh->SetCastShadow(true);
    }
}

void AShipActor::SpawnVisualActor()
{
    if (VisualActor)
    {
        return;
    }

    if (!ShipVisualBP)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = GetInstigator();

    VisualActor = World->SpawnActor<AActor>(
        ShipVisualBP,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParams);

    if (VisualActor)
    {
        VisualActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
        VisualActor->SetActorRelativeLocation(FVector::ZeroVector);
        VisualActor->SetActorRelativeRotation(FRotator::ZeroRotator);
        VisualActor->SetActorRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
    }
}

void AShipActor::DestroyVisualActor()
{
    if (VisualActor)
    {
        VisualActor->Destroy();
        VisualActor = nullptr;
    }
}

void AShipActor::SetHullMesh(UStaticMesh* Mesh)
{
    if (HullMesh)
    {
        HullMesh->SetStaticMesh(Mesh);
        UpdateDerivedPointsFromHull();
    }
}

void AShipActor::RebuildAllGeneratedComponents()
{
    RebuildMainEnginePoints();
    RebuildMainEngineEmitters();

    RebuildThrusterPoints();
    RebuildThrusterEmitters();

    RebuildWeaponMountPoints();
    RebuildTurretBasePoints();
    RebuildDockPoints();
    RebuildLandingPoints();

    if (RuntimeShip)
    {
        BuildNavLightsFromRuntime();
    }
    else
    {
        RebuildNavLights();
    }
}

void AShipActor::RebuildMainEnginePoints()
{
    ClearSceneComponentArray(MainEnginePoints);
    CreateMainEnginePoints();
}

void AShipActor::RebuildMainEngineEmitters()
{
    UE_LOG(LogTemp, Warning,
        TEXT("[ShipActor] RebuildMainEngineEmitters: BEGIN Ship='%s' Points=%d"),
        *GetName(),
        MainEnginePoints.Num());

    // ----------------------------------------------------
    // Clear existing
    // ----------------------------------------------------

    ClearComponentArray(MainEngineEmitters);
    ClearPointLights(MainEngineLights);

    // ----------------------------------------------------
    // Early out
    // ----------------------------------------------------

    if (!bEnableMainEngineEmitters || !MainEngineEmitterSystem)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[ShipActor] RebuildMainEngineEmitters: SKIPPED (disabled or no system)"));
        return;
    }

    // ----------------------------------------------------
    // Spawn emitters
    // ----------------------------------------------------

    for (int32 Index = 0; Index < MainEnginePoints.Num(); ++Index)
    {
        USceneComponent* Point = MainEnginePoints[Index];
        if (!Point)
        {
            continue;
        }

        const FString Name = FString::Printf(TEXT("MainEngineEmitter_%d"), Index);

        UNiagaraComponent* Emitter =
            NewObject<UNiagaraComponent>(this, *Name);

        if (!Emitter)
        {
            continue;
        }

        Emitter->SetupAttachment(Point);
        Emitter->RegisterComponent();

        Emitter->SetAsset(MainEngineEmitterSystem);
        Emitter->SetRelativeLocation(FVector::ZeroVector);
        Emitter->SetRelativeRotation(MainEngineEmitterRelativeRotation);
        Emitter->SetRelativeScale3D(MainEngineEmitterRelativeScale);

        // ----------------------------------------------------
        // Engine ON/OFF control
        // ----------------------------------------------------

        Emitter->SetAutoActivate(bMainEnginesActive);

        if (bMainEnginesActive)
        {
            Emitter->Activate(true);
        }
        else
        {
            Emitter->Deactivate();
        }

        MainEngineEmitters.Add(Emitter);

        // ----------------------------------------------------
        // Engine glow light
        // ----------------------------------------------------

        const FString LightName = FString::Printf(TEXT("MainEngineLight_%d"), Index);

        UPointLightComponent* Light =
            NewObject<UPointLightComponent>(this, *LightName);

        if (Light)
        {
            Light->SetupAttachment(Point);
            Light->RegisterComponent();

            Light->SetRelativeLocation(FVector::ZeroVector);

            Light->SetLightColor(FLinearColor(0.6f, 0.7f, 1.0f)); // bluish engine glow

            Light->SetIntensity(1500.0f);
            Light->SetAttenuationRadius(300.0f);

            Light->SetCastShadows(false);
            Light->SetUseInverseSquaredFalloff(true);
            Light->SetMobility(EComponentMobility::Movable);

            Light->SetVisibility(bMainEnginesActive);
            Light->SetHiddenInGame(!bMainEnginesActive);

            MainEngineLights.Add(Light);
        }
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[ShipActor] RebuildMainEngineEmitters: END Created=%d"),
        MainEngineEmitters.Num());
}

void AShipActor::RebuildThrusterPoints()
{
    ClearComponentArray(ThrusterPoints);

    for (int32 Index = 0; Index < ThrusterPointDefs.Num(); ++Index)
    {
        const FShipPointDef& PointDef = ThrusterPointDefs[Index];

        const FString Name = FString::Printf(TEXT("ThrusterPoint_%d"), Index);

        USceneComponent* PointComp = NewObject<USceneComponent>(this, *Name);
        if (!PointComp)
        {
            continue;
        }

        PointComp->SetupAttachment(RootComponent);
        PointComp->RegisterComponent();
        PointComp->SetRelativeLocation(PointDef.LocalOffset);
        PointComp->SetRelativeRotation(PointDef.LocalRotation);

        ThrusterPoints.Add(PointComp);
    }
}

void AShipActor::RebuildWeaponMountPoints()
{
    ClearSceneComponentArray(WeaponMountPoints);
    CreateWeaponMountPoints();
}

void AShipActor::RebuildTurretBasePoints()
{
    ClearSceneComponentArray(TurretBasePoints);
    CreateTurretBasePoints();
}

void AShipActor::RebuildDockPoints()
{
    ClearSceneComponentArray(DockPoints);
    CreateDockPoints();
}

void AShipActor::RebuildLandingPoints()
{
    ClearSceneComponentArray(LandingPoints);
    CreateLandingPoints();
}

void AShipActor::RefreshNavLights()
{
    RebuildNavLights();
}

void AShipActor::RebuildNavLights()
{
    UE_LOG(LogTemp, Warning,
        TEXT("[ShipActor] RebuildNavLights: BEGIN Ship='%s' DefCount=%d"),
        *GetName(),
        NavLightDefs.Num());

    // ----------------------------------------------------
    // Clear existing
    // ----------------------------------------------------

    ClearNavLightArray(NavLights);
    ClearNavLightVisuals();

    // ----------------------------------------------------
    // Early out
    // ----------------------------------------------------

    if (!bEnableNavLights || !LightRoot || NavLightDefs.Num() == 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[ShipActor] RebuildNavLights: SKIPPED"));
        return;
    }

    // ----------------------------------------------------
    // Build lights
    // ----------------------------------------------------

    for (int32 Index = 0; Index < NavLightDefs.Num(); ++Index)
    {
        const FShipNavLightDef& Def = NavLightDefs[Index];

        const FString Name = FString::Printf(TEXT("NavLight_%d"), Index);

        // ----------------------------------------------------
        // LOGIC COMPONENT
        // ----------------------------------------------------

        UNavLightComponent* NavLight =
            NewObject<UNavLightComponent>(this, *Name);

        if (!NavLight)
        {
            continue;
        }

        NavLight->SetupAttachment(LightRoot);
        NavLight->RegisterComponent();

        NavLight->ApplyDefinition(Def);
        NavLight->SetGlobalMultipliers(
            NavLightIntensityMultiplier,
            NavLightRadiusMultiplier);

        NavLights.Add(NavLight);

        // ----------------------------------------------------
        // BULB MESH + MATERIAL (FIXED SCALE)
        // ----------------------------------------------------

        if (NavLightBulbMesh)
        {
            const FString BulbName = FString::Printf(TEXT("NavLightBulb_%d"), Index);

            UStaticMeshComponent* Bulb =
                NewObject<UStaticMeshComponent>(this, *BulbName);

            if (Bulb)
            {
                Bulb->SetupAttachment(LightRoot);
                Bulb->RegisterComponent();

                Bulb->SetStaticMesh(NavLightBulbMesh);
                Bulb->SetRelativeLocation(Def.LocalOffset);
                Bulb->SetRelativeRotation(Def.LocalRotation);

                Bulb->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                Bulb->SetGenerateOverlapEvents(false);
                Bulb->SetMobility(EComponentMobility::Movable);

                Bulb->SetRelativeScale3D(FVector(0.02f));

                // ---- MATERIAL ----

                if (NavLightBulbMaterial)
                {
                    UMaterialInstanceDynamic* MID =
                        UMaterialInstanceDynamic::Create(NavLightBulbMaterial, this);

                    if (MID)
                    {
                        MID->SetVectorParameterValue(TEXT("GlowColor"), Def.Color);

                         MID->SetScalarParameterValue(
                            TEXT("GlowIntensity"),
                            FMath::Max(1.0f, Def.Intensity * 0.002f));

                        Bulb->SetMaterial(0, MID);
                        NavLightBulbMIDs.Add(MID);
                    }
                    else
                    {
                        NavLightBulbMIDs.Add(nullptr);
                    }
                }
                else
                {
                    NavLightBulbMIDs.Add(nullptr);
                }

                NavLightBulbMeshes.Add(Bulb);
            }
        }

        // ----------------------------------------------------
        // POINT LIGHT (CLAMPED)
        // ----------------------------------------------------

        const FString PointLightName = FString::Printf(TEXT("NavLightPoint_%d"), Index);

        UPointLightComponent* PointLight =
            NewObject<UPointLightComponent>(this, *PointLightName);

        if (PointLight)
        {
            PointLight->SetupAttachment(LightRoot);
            PointLight->RegisterComponent();

            PointLight->SetRelativeLocation(Def.LocalOffset);
            PointLight->SetRelativeRotation(Def.LocalRotation);

            PointLight->SetLightColor(Def.Color);

            PointLight->SetIntensity(
                FMath::Clamp(Def.Intensity * NavLightIntensityMultiplier, 50.0f, 500.0f));

            PointLight->SetAttenuationRadius(
                FMath::Clamp(Def.Radius * NavLightRadiusMultiplier, 10.0f, 80.0f));

            PointLight->SetCastShadows(false);
            PointLight->SetUseInverseSquaredFalloff(true);
            PointLight->SetMobility(EComponentMobility::Movable);

            NavLightPointLights.Add(PointLight);
        }
    }

    // ----------------------------------------------------
    // Reset sequence system
    // ----------------------------------------------------

    NavLightSequenceTimer = 0.0f;
    NavLightSequenceIndex = 0;

    UE_LOG(LogTemp, Warning,
        TEXT("[ShipActor] RebuildNavLights: END Created=%d"),
        NavLights.Num());
}
void AShipActor::ApplyLegacyTransform(const FVector& Loc, const FVector& Rot)
{
    SetActorLocation(ConvertLegacyLocation(Loc));
    SetActorRotation(ConvertLegacyRotation(Rot));
}

FVector AShipActor::ConvertLegacyLocation(const FVector& V)
{
    return FVector(V.X, V.Y, V.Z);
}

FRotator AShipActor::ConvertLegacyRotation(const FVector& V)
{
    return FRotator(V.Y, V.X, V.Z);
}

void AShipActor::UpdateDerivedPointsFromHull()
{
    if (HullMesh && HullMesh->GetStaticMesh())
    {
        const FVector Extent = HullMesh->Bounds.BoxExtent;
        FocusPoint->SetRelativeLocation(FVector(0.0f, 0.0f, Extent.Z * 0.5f));
    }
    else
    {
        FocusPoint->SetRelativeLocation(FocusPointOffset);
    }

    BridgePoint->SetRelativeLocation(BridgePointOffset);
    ChasePoint->SetRelativeLocation(ChasePointOffset);

    DriveCenterPoint->SetRelativeLocation(DriveCenterPointOffset);
    QuantumPoint->SetRelativeLocation(QuantumPointOffset);
    ShieldPoint->SetRelativeLocation(ShieldPointOffset);
    SensorPoint->SetRelativeLocation(SensorPointOffset);
    NavPoint->SetRelativeLocation(NavPointOffset);
    ComputerPointA->SetRelativeLocation(ComputerPointAOffset);
    ComputerPointB->SetRelativeLocation(ComputerPointBOffset);
    ReactorPoint->SetRelativeLocation(ReactorPointOffset);
}

void AShipActor::UpdateNavLights(float DeltaTime)
{
    if (!bEnableNavLights || NavLights.Num() == 0)
    {
        return;
    }

    const float TimeSeconds =
        GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    for (int32 Index = 0; Index < NavLights.Num(); ++Index)
    {
        UNavLightComponent* NavLight = NavLights[Index];

        if (!NavLight)
        {
            continue;
        }

        const FShipNavLightDef& Def = NavLight->GetDefinition();

        bool bVisible = true;

        if (Def.Mode == EShipNavLightMode::Blink)
        {
            const float Period =
                FMath::Max(0.05f, Def.BlinkInterval);

            const float PhaseTime =
                Def.PhaseOffset * Period;

            const float T =
                FMath::Fmod(TimeSeconds + PhaseTime, Period);

            bVisible = T < (Period * 0.5f);
        }
        else if (Def.Mode == EShipNavLightMode::Sequence)
        {
            NavLightSequenceTimer += DeltaTime;

            const float SequenceInterval = 0.20f;

            if (NavLightSequenceTimer >= SequenceInterval)
            {
                NavLightSequenceTimer = 0.0f;

                NavLightSequenceIndex =
                    (NavLightSequenceIndex + 1) %
                    FMath::Max(1, NavLights.Num());
            }

            bVisible = Index == NavLightSequenceIndex;
        }
        else
        {
            bVisible = true;
        }

        NavLight->SetVisibility(bVisible);
        NavLight->SetHiddenInGame(!bVisible);

        if (NavLightBulbMeshes.IsValidIndex(Index))
        {
            UStaticMeshComponent* BulbMesh = NavLightBulbMeshes[Index];

            if (BulbMesh)
            {
                BulbMesh->SetVisibility(bVisible);
                BulbMesh->SetHiddenInGame(!bVisible);
            }
        }

        if (NavLightPointLights.IsValidIndex(Index))
        {
            UPointLightComponent* PointLight = NavLightPointLights[Index];

            if (PointLight)
            {
                PointLight->SetVisibility(bVisible);
                PointLight->SetHiddenInGame(!bVisible);

                PointLight->SetLightColor(Def.Color);

                PointLight->SetIntensity(
                    bVisible
                    ? Def.Intensity * NavLightIntensityMultiplier
                    : 0.0f);

                PointLight->SetAttenuationRadius(
                    Def.Radius * NavLightRadiusMultiplier);
            }
        }

        if (NavLightBulbMIDs.IsValidIndex(Index))
        {
            UMaterialInstanceDynamic* MID = NavLightBulbMIDs[Index];

            if (MID)
            {
                const float Glow =
                    bVisible
                    ? FMath::Max(1.0f, Def.Intensity * 0.01f)
                    : 0.0f;

                MID->SetVectorParameterValue(TEXT("GlowColor"), Def.Color);
                MID->SetScalarParameterValue(TEXT("GlowIntensity"), Glow);
            }
        }
    }
}

void AShipActor::CreateMainEnginePoints()
{
    if (MainEnginePointDefs.Num() > 0)
    {
        for (int32 Index = 0; Index < MainEnginePointDefs.Num(); ++Index)
        {
            const FShipPointDef& Def = MainEnginePointDefs[Index];
            USceneComponent* Point = CreateGeneratedPoint(
                TEXT("MainEnginePoint"),
                Index,
                Def.LocalOffset,
                Def.LocalRotation);

            if (Point)
            {
                MainEnginePoints.Add(Point);
            }
        }

        return;
    }

    const int32 Count = FMath::Max(0, NumMainEnginePoints);

    for (int32 Index = 0; Index < Count; ++Index)
    {
        float YOffset = 0.0f;

        if (Count > 1)
        {
            const float Alpha = (float)Index / (float)(Count - 1);
            YOffset = -MainEngineSpread + (2.0f * MainEngineSpread * Alpha);
        }

        USceneComponent* Point = CreateGeneratedPoint(
            TEXT("MainEnginePoint"),
            Index,
            FVector(MainEngineX, YOffset, 0.0f),
            FRotator::ZeroRotator);

        if (Point)
        {
            MainEnginePoints.Add(Point);
        }
    }
}

void AShipActor::CreateThrusterPoints()
{
    if (ThrusterPointDefs.Num() > 0)
    {
        for (int32 Index = 0; Index < ThrusterPointDefs.Num(); ++Index)
        {
            const FShipPointDef& Def = ThrusterPointDefs[Index];
            USceneComponent* Point = CreateGeneratedPoint(
                TEXT("ThrusterPoint"),
                Index,
                Def.LocalOffset,
                Def.LocalRotation);

            if (Point)
            {
                ThrusterPoints.Add(Point);
            }
        }

        return;
    }

    const int32 Count = FMath::Max(0, NumThrusterPoints);

    for (int32 Index = 0; Index < Count; ++Index)
    {
        float YOffset = 0.0f;

        if (Count > 1)
        {
            const float Alpha = (float)Index / (float)(Count - 1);
            YOffset = -ThrusterSpread + (2.0f * ThrusterSpread * Alpha);
        }

        USceneComponent* Point = CreateGeneratedPoint(
            TEXT("ThrusterPoint"),
            Index,
            FVector(ThrusterX, YOffset, 0.0f),
            FRotator::ZeroRotator);

        if (Point)
        {
            ThrusterPoints.Add(Point);
        }
    }
}

void AShipActor::CreateWeaponMountPoints()
{
    if (WeaponMountPointDefs.Num() > 0)
    {
        for (int32 Index = 0; Index < WeaponMountPointDefs.Num(); ++Index)
        {
            const FShipPointDef& Def = WeaponMountPointDefs[Index];
            USceneComponent* Point = CreateGeneratedPoint(
                TEXT("WeaponMountPoint"),
                Index,
                Def.LocalOffset,
                Def.LocalRotation);

            if (Point)
            {
                WeaponMountPoints.Add(Point);
            }
        }

        return;
    }

    const int32 Count = FMath::Max(0, NumWeaponMountPoints);

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const float X = 100.0f + (Index * 25.0f);
        const float Y = ((Index % 2) == 0) ? -50.0f : 50.0f;
        const float Z = 0.0f;

        USceneComponent* Point = CreateGeneratedPoint(
            TEXT("WeaponMountPoint"),
            Index,
            FVector(X, Y, Z),
            FRotator::ZeroRotator);

        if (Point)
        {
            WeaponMountPoints.Add(Point);
        }
    }
}

void AShipActor::CreateTurretBasePoints()
{
    if (TurretBasePointDefs.Num() > 0)
    {
        for (int32 Index = 0; Index < TurretBasePointDefs.Num(); ++Index)
        {
            const FShipPointDef& Def = TurretBasePointDefs[Index];
            USceneComponent* Point = CreateGeneratedPoint(
                TEXT("TurretBasePoint"),
                Index,
                Def.LocalOffset,
                Def.LocalRotation);

            if (Point)
            {
                TurretBasePoints.Add(Point);
            }
        }

        return;
    }

    const int32 Count = FMath::Max(0, NumTurretBasePoints);

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const float X = (Index < 2) ? 50.0f : -50.0f;
        const float Y = ((Index % 2) == 0) ? -60.0f : 60.0f;
        const float Z = (Index < 2) ? 40.0f : -40.0f;

        USceneComponent* Point = CreateGeneratedPoint(
            TEXT("TurretBasePoint"),
            Index,
            FVector(X, Y, Z),
            FRotator::ZeroRotator);

        if (Point)
        {
            TurretBasePoints.Add(Point);
        }
    }
}

void AShipActor::CreateDockPoints()
{
    if (DockPointDefs.Num() > 0)
    {
        for (int32 Index = 0; Index < DockPointDefs.Num(); ++Index)
        {
            const FShipPointDef& Def = DockPointDefs[Index];
            USceneComponent* Point = CreateGeneratedPoint(
                TEXT("DockPoint"),
                Index,
                Def.LocalOffset,
                Def.LocalRotation);

            if (Point)
            {
                DockPoints.Add(Point);
            }
        }

        return;
    }

    const int32 Count = FMath::Max(0, NumDockPoints);

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const float X = 0.0f;
        const float Y = ((Index % 2) == 0) ? -120.0f : 120.0f;
        const float Z = 0.0f;

        USceneComponent* Point = CreateGeneratedPoint(
            TEXT("DockPoint"),
            Index,
            FVector(X, Y, Z),
            FRotator::ZeroRotator);

        if (Point)
        {
            DockPoints.Add(Point);
        }
    }
}

void AShipActor::CreateLandingPoints()
{
    if (LandingPointDefs.Num() > 0)
    {
        for (int32 Index = 0; Index < LandingPointDefs.Num(); ++Index)
        {
            const FShipPointDef& Def = LandingPointDefs[Index];
            USceneComponent* Point = CreateGeneratedPoint(
                TEXT("LandingPoint"),
                Index,
                Def.LocalOffset,
                Def.LocalRotation);

            if (Point)
            {
                LandingPoints.Add(Point);
            }
        }

        return;
    }

    const int32 Count = FMath::Max(0, NumLandingPoints);

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const float X = 0.0f;
        const float Y = (float)(Index * 80.0f);
        const float Z = -40.0f;

        USceneComponent* Point = CreateGeneratedPoint(
            TEXT("LandingPoint"),
            Index,
            FVector(X, Y, Z),
            FRotator::ZeroRotator);

        if (Point)
        {
            LandingPoints.Add(Point);
        }
    }
}

USceneComponent* AShipActor::CreateGeneratedPoint(
    const FString& BaseName,
    int32 Index,
    const FVector& LocalOffset,
    const FRotator& LocalRotation)
{
    const FString Name = FString::Printf(TEXT("%s_%d"), *BaseName, Index);
    USceneComponent* Point = NewObject<USceneComponent>(this, *Name);

    if (!Point)
    {
        return nullptr;
    }

    Point->SetupAttachment(PointRoot);
    Point->RegisterComponent();
    Point->SetRelativeLocation(LocalOffset);
    Point->SetRelativeRotation(LocalRotation);

    return Point;
}

void AShipActor::ClearSceneComponentArray(TArray<USceneComponent*>& Components)
{
    for (USceneComponent* Component : Components)
    {
        if (Component)
        {
            Component->DestroyComponent();
        }
    }

    Components.Empty();
}

void AShipActor::ClearNavLightArray(TArray<UNavLightComponent*>& Components)
{
    for (UNavLightComponent* Component : Components)
    {
        if (Component)
        {
            Component->DestroyComponent();
        }
    }

    Components.Empty();
}

void AShipActor::ClearNavLightVisuals()
{
    for (int32 i = 0; i < NavLightBulbMeshes.Num(); ++i)
    {
        UStaticMeshComponent* Comp = NavLightBulbMeshes[i];
        if (Comp)
        {
            Comp->DestroyComponent();
        }
    }
    NavLightBulbMeshes.Empty();

    for (int32 i = 0; i < NavLightPointLights.Num(); ++i)
    {
        UPointLightComponent* Light = NavLightPointLights[i];
        if (Light)
        {
            Light->DestroyComponent();
        }
    }
    NavLightPointLights.Empty();
    NavLightBulbMIDs.Empty();
}

void AShipActor::RebuildThrusterEmitters()
{
    UE_LOG(LogTemp, Warning,
        TEXT("[ShipActor] RebuildThrusterEmitters: BEGIN Ship='%s' Points=%d"),
        *GetName(),
        ThrusterPoints.Num());

    // ----------------------------------------------------
    // Clear existing
    // ----------------------------------------------------

    ClearComponentArray(ThrusterEmitters);

    // ----------------------------------------------------
    // Early out
    // ----------------------------------------------------

    if (!bEnableThrusterEmitters || !ThrusterEmitterSystem)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[ShipActor] RebuildThrusterEmitters: SKIPPED (disabled or no system)"));
        return;
    }

    // ----------------------------------------------------
    // Spawn emitters
    // ----------------------------------------------------

    for (int32 Index = 0; Index < ThrusterPoints.Num(); ++Index)
    {
        USceneComponent* Point = ThrusterPoints[Index];
        if (!Point)
        {
            continue;
        }

        const FString Name = FString::Printf(TEXT("ThrusterEmitter_%d"), Index);

        UNiagaraComponent* Emitter =
            NewObject<UNiagaraComponent>(this, *Name);

        if (!Emitter)
        {
            continue;
        }

        Emitter->SetupAttachment(Point);
        Emitter->RegisterComponent();

        Emitter->SetAsset(ThrusterEmitterSystem);
        Emitter->SetRelativeLocation(FVector::ZeroVector);
        Emitter->SetRelativeRotation(ThrusterEmitterRelativeRotation);
        Emitter->SetRelativeScale3D(ThrusterEmitterRelativeScale);

        // ----------------------------------------------------
        // Thruster ON/OFF control
        // ----------------------------------------------------

        Emitter->SetAutoActivate(bThrustersActive);

        if (bThrustersActive)
        {
            Emitter->Activate(true);
        }
        else
        {
            Emitter->Deactivate();
        }

        ThrusterEmitters.Add(Emitter);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[ShipActor] RebuildThrusterEmitters: END Created=%d"),
        ThrusterEmitters.Num());
}

USceneComponent* AShipActor::GetMainEnginePoint(int32 Index) const
{
    return MainEnginePoints.IsValidIndex(Index) ? MainEnginePoints[Index] : nullptr;
}

USceneComponent* AShipActor::GetThrusterPoint(int32 Index) const
{
    return ThrusterPoints.IsValidIndex(Index) ? ThrusterPoints[Index] : nullptr;
}

USceneComponent* AShipActor::GetWeaponMountPoint(int32 Index) const
{
    return WeaponMountPoints.IsValidIndex(Index) ? WeaponMountPoints[Index] : nullptr;
}

USceneComponent* AShipActor::GetTurretBasePoint(int32 Index) const
{
    return TurretBasePoints.IsValidIndex(Index) ? TurretBasePoints[Index] : nullptr;
}

USceneComponent* AShipActor::GetDockPoint(int32 Index) const
{
    return DockPoints.IsValidIndex(Index) ? DockPoints[Index] : nullptr;
}

USceneComponent* AShipActor::GetLandingPoint(int32 Index) const
{
    return LandingPoints.IsValidIndex(Index) ? LandingPoints[Index] : nullptr;
}

USceneComponent* AShipActor::FindWeaponMountPointByName(FName PointName) const
{
    if (PointName.IsNone())
    {
        return nullptr;
    }

    for (int32 Index = 0; Index < WeaponMountPointDefs.Num(); ++Index)
    {
        if (WeaponMountPointDefs[Index].PointName == PointName)
        {
            return WeaponMountPoints.IsValidIndex(Index) ? WeaponMountPoints[Index] : nullptr;
        }
    }

    return nullptr;
}

USceneComponent* AShipActor::FindTurretBasePointByName(FName PointName) const
{
    if (PointName.IsNone())
    {
        return nullptr;
    }

    for (int32 Index = 0; Index < TurretBasePointDefs.Num(); ++Index)
    {
        if (TurretBasePointDefs[Index].PointName == PointName)
        {
            return TurretBasePoints.IsValidIndex(Index) ? TurretBasePoints[Index] : nullptr;
        }
    }

    return nullptr;
}

bool AShipActor::AreMainEnginesActive() const
{
    return bMainEnginesActive;
}

bool AShipActor::AreThrustersActive() const
{
    return bThrustersActive;
}

void AShipActor::SetMainEnginesActive(bool bActive)
{
    bMainEnginesActive = bActive;

    for (TObjectPtr<UNiagaraComponent>& Emitter : MainEngineEmitters)
    {
        if (Emitter)
        {
            if (bMainEnginesActive)
            {
                Emitter->Activate(true);
            }
            else
            {
                Emitter->Deactivate();
            }
        }
    }

    for (TObjectPtr<UPointLightComponent>& Light : MainEngineLights)
    {
        if (Light)
        {
            Light->SetVisibility(bMainEnginesActive);
            Light->SetHiddenInGame(!bMainEnginesActive);
        }
    }
}

void AShipActor::SetThrustersActive(bool bActive)
{
    bThrustersActive = bActive;

    for (TObjectPtr<UNiagaraComponent>& Emitter : ThrusterEmitters)
    {
        if (Emitter)
        {
            if (bThrustersActive)
            {
                Emitter->Activate(true);
            }
            else
            {
                Emitter->Deactivate();
            }
        }
    }
}

FVector AShipActor::ConvertLegacyRegionLocToUELocal(const FVector& LegacyLoc) const
{
    /*
     * Starshatter legacy local axes:
     * X = side
     * Y = forward/depth
     * Z = up
     *
     * Unreal local axes:
     * X = forward/depth
     * Y = side
     * Z = up
     */
    return FVector(
        LegacyLoc.Y,
        LegacyLoc.X,
        LegacyLoc.Z);
}

void AShipActor::SetCutsceneNavMovement(
    const FVector& InStartLegacy,
    const FVector& InTargetLegacy,
    float InSpeed)
{
    CutsceneStartLocal = ConvertLegacyRegionLocToUELocal(InStartLegacy);
    CutsceneTargetLocal = ConvertLegacyRegionLocToUELocal(InTargetLegacy);
    CutsceneMoveSpeed = FMath::Max(InSpeed, 1.0f);
    bUseCutsceneNavMovement = true;

    SetActorRelativeLocation(CutsceneStartLocal);

    const FVector Direction = CutsceneTargetLocal - CutsceneStartLocal;
    if (!Direction.IsNearlyZero())
    {
        const FRotator MoveRotation = Direction.Rotation();
        SetActorRelativeRotation(MoveRotation);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[ShipActor] SetCutsceneNavMovement Actor=%s StartLegacy=%s TargetLegacy=%s StartUE=%s TargetUE=%s Speed=%.2f"),
        *GetName(),
        *InStartLegacy.ToString(),
        *InTargetLegacy.ToString(),
        *CutsceneStartLocal.ToString(),
        *CutsceneTargetLocal.ToString(),
        CutsceneMoveSpeed);
}


void AShipActor::SetCutsceneLocalMovement(
    const FVector& InStartLocal,
    const FVector& InTargetLocal,
    float InSpeed)
{
    CutsceneStartLocal = InStartLocal;
    CutsceneTargetLocal = InTargetLocal;
    CutsceneMoveSpeed = FMath::Max(InSpeed, 1.0f);
    bUseCutsceneNavMovement = true;

    if (GetRootComponent())
    {
        GetRootComponent()->SetRelativeLocation(CutsceneStartLocal);
    }

    const FVector Direction = CutsceneTargetLocal - CutsceneStartLocal;
    if (!Direction.IsNearlyZero())
    {
        GetRootComponent()->SetRelativeRotation(Direction.Rotation());
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[ShipActor] SetCutsceneLocalMovement Actor=%s Start=%s Target=%s Speed=%.2f"),
        *GetName(),
        *CutsceneStartLocal.ToString(),
        *CutsceneTargetLocal.ToString(),
        CutsceneMoveSpeed);
}

void AShipActor::UpdateCutsceneNavMovement(float DeltaTime)
{
    if (!GetRootComponent())
    {
        return;
    }

    const FVector CurrentLocal = GetRootComponent()->GetRelativeLocation();

    const FVector ToTarget = CutsceneTargetLocal - CurrentLocal;
    const float DistanceRemaining = ToTarget.Size();

    if (DistanceRemaining <= KINDA_SMALL_NUMBER)
    {
        GetRootComponent()->SetRelativeLocation(CutsceneTargetLocal);
        bUseCutsceneNavMovement = false;

        UE_LOG(LogTemp, Warning,
            TEXT("[ShipActor] Cutscene movement complete Actor=%s Final=%s"),
            *GetName(),
            *CutsceneTargetLocal.ToString());

        return;
    }

    const FVector Direction = ToTarget / DistanceRemaining;
    const float Step = CutsceneMoveSpeed * DeltaTime;

    FVector NewLocal;

    if (Step >= DistanceRemaining)
    {
        NewLocal = CutsceneTargetLocal;
        bUseCutsceneNavMovement = false;
    }
    else
    {
        NewLocal = CurrentLocal + (Direction * Step);
    }

    GetRootComponent()->SetRelativeLocation(NewLocal);
    GetRootComponent()->SetRelativeRotation(Direction.Rotation());
}

void AShipActor::BindRuntimeShip(Ship* InShip)
{
    RuntimeShip = InShip;

    Drive* MainDrive =
        RuntimeShip ? RuntimeShip->GetMainDrive() : nullptr;

    UE_LOG(LogTemp, Warning,
        TEXT("[ShipActor] BindRuntimeShip Actor=%s RuntimeShip=%p Ship='%s' NavLightSystems=%d MainDrive=%p DrivePorts=%d"),
        *GetName(),
        RuntimeShip,
        RuntimeShip ? *FString(RuntimeShip->GetName()) : TEXT("NULL"),
        RuntimeShip ? RuntimeShip->navlights.size() : 0,
        MainDrive,
        MainDrive ? MainDrive->NumPorts() : 0);

    BuildNavLightsFromRuntime();
    BuildMainEnginesFromRuntime();
    BuildThrustersFromRuntime();
}

void AShipActor::UpdateFromRuntimeShip(float DeltaTime)
{
    if (!RuntimeShip)
    {
        return;
    }

    const FVector RuntimeLocationLegacy = RuntimeShip->GetLocation();
    const FVector RuntimeVelocityLegacy = RuntimeShip->GetVelocity();
    const FVector RuntimeHeadingLegacy = RuntimeShip->GetHeading();

    const FVector RuntimeVelocityUE =
        ShipUtils::LegacySimToUnreal(RuntimeVelocityLegacy);

    const FVector RuntimeHeadingUE =
        ShipUtils::LegacySimToUnreal(RuntimeHeadingLegacy).GetSafeNormal();

    const bool bHasUsableVelocity =
        RuntimeVelocityUE.SizeSquared() >
        FMath::Square(RuntimeVelocityVisibleThreshold);

    FRotator DesiredRotation = GetActorRotation();

    if (!RuntimeHeadingUE.IsNearlyZero())
    {
        DesiredRotation = RuntimeHeadingUE.Rotation();
    }
    else if (bHasUsableVelocity)
    {
        DesiredRotation = RuntimeVelocityUE.GetSafeNormal().Rotation();
    }

    const FQuat ModelOffsetQuat = FQuat(FRotator(0.0f, 0.0f, 0.0f));
    DesiredRotation = (ModelOffsetQuat * DesiredRotation.Quaternion()).Rotator();

    //-------------------------------------------------------------
    // First frame: preserve correct spawn position.
    //-------------------------------------------------------------
    if (!bHasRuntimeTransform)
    {
        InitialRuntimeLocationLegacy = RuntimeLocationLegacy;
        InitialActorLocationUE = GetActorLocation();

        SetActorRotation(DesiredRotation);

        LastRuntimeLocation = InitialActorLocationUE;
        LastRuntimeVelocity = RuntimeVelocityUE;
        bHasRuntimeTransform = true;

        SetThrustersActive(bHasUsableVelocity);
        return;
    }

    //-------------------------------------------------------------
    // Convert only movement delta, not absolute location.
    //-------------------------------------------------------------
    const FVector RuntimeDeltaLegacy =
        RuntimeLocationLegacy - InitialRuntimeLocationLegacy;

    const FVector RuntimeDeltaUE =
        ShipUtils::LegacySimToUnreal(RuntimeDeltaLegacy);

    const FVector DesiredLocation =
        InitialActorLocationUE + RuntimeDeltaUE;

    const FVector SmoothedLocation = FMath::VInterpTo(
        GetActorLocation(),
        DesiredLocation,
        DeltaTime,
        RuntimeLocationInterpSpeed
    );

    const FRotator SmoothedRotation = FMath::RInterpTo(
        GetActorRotation(),
        DesiredRotation,
        DeltaTime,
        RuntimeRotationInterpSpeed
    );

    SetActorLocation(SmoothedLocation);
    SetActorRotation(SmoothedRotation);

    LastRuntimeLocation = DesiredLocation;
    LastRuntimeVelocity = RuntimeVelocityUE;

    SetThrustersActive(bHasUsableVelocity);

    UE_LOG(LogTemp, Warning,
        TEXT("[ShipActor::UpdateFromRuntimeShip] Ship='%s' ActorLoc=%s DesiredLoc=%s ActorFwd=%s ActorRot=%s LegacyDelta=%s UEDelta=%s LegacyHeading=%s UEHeading=%s LegacyVel=%s UEVel=%s"),
        *GetName(),
        *GetActorLocation().ToString(),
        *DesiredLocation.ToString(),
        *GetActorForwardVector().ToString(),
        *GetActorRotation().ToString(),
        *RuntimeDeltaLegacy.ToString(),
        *RuntimeDeltaUE.ToString(),
        *RuntimeHeadingLegacy.ToString(),
        *RuntimeHeadingUE.ToString(),
        *RuntimeVelocityLegacy.ToString(),
        *RuntimeVelocityUE.ToString());
}

void AShipActor::BuildNavLightsFromRuntime()
{
    if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
    {
        return;
    }

    if (GetName().StartsWith(TEXT("Default__")) ||
        GetName().StartsWith(TEXT("SKEL_")) ||
        GetName().StartsWith(TEXT("REINST_")))
    {
        return;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[ShipActor] BuildNavLightsFromRuntime ENTER Actor='%s' RuntimeShip=%p"),
        *GetName(),
        RuntimeShip);

    ClearNavLightArray(NavLights);
    ClearNavLightVisuals();

    if (!RuntimeShip)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[ShipActor] BuildNavLightsFromRuntime: RuntimeShip is null Actor='%s'"),
            *GetName());
        return;
    }

    if (RuntimeShip->navlights.size() <= 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[ShipActor] BuildNavLightsFromRuntime: no runtime navlights Actor='%s'"),
            *GetName());
        return;
    }

    int32 BuiltCount = 0;

    for (int32 LightSystemIndex = 0;
        LightSystemIndex < RuntimeShip->navlights.size();
        ++LightSystemIndex)
    {
        NavLight* RuntimeNavLight =
            RuntimeShip->navlights[LightSystemIndex];

        if (!RuntimeNavLight)
        {
            continue;
        }

        for (int32 BeaconIndex = 0;
            BeaconIndex < RuntimeNavLight->NumBeacons();
            ++BeaconIndex)
        {
            FShipNavLightDef Def;

            Def.LocalOffset =
                RuntimeNavLight->GetBeaconLocalLocation(BeaconIndex);

            Def.LocalRotation =
                RuntimeNavLight->GetBeaconLocalRotation(BeaconIndex);

            Def.Color =
                RuntimeNavLight->GetBeaconColor(BeaconIndex);

            Def.Intensity =
                RuntimeNavLight->GetBeaconIntensity(BeaconIndex);

            Def.Radius =
                RuntimeNavLight->GetBeaconRadius(BeaconIndex);

            Def.Mode =
                RuntimeNavLight->GetBeaconMode(BeaconIndex);

            Def.BlinkInterval =
                RuntimeNavLight->GetBeaconBlinkInterval(BeaconIndex);

            Def.PhaseOffset =
                RuntimeNavLight->GetBeaconPhaseOffset(BeaconIndex);

            AddRuntimeNavLightComponent(Def);

            BuiltCount++;
        }
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[ShipActor] BuildNavLightsFromRuntime Actor='%s' Ship='%s' RuntimeSystems=%d BuiltLights=%d"),
        *GetName(),
        *FString(RuntimeShip->GetName()),
        RuntimeShip->navlights.size(),
        BuiltCount);
}

void AShipActor::BuildMainEnginesFromRuntime()
{
    ClearRuntimeMainEngines();

    if (!RuntimeShip)
    {
        return;
    }

    Drive* MainDrive = RuntimeShip->GetMainDrive();

    if (!MainDrive)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[ShipActor] BuildMainEnginesFromRuntime: no main drive Actor='%s'"),
            *GetName());
        return;
    }

    const int32 PortCount = MainDrive->NumPorts();

    for (int32 i = 0; i < PortCount; ++i)
    {
        USceneComponent* Point =
            NewObject<USceneComponent>(
                this,
                *FString::Printf(TEXT("RuntimeMainEnginePoint_%d"), i));

        if (!Point)
        {
            continue;
        }

        Point->SetupAttachment(RootComponent);
        Point->RegisterComponent();

        Point->SetRelativeLocation(MainDrive->GetPortLocation(i));
        Point->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

        RuntimeMainEnginePoints.Add(Point);

        UNiagaraComponent* Emitter =
            NewObject<UNiagaraComponent>(
                this,
                *FString::Printf(TEXT("RuntimeMainEngineEmitter_%d"), i));

        if (Emitter)
        {
            Emitter->SetupAttachment(Point);
            Emitter->RegisterComponent();

            Emitter->SetAsset(MainEngineEmitterSystem);
            Emitter->SetRelativeLocation(FVector::ZeroVector);
            Emitter->SetRelativeRotation(FRotator::ZeroRotator);
            Emitter->SetRelativeScale3D(
                MainEngineEmitterRelativeScale * MainDrive->GetPortScale(i));

            Emitter->SetAutoActivate(false);
            Emitter->Deactivate();

            RuntimeMainEngineEmitters.Add(Emitter);
        }
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[ShipActor] BuildMainEnginesFromRuntime Actor='%s' Ports=%d Emitters=%d"),
        *GetName(),
        PortCount,
        RuntimeMainEngineEmitters.Num());
}

void AShipActor::ClearRuntimeNavLights()
{
    for (UNavLightComponent* Comp : NavLights)
    {
        if (Comp)
        {
            Comp->DestroyComponent();
        }
    }

    for (UStaticMeshComponent* Mesh : NavLightBulbMeshes)
    {
        if (Mesh)
        {
            Mesh->DestroyComponent();
        }
    }

    for (UPointLightComponent* Light : NavLightPointLights)
    {
        if (Light)
        {
            Light->DestroyComponent();
        }
    }

    NavLights.Empty();
    NavLightBulbMeshes.Empty();
    NavLightPointLights.Empty();
    NavLightBulbMIDs.Empty();
}

void AShipActor::ClearRuntimeMainEngines()
{
    for (UNiagaraComponent* Emitter : RuntimeMainEngineEmitters)
    {
        if (Emitter)
        {
            Emitter->DestroyComponent();
        }
    }

    RuntimeMainEngineEmitters.Empty();

    for (USceneComponent* Point : RuntimeMainEnginePoints)
    {
        if (Point)
        {
            Point->DestroyComponent();
        }
    }

    RuntimeMainEnginePoints.Empty();
}

void AShipActor::UpdateMainEnginesFromRuntime(float DeltaTime)
{
    if (!RuntimeShip)
    {
        return;
    }

    Drive* MainDrive = RuntimeShip->GetMainDrive();

    if (!MainDrive)
    {
        return;
    }

    const float EnginePower =
        FMath::Clamp(MainDrive->GetVisualPower(), 0.0f, 1.5f);

    const float EngineIntensity =
        FMath::Clamp(MainDrive->GetIntensity(), 0.0f, 1.0f);

    const bool bActive =
        EnginePower > 0.02f;

    for (UNiagaraComponent* Emitter : RuntimeMainEngineEmitters)
    {
        if (!Emitter)
        {
            continue;
        }

        if (bActive)
        {
            if (!Emitter->IsActive())
            {
                Emitter->Activate(true);
            }

            Emitter->SetVisibility(true);
            Emitter->SetHiddenInGame(false);
        }
        else
        {
            Emitter->Deactivate();
            Emitter->SetVisibility(false);
            Emitter->SetHiddenInGame(true);
        }

        Emitter->SetVariableFloat(
            TEXT("User.EnginePower"),
            EnginePower);

        Emitter->SetVariableFloat(
            TEXT("User.EngineIntensity"),
            EngineIntensity * MainEngineIntensityScale);

        Emitter->SetVariableFloat(
            TEXT("User.EngineLength"),
            FMath::Lerp(0.05f, 1.0f, EngineIntensity));
    }
}

void AShipActor::AddRuntimeNavLightComponent(const FShipNavLightDef& Def)
{
    UNavLightComponent* NavComp =
        NewObject<UNavLightComponent>(this);

    if (!NavComp)
    {
        return;
    }

    NavComp->RegisterComponent();
    NavComp->AttachToComponent(
        GetRootComponent(),
        FAttachmentTransformRules::KeepRelativeTransform);

    NavComp->ApplyDefinition(Def);
    NavComp->SetRelativeLocation(Def.LocalOffset);
    NavComp->SetRelativeRotation(Def.LocalRotation);

    NavLights.Add(NavComp);

    UPointLightComponent* PointLight =
        NewObject<UPointLightComponent>(this);

    if (PointLight)
    {
        PointLight->RegisterComponent();
        PointLight->AttachToComponent(
            GetRootComponent(),
            FAttachmentTransformRules::KeepRelativeTransform);

        PointLight->SetRelativeLocation(Def.LocalOffset);
        PointLight->SetLightColor(Def.Color);
        PointLight->SetIntensity(0.0f);
        PointLight->SetAttenuationRadius(Def.Radius);

        NavLightPointLights.Add(PointLight);
    }

    NavLightBulbMeshes.Add(nullptr);
    NavLightBulbMIDs.Add(nullptr);
}

void AShipActor::CreateEngineAudioComponents()
{
    if (!EngineAudioComponent && EngineSoundCue)
    {
        EngineAudioComponent =
            NewObject<UAudioComponent>(this, TEXT("EngineAudioComponent"));

        if (EngineAudioComponent)
        {
            EngineAudioComponent->SetupAttachment(RootComponent);
            EngineAudioComponent->RegisterComponent();
            EngineAudioComponent->SetSound(EngineSoundCue);
            EngineAudioComponent->bAutoActivate = false;
            EngineAudioComponent->SetVolumeMultiplier(0.0f);
        }
    }

    if (!BurnerAudioComponent && BurnerSoundCue)
    {
        BurnerAudioComponent =
            NewObject<UAudioComponent>(this, TEXT("BurnerAudioComponent"));

        if (BurnerAudioComponent)
        {
            BurnerAudioComponent->SetupAttachment(RootComponent);
            BurnerAudioComponent->RegisterComponent();
            BurnerAudioComponent->SetSound(BurnerSoundCue);
            BurnerAudioComponent->bAutoActivate = false;
            BurnerAudioComponent->SetVolumeMultiplier(0.0f);
        }
    }

    if (!RumbleAudioComponent && RumbleSoundCue)
    {
        RumbleAudioComponent =
            NewObject<UAudioComponent>(this, TEXT("RumbleAudioComponent"));

        if (RumbleAudioComponent)
        {
            RumbleAudioComponent->SetupAttachment(RootComponent);
            RumbleAudioComponent->RegisterComponent();
            RumbleAudioComponent->SetSound(RumbleSoundCue);
            RumbleAudioComponent->bAutoActivate = false;
            RumbleAudioComponent->SetVolumeMultiplier(0.0f);
        }
    }
}

void AShipActor::UpdateEngineAudioFromRuntime(float DeltaTime)
{
    if (!RuntimeShip)
    {
        return;
    }

    Drive* MainDrive = RuntimeShip->GetMainDrive();

    if (!MainDrive)
    {
        return;
    }

    CreateEngineAudioComponents();

    const float EnginePower =
        FMath::Clamp(MainDrive->GetVisualPower(), 0.0f, 1.5f);

    const float EngineIntensity =
        FMath::Clamp(MainDrive->GetIntensity(), 0.0f, 1.0f);

    const float AugPower =
        FMath::Clamp(MainDrive->GetAugmenterThrottle(), 0.0f, 1.0f);

    const bool bEngineActive =
        EnginePower > 0.02f;

    const bool bBurnerActive =
        AugPower > 0.05f && MainDrive->IsAugmenterOn();

    if (EngineAudioComponent)
    {
        if (bEngineActive && !EngineAudioComponent->IsPlaying())
        {
            EngineAudioComponent->Play();
        }

        if (!bEngineActive && EngineAudioComponent->IsPlaying())
        {
            EngineAudioComponent->Stop();
        }

        EngineAudioComponent->SetVolumeMultiplier(
            FMath::Clamp(EngineIntensity, 0.0f, 1.0f));

        EngineAudioComponent->SetPitchMultiplier(
            FMath::Lerp(0.85f, 1.25f, EngineIntensity));
    }

    if (BurnerAudioComponent)
    {
        if (bBurnerActive && !BurnerAudioComponent->IsPlaying())
        {
            BurnerAudioComponent->Play();
        }

        if (!bBurnerActive && BurnerAudioComponent->IsPlaying())
        {
            BurnerAudioComponent->Stop();
        }

        BurnerAudioComponent->SetVolumeMultiplier(
            FMath::Clamp(AugPower, 0.0f, 1.0f));

        BurnerAudioComponent->SetPitchMultiplier(
            FMath::Lerp(1.0f, 1.35f, AugPower));
    }

    if (RumbleAudioComponent)
    {
        const bool bRumbleActive =
            EnginePower > 0.15f;

        if (bRumbleActive && !RumbleAudioComponent->IsPlaying())
        {
            RumbleAudioComponent->Play();
        }

        if (!bRumbleActive && RumbleAudioComponent->IsPlaying())
        {
            RumbleAudioComponent->Stop();
        }

        RumbleAudioComponent->SetVolumeMultiplier(
            FMath::Clamp(EnginePower * 0.35f, 0.0f, 0.75f));

        RumbleAudioComponent->SetPitchMultiplier(
            FMath::Lerp(0.75f, 1.05f, EngineIntensity));
    }
}

void AShipActor::BuildThrustersFromRuntime()
{
    ClearRuntimeThrusters();

    if (!RuntimeShip)
    {
        return;
    }

    Thruster* RuntimeThruster = RuntimeShip->GetThruster();

    if (!RuntimeThruster)
    {
        return;
    }

    const int32 NumPorts = RuntimeThruster->NumThrusters();

    UE_LOG(LogTemp, Warning,
        TEXT("[ShipActor] BuildThrustersFromRuntime Actor='%s' Ports=%d"),
        *GetName(),
        NumPorts);

    for (int32 PortIndex = 0; PortIndex < NumPorts; ++PortIndex)
    {
        const FThrusterPort* Port =
            RuntimeThruster->GetPort(PortIndex);

        if (!Port)
        {
            continue;
        }

        FRuntimeThrusterFX FX;

        FX.PointName = Port->PointName;
        FX.Direction = Port->Direction;

        FX.Location = Port->Location;
        FX.Rotation = Port->Rotation;

        FX.PortScale = Port->PortScale;
        FX.AudioMultiplier = Port->AudioMultiplier;

        //-----------------------------------------------------
        // Flare
        //-----------------------------------------------------

        if (Port->bShowFlare && ThrusterFlareSystem)
        {
            FX.Flare =
                NewObject<UNiagaraComponent>(this);

            FX.Flare->SetAsset(ThrusterFlareSystem);

            FX.Flare->SetupAttachment(GetRootComponent());

            FX.Flare->SetRelativeLocation(Port->Location);
            FX.Flare->SetRelativeRotation(Port->Rotation);

            FX.Flare->SetAutoActivate(false);

            FX.Flare->RegisterComponent();

            FX.Flare->SetVariableFloat(
                TEXT("Scale"),
                Port->FlareScale);

            FX.Flare->SetVariableLinearColor(
                TEXT("ThrusterColor"),
                Port->ThrusterColor);
        }

        //-----------------------------------------------------
        // Trail
        //-----------------------------------------------------

        if (Port->bShowTrail && ThrusterTrailSystem)
        {
            FX.Trail =
                NewObject<UNiagaraComponent>(this);

            FX.Trail->SetAsset(ThrusterTrailSystem);

            FX.Trail->SetupAttachment(GetRootComponent());

            FX.Trail->SetRelativeLocation(Port->Location);
            FX.Trail->SetRelativeRotation(Port->Rotation);

            FX.Trail->SetAutoActivate(false);

            FX.Trail->RegisterComponent();

            FX.Trail->SetVariableFloat(
                TEXT("Scale"),
                Port->TrailScale);

            FX.Trail->SetVariableLinearColor(
                TEXT("ThrusterColor"),
                Port->ThrusterColor);
        }

        RuntimeThrusterFX.Add(FX);

        UE_LOG(LogTemp, Warning,
            TEXT("[ShipActor] ThrusterFX[%d] Name='%s' Dir=%d Loc=%s Rot=%s"),
            PortIndex,
            *Port->PointName.ToString(),
            static_cast<int32>(Port->Direction),
            *Port->Location.ToString(),
            *Port->Rotation.ToString());
    }
}

void AShipActor::UpdateThrustersFromRuntime(float DeltaTime)
{
    if (!RuntimeShip)
    {
        return;
    }

    Thruster* RuntimeThruster =
        RuntimeShip->GetThruster();

    if (!RuntimeThruster)
    {
        return;
    }

    const int32 Count =
        FMath::Min(
            RuntimeThrusterFX.Num(),
            RuntimeThruster->NumThrusters());

    for (int32 i = 0; i < Count; ++i)
    {
        const float Power = RuntimeThruster->GetVisualPower(i);
        const FThrusterPort* Port = RuntimeThruster->GetPort(i);

        if (Port && Power > 0.01f)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[ShipActor] THRUSTER FIRING Actor='%s' Index=%d Name='%s' Dir=%d Power=%.3f Fire=0x%04X Loc=%s"),
                *GetName(),
                i,
                *Port->PointName.ToString(),
                static_cast<int32>(Port->Direction),
                Power,
                Port->Fire,
                *Port->Location.ToString());
        }

        FRuntimeThrusterFX& FX = RuntimeThrusterFX[i];

        const bool bActive = Power > 0.01f;

        //-----------------------------------------------------
        // Flare
        //-----------------------------------------------------

        if (FX.Flare)
        {
            FX.Flare->SetVariableFloat(
                TEXT("Power"),
                Power);

            if (bActive)
            {
                if (!FX.Flare->IsActive())
                {
                    FX.Flare->Activate(true);
                }
            }
            else
            {
                if (FX.Flare->IsActive())
                {
                    FX.Flare->Deactivate();
                }
            }
        }

        //-----------------------------------------------------
        // Trail
        //-----------------------------------------------------

        if (FX.Trail)
        {
            FX.Trail->SetVariableFloat(
                TEXT("Power"),
                Power);

            if (bActive)
            {
                if (!FX.Trail->IsActive())
                {
                    FX.Trail->Activate(true);
                }
            }
            else
            {
                if (FX.Trail->IsActive())
                {
                    FX.Trail->Deactivate();
                }
            }
        }
    }
}

void AShipActor::ClearRuntimeThrusters()
{
    for (FRuntimeThrusterFX& FX : RuntimeThrusterFX)
    {
        if (FX.Flare)
        {
            FX.Flare->DestroyComponent();
            FX.Flare = nullptr;
        }

        if (FX.Trail)
        {
            FX.Trail->DestroyComponent();
            FX.Trail = nullptr;
        }
    }

    RuntimeThrusterFX.Empty();
}