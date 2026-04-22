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
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

AShipActor::AShipActor()
{
    PrimaryActorTick.bCanEverTick = true;

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

    FocusPoint->SetRelativeLocation(FocusPointOffset);
    BridgePoint->SetRelativeLocation(BridgePointOffset);
    ChasePoint->SetRelativeLocation(ChasePointOffset);

    DriveCenterPoint->SetRelativeLocation(FVector::ZeroVector);
    QuantumPoint->SetRelativeLocation(FVector::ZeroVector);
    ShieldPoint->SetRelativeLocation(FVector::ZeroVector);
    SensorPoint->SetRelativeLocation(FVector::ZeroVector);
    NavPoint->SetRelativeLocation(FVector::ZeroVector);
    ComputerPointA->SetRelativeLocation(FVector::ZeroVector);
    ComputerPointB->SetRelativeLocation(FVector::ZeroVector);
    ReactorPoint->SetRelativeLocation(FVector::ZeroVector);

    /*
     * Default fallback nav lights.
     * These are generic and can be overridden per ship.
     */
    {
        FShipNavLightDef Port;
        Port.LocalOffset = FVector(0.0f, -100.0f, 20.0f);
        Port.Color = FLinearColor::Red;
        Port.Intensity = 3000.0f;
        Port.Radius = 300.0f;
        Port.bBlink = false;
        Port.BlinkInterval = 1.0f;
        NavLightDefs.Add(Port);

        FShipNavLightDef Starboard;
        Starboard.LocalOffset = FVector(0.0f, 100.0f, 20.0f);
        Starboard.Color = FLinearColor::Green;
        Starboard.Intensity = 3000.0f;
        Starboard.Radius = 300.0f;
        Starboard.bBlink = false;
        Starboard.BlinkInterval = 1.0f;
        NavLightDefs.Add(Starboard);

        FShipNavLightDef Dorsal;
        Dorsal.LocalOffset = FVector(-40.0f, 0.0f, 60.0f);
        Dorsal.Color = FLinearColor::White;
        Dorsal.Intensity = 2500.0f;
        Dorsal.Radius = 250.0f;
        Dorsal.bBlink = false;
        Dorsal.BlinkInterval = 1.0f;
        NavLightDefs.Add(Dorsal);

        FShipNavLightDef Ventral;
        Ventral.LocalOffset = FVector(-40.0f, 0.0f, -60.0f);
        Ventral.Color = FLinearColor(0.6f, 0.6f, 1.0f);
        Ventral.Intensity = 2000.0f;
        Ventral.Radius = 250.0f;
        Ventral.bBlink = false;
        Ventral.BlinkInterval = 1.0f;
        NavLightDefs.Add(Ventral);
    }
}

void AShipActor::OnConstruction(const FTransform& Transform)
{
    UE_LOG(LogTemp, Warning,
        TEXT("[ShipActor] OnConstruction: %s  Auto=%d Rebuild=%d Defs=%d"),
        *GetName(),
        bAutoRebuildGeneratedComponents ? 1 : 0,
        bRebuildOnConstruction ? 1 : 0,
        NavLightDefs.Num());

    Super::OnConstruction(Transform);

    UpdateDerivedPointsFromHull();

    if (bAutoRebuildGeneratedComponents && bRebuildOnConstruction)
    {
        RebuildAllGeneratedComponents();
    }
}

void AShipActor::BeginPlay()
{
    Super::BeginPlay();
}

void AShipActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    /*
     * Nav light blinking is intentionally not implemented yet.
     * Most structural components are anchors only at this stage.
     */
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
    RebuildThrusterPoints();
    RebuildWeaponMountPoints();
    RebuildTurretBasePoints();
    RebuildDockPoints();
    RebuildLandingPoints();
    RebuildNavLights();
}

void AShipActor::RebuildMainEnginePoints()
{
    ClearSceneComponentArray(MainEnginePoints);
    CreateMainEnginePoints();
}

void AShipActor::RebuildThrusterPoints()
{
    ClearSceneComponentArray(ThrusterPoints);
    CreateThrusterPoints();
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

    ClearNavLightArray(NavLightComponents);
    ClearEmitterArray(NavLightEmitters);

    if (!bEnableNavLights || !LightRoot)
    {
        return;
    }

    for (int32 Index = 0; Index < NavLightDefs.Num(); ++Index)
    {
        const FShipNavLightDef& Def = NavLightDefs[Index];

        const FString Name = FString::Printf(TEXT("NavLight_%d"), Index);

        /*
         * === LIGHT ===
         */
        UPointLightComponent* Light = NewObject<UPointLightComponent>(this, *Name);

        if (!Light)
        {
            continue;
        }

        Light->SetupAttachment(LightRoot);
        Light->RegisterComponent();

        Light->SetRelativeLocation(Def.LocalOffset);
        Light->SetRelativeRotation(Def.LocalRotation);
        Light->SetLightColor(Def.Color);

        const float FinalIntensity = Def.Intensity * NavLightIntensityMultiplier;
        const float FinalRadius = Def.Radius * NavLightRadiusMultiplier;

        Light->SetUseInverseSquaredFalloff(false);
        Light->SetIntensity(FinalIntensity);
        Light->SetAttenuationRadius(FinalRadius);
        Light->SetCastShadows(false);

        NavLightComponents.Add(Light);

        /*
         * === VISIBLE EMITTER ===
         */
        UStaticMeshComponent* Emitter = NewObject<UStaticMeshComponent>(this);

        if (Emitter)
        {
            Emitter->SetupAttachment(Light);
            Emitter->RegisterComponent();

            Emitter->SetRelativeLocation(FVector::ZeroVector);
            Emitter->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Emitter->SetCastShadow(false);

            if (NavLightMesh)
            {
                Emitter->SetStaticMesh(NavLightMesh);
            }

            // scale small but visible
            Emitter->SetWorldScale3D(FVector(0.1f));

            if (NavLightMaterial)
            {
                UMaterialInstanceDynamic* MID =
                    UMaterialInstanceDynamic::Create(NavLightMaterial, this);

                if (MID)
                {
                    MID->SetVectorParameterValue("EmissiveColor", Def.Color);
                    Emitter->SetMaterial(0, MID);
                }
            }

            NavLightEmitters.Add(Emitter);
        }

        UE_LOG(LogTemp, Warning,
            TEXT("[ShipActor] NavLight_%d created (Light + Emitter)"),
            Index);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[ShipActor] RebuildNavLights: END Created=%d"),
        NavLightComponents.Num());
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

void AShipActor::ClearNavLightArray(TArray<UPointLightComponent*>& Components)
{
    for (UPointLightComponent* Component : Components)
    {
        if (Component)
        {
            Component->DestroyComponent();
        }
    }

    Components.Empty();
}

void AShipActor::ClearEmitterArray(TArray<UStaticMeshComponent*>& Components)
{
    for (UStaticMeshComponent* Comp : Components)
    {
        if (Comp)
        {
            Comp->DestroyComponent();
        }
    }

    Components.Empty();
}