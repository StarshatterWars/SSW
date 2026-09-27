/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         BaikalShipActor.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    BaikalShipActor applies Baikal.def values to the
    ShipActor base class.

    Runtime systems now own:
    - drive ports
    - thruster ports
    - nav lights

    This actor now only owns structural points and
    camera framing offsets.
*/

#include "BaikalShipActor.h"

ABaikalShipActor::ABaikalShipActor()
{
    ApplyBaikalDefaults();
}

void ABaikalShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyBaikalDefaults();

    Super::OnConstruction(Transform);

    ApplyBaikalFixedPoints();
}

void ABaikalShipActor::ApplyBaikalDefaults()
{
    bAutoRebuildGeneratedComponents = true;
    bRebuildOnConstruction = true;

    /*
     * Runtime visual systems
     */
    bEnableMainEngineEmitters = true;
    bEnableThrusterEmitters = true;

    /*
     * Main engine Niagara defaults
     */
    MainEngineEmitterRelativeScale =
        FVector(1.15f, 0.50f, 0.50f);

    MainEngineEmitterRelativeRotation =
        FRotator(0.0f, 180.0f, 0.0f);

    /*
     * Camera and framing points
     */
    FocusPointOffset =
        FVector(92.0f, 0.0f, 0.0f);

    BridgePointOffset =
        FVector(92.0f, 0.0f, 0.0f);

    ChasePointOffset =
        FVector(200.0f, -1000.0f, 0.0f);

    /*
     * Ship subsystem reference points
     */
    DriveCenterPointOffset =
        FVector(-220.0f, 0.0f, 0.0f);

    QuantumPointOffset =
        FVector(-120.0f, 0.0f, 0.0f);

    ShieldPointOffset =
        FVector(-80.0f, 20.0f, 0.0f);

    SensorPointOffset =
        FVector(180.0f, 0.0f, 0.0f);

    NavPointOffset =
        FVector(60.0f, 16.0f, 0.0f);

    ComputerPointAOffset =
        FVector(80.0f, 16.0f, 20.0f);

    ComputerPointBOffset =
        FVector(80.0f, -16.0f, -20.0f);

    ReactorPointOffset =
        FVector(-60.0f, 0.0f, 0.0f);

    /*
     * Structural generated points only
     */
    NumWeaponMountPoints = 5;
    NumTurretBasePoints = 0;
    NumDockPoints = 0;
    NumLandingPoints = 0;

    BuildBaikalWeaponMountPoints();

    TurretBasePointDefs.Empty();
    DockPointDefs.Empty();
    LandingPointDefs.Empty();

    SetActorScale3D(FVector(1.6f));
}

void ABaikalShipActor::ApplyBaikalFixedPoints()
{
    if (FocusPoint)
    {
        FocusPoint->SetRelativeLocation(
            FVector(92.0f, 0.0f, 0.0f));
    }

    if (BridgePoint)
    {
        BridgePoint->SetRelativeLocation(
            FVector(92.0f, 0.0f, 0.0f));
    }

    if (ChasePoint)
    {
        ChasePoint->SetRelativeLocation(
            FVector(200.0f, -1000.0f, 0.0f));
    }

    if (DriveCenterPoint)
    {
        DriveCenterPoint->SetRelativeLocation(
            FVector(-220.0f, 0.0f, 0.0f));
    }

    if (QuantumPoint)
    {
        QuantumPoint->SetRelativeLocation(
            FVector(-120.0f, 0.0f, 0.0f));
    }

    if (ShieldPoint)
    {
        ShieldPoint->SetRelativeLocation(
            FVector(-80.0f, 20.0f, 0.0f));
    }

    if (SensorPoint)
    {
        SensorPoint->SetRelativeLocation(
            FVector(180.0f, 0.0f, 0.0f));
    }

    if (NavPoint)
    {
        NavPoint->SetRelativeLocation(
            FVector(60.0f, 16.0f, 0.0f));
    }

    if (ComputerPointA)
    {
        ComputerPointA->SetRelativeLocation(
            FVector(80.0f, 16.0f, 20.0f));
    }

    if (ComputerPointB)
    {
        ComputerPointB->SetRelativeLocation(
            FVector(80.0f, -16.0f, -20.0f));
    }

    if (ReactorPoint)
    {
        ReactorPoint->SetRelativeLocation(
            FVector(-60.0f, 0.0f, 0.0f));
    }
}

void ABaikalShipActor::BuildBaikalWeaponMountPoints()
{
    WeaponMountPointDefs.Empty();

    FShipPointDef W;

    W.LocalOffset =
        FVector(500.0f, 25.0f, 0.0f);

    W.LocalRotation =
        FRotator(0.0f, 0.0f, 0.0f);

    W.PointName =
        TEXT("Fwd_Cannon");

    WeaponMountPointDefs.Add(W);

    W.LocalOffset =
        FVector(450.0f, 0.0f, -20.0f);

    W.LocalRotation =
        FRotator(0.0f, 0.0f, 0.0f);

    W.PointName =
        TEXT("Interceptor_Left");

    WeaponMountPointDefs.Add(W);

    W.LocalOffset =
        FVector(450.0f, 0.0f, 20.0f);

    W.LocalRotation =
        FRotator(0.0f, 0.0f, 0.0f);

    W.PointName =
        TEXT("Interceptor_Right");

    WeaponMountPointDefs.Add(W);

    W.LocalOffset =
        FVector(-30.0f, 0.0f, 85.0f);

    W.LocalRotation =
        FRotator(0.0f, 90.0f, 0.0f);

    W.PointName =
        TEXT("Starboard_Cannon");

    WeaponMountPointDefs.Add(W);

    W.LocalOffset =
        FVector(-30.0f, 0.0f, -85.0f);

    W.LocalRotation =
        FRotator(0.0f, -90.0f, 0.0f);

    W.PointName =
        TEXT("Port_Cannon");

    WeaponMountPointDefs.Add(W);
}

void ABaikalShipActor::BuildBaikalTurretBasePoints()
{
    TurretBasePointDefs.Empty();
}

void ABaikalShipActor::BuildBaikalDockPoints()
{
    DockPointDefs.Empty();
}

void ABaikalShipActor::BuildBaikalLandingPoints()
{
    LandingPointDefs.Empty();
}