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

    HullMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HullMesh"));
    HullMesh->SetupAttachment(VisualRoot);
    HullMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HullMesh->SetGenerateOverlapEvents(false);
    HullMesh->SetMobility(EComponentMobility::Movable);

    FocusPoint = CreateDefaultSubobject<USceneComponent>(TEXT("FocusPoint"));
    FocusPoint->SetupAttachment(ShipRoot);

    BridgePoint = CreateDefaultSubobject<USceneComponent>(TEXT("BridgePoint"));
    BridgePoint->SetupAttachment(ShipRoot);

    ChasePoint = CreateDefaultSubobject<USceneComponent>(TEXT("ChasePoint"));
    ChasePoint->SetupAttachment(ShipRoot);

    DriveCenterPoint = CreateDefaultSubobject<USceneComponent>(TEXT("DriveCenterPoint"));
    DriveCenterPoint->SetupAttachment(ShipRoot);

    QuantumPoint = CreateDefaultSubobject<USceneComponent>(TEXT("QuantumPoint"));
    QuantumPoint->SetupAttachment(ShipRoot);

    ShieldPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ShieldPoint"));
    ShieldPoint->SetupAttachment(ShipRoot);

    SensorPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SensorPoint"));
    SensorPoint->SetupAttachment(ShipRoot);

    NavPoint = CreateDefaultSubobject<USceneComponent>(TEXT("NavPoint"));
    NavPoint->SetupAttachment(ShipRoot);

    ComputerPointA = CreateDefaultSubobject<USceneComponent>(TEXT("ComputerPointA"));
    ComputerPointA->SetupAttachment(ShipRoot);

    ComputerPointB = CreateDefaultSubobject<USceneComponent>(TEXT("ComputerPointB"));
    ComputerPointB->SetupAttachment(ShipRoot);

    ReactorPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ReactorPoint"));
    ReactorPoint->SetupAttachment(ShipRoot);

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
    Super::OnConstruction(Transform);

    UpdateDerivedPointsFromHull();
    RebuildAllGeneratedComponents();
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

void AShipActor::RebuildNavLights()
{
    ClearNavLightArray(NavLightComponents);

    for (int32 Index = 0; Index < NavLightDefs.Num(); ++Index)
    {
        const FShipNavLightDef& Def = NavLightDefs[Index];

        const FString Name = FString::Printf(TEXT("NavLight_%d"), Index);
        UPointLightComponent* Light = NewObject<UPointLightComponent>(this, *Name);

        if (!Light)
        {
            continue;
        }

        Light->SetupAttachment(ShipRoot);
        Light->RegisterComponent();

        Light->SetRelativeLocation(Def.LocalOffset);
        Light->SetRelativeRotation(Def.LocalRotation);
        Light->SetLightColor(Def.Color);
        Light->SetIntensity(Def.Intensity);
        Light->SetAttenuationRadius(Def.Radius);
        Light->SetCastShadows(false);
        Light->SetVisibility(true);

        NavLightComponents.Add(Light);
    }
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
    const int32 Count = FMath::Max(0, NumMainEnginePoints);

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const FString Name = FString::Printf(TEXT("MainEnginePoint_%d"), Index);
        USceneComponent* Point = NewObject<USceneComponent>(this, *Name);

        if (!Point)
        {
            continue;
        }

        Point->SetupAttachment(ShipRoot);
        Point->RegisterComponent();

        float YOffset = 0.0f;
        if (Count > 1)
        {
            const float Alpha = (float)Index / (float)(Count - 1);
            YOffset = -MainEngineSpread + (2.0f * MainEngineSpread * Alpha);
        }

        Point->SetRelativeLocation(FVector(MainEngineX, YOffset, 0.0f));
        MainEnginePoints.Add(Point);
    }
}

void AShipActor::CreateThrusterPoints()
{
    const int32 Count = FMath::Max(0, NumThrusterPoints);

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const FString Name = FString::Printf(TEXT("ThrusterPoint_%d"), Index);
        USceneComponent* Point = NewObject<USceneComponent>(this, *Name);

        if (!Point)
        {
            continue;
        }

        Point->SetupAttachment(ShipRoot);
        Point->RegisterComponent();

        float YOffset = 0.0f;
        if (Count > 1)
        {
            const float Alpha = (float)Index / (float)(Count - 1);
            YOffset = -ThrusterSpread + (2.0f * ThrusterSpread * Alpha);
        }

        Point->SetRelativeLocation(FVector(ThrusterX, YOffset, 0.0f));
        ThrusterPoints.Add(Point);
    }
}

void AShipActor::CreateWeaponMountPoints()
{
    const int32 Count = FMath::Max(0, NumWeaponMountPoints);

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const FString Name = FString::Printf(TEXT("WeaponMountPoint_%d"), Index);
        USceneComponent* Point = NewObject<USceneComponent>(this, *Name);

        if (!Point)
        {
            continue;
        }

        Point->SetupAttachment(ShipRoot);
        Point->RegisterComponent();

        const float X = 100.0f + (Index * 25.0f);
        const float Y = ((Index % 2) == 0) ? -50.0f : 50.0f;
        const float Z = 0.0f;

        Point->SetRelativeLocation(FVector(X, Y, Z));
        WeaponMountPoints.Add(Point);
    }
}

void AShipActor::CreateTurretBasePoints()
{
    const int32 Count = FMath::Max(0, NumTurretBasePoints);

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const FString Name = FString::Printf(TEXT("TurretBasePoint_%d"), Index);
        USceneComponent* Point = NewObject<USceneComponent>(this, *Name);

        if (!Point)
        {
            continue;
        }

        Point->SetupAttachment(ShipRoot);
        Point->RegisterComponent();

        const float X = (Index < 2) ? 50.0f : -50.0f;
        const float Y = ((Index % 2) == 0) ? -60.0f : 60.0f;
        const float Z = (Index < 2) ? 40.0f : -40.0f;

        Point->SetRelativeLocation(FVector(X, Y, Z));
        TurretBasePoints.Add(Point);
    }
}

void AShipActor::CreateDockPoints()
{
    const int32 Count = FMath::Max(0, NumDockPoints);

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const FString Name = FString::Printf(TEXT("DockPoint_%d"), Index);
        USceneComponent* Point = NewObject<USceneComponent>(this, *Name);

        if (!Point)
        {
            continue;
        }

        Point->SetupAttachment(ShipRoot);
        Point->RegisterComponent();

        const float X = 0.0f;
        const float Y = ((Index % 2) == 0) ? -120.0f : 120.0f;
        const float Z = 0.0f;

        Point->SetRelativeLocation(FVector(X, Y, Z));
        DockPoints.Add(Point);
    }
}

void AShipActor::CreateLandingPoints()
{
    const int32 Count = FMath::Max(0, NumLandingPoints);

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const FString Name = FString::Printf(TEXT("LandingPoint_%d"), Index);
        USceneComponent* Point = NewObject<USceneComponent>(this, *Name);

        if (!Point)
        {
            continue;
        }

        Point->SetupAttachment(ShipRoot);
        Point->RegisterComponent();

        const float X = 0.0f;
        const float Y = (float)(Index * 80.0f);
        const float Z = -40.0f;

        Point->SetRelativeLocation(FVector(X, Y, Z));
        LandingPoints.Add(Point);
    }
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