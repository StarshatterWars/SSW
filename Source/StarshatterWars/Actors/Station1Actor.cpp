/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         Station1Actor.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Station1Actor applies FreightXfer.def values to the
    ShipActor base class.

    This class remains structural only. It does not
    implement gameplay systems, docking logic, launch logic,
    recovery logic, or animated subsystem behavior.
*/

#include "Station1Actor.h"

AStation1Actor::AStation1Actor()
{
    ApplyStation1Defaults();
}

void AStation1Actor::OnConstruction(const FTransform& Transform)
{
    ApplyStation1Defaults();

    Super::OnConstruction(Transform);

    ApplyStation1FixedPoints();
}

void AStation1Actor::ApplyStation1Defaults()
{
    /*
     * FreightXfer.def / Station1 values:
     *
     * name:        Station1
     * display:     Freight Xfer
     * class:       STATION
     * model:       Station1
     * scale:       5
     * chase:       (0, -1200, 250)
     * bridge:      (0, 0, 32)
     * sensor:      (0, -16, 380)
     *
     * flightdeck:
     * Launch Bay 1  loc (390, 20, 930)
     * Docking Bay 1 loc (-390, 20, 930)
     * Launch Bay 2  loc (390, 20, -930)
     * Docking Bay 2 loc (-390, 20, -930)
     *
     * navlights:
     * (-390, 0, -1270)
     * (-390, 0, 1270)
     * (390, 0, -1270)
     * (390, 0, 1270)
     */

    bAutoRebuildGeneratedComponents = true;
    bRebuildOnConstruction = true;

    /*
     * Station has no main drive or thruster block in FreightXfer.def.
     * Keep engine and thruster VFX disabled.
     */
    bEnableMainEngineEmitters = false;
    bEnableThrusterEmitters = false;

    FocusPointOffset =
        FVector(0.0f, 0.0f, 32.0f);

    BridgePointOffset =
        FVector(0.0f, 0.0f, 32.0f);

    ChasePointOffset =
        FVector(0.0f, -1200.0f, 250.0f);

    NumWeaponMountPoints = 0;
    NumTurretBasePoints = 0;

    /*
     * Two recovery/docking bays are defined.
     */
    NumDockPoints = 2;

    /*
     * Two launch bays are defined.
     */
    NumLandingPoints = 2;

    WeaponMountPointDefs.Empty();
    TurretBasePointDefs.Empty();
    DockPointDefs.Empty();
    LandingPointDefs.Empty();

    /*
     * Docking Bay 1
     */
    {
        FShipPointDef Point;
        Point.LocalOffset =
            FVector(-390.0f, 20.0f, 930.0f);
        Point.LocalRotation =
            FRotator(0.0f, 90.0f, 0.0f);
        DockPointDefs.Add(Point);
    }

    /*
     * Docking Bay 2
     */
    {
        FShipPointDef Point;
        Point.LocalOffset =
            FVector(-390.0f, 20.0f, -930.0f);
        Point.LocalRotation =
            FRotator(0.0f, 90.0f, 0.0f);
        DockPointDefs.Add(Point);
    }

    /*
     * Launch Bay 1
     */
    {
        FShipPointDef Point;
        Point.LocalOffset =
            FVector(390.0f, 20.0f, 930.0f);
        Point.LocalRotation =
            FRotator(0.0f, 90.0f, 0.0f);
        LandingPointDefs.Add(Point);
    }

    /*
     * Launch Bay 2
     */
    {
        FShipPointDef Point;
        Point.LocalOffset =
            FVector(390.0f, 20.0f, -930.0f);
        Point.LocalRotation =
            FRotator(0.0f, 90.0f, 0.0f);
        LandingPointDefs.Add(Point);
    }

    SetActorScale3D(FVector(5.0f, 5.0f, 5.0f));
}

void AStation1Actor::ApplyStation1FixedPoints()
{
    if (FocusPoint)
    {
        FocusPoint->SetRelativeLocation(
            FVector(0.0f, 0.0f, 32.0f));
    }

    if (BridgePoint)
    {
        BridgePoint->SetRelativeLocation(
            FVector(0.0f, 0.0f, 32.0f));
    }

    if (ChasePoint)
    {
        ChasePoint->SetRelativeLocation(
            FVector(0.0f, -1200.0f, 250.0f));
    }

    if (DriveCenterPoint)
    {
        DriveCenterPoint->SetRelativeLocation(
            FVector::ZeroVector);
    }

    if (QuantumPoint)
    {
        QuantumPoint->SetRelativeLocation(
            FVector::ZeroVector);
    }

    if (ShieldPoint)
    {
        ShieldPoint->SetRelativeLocation(
            FVector::ZeroVector);
    }

    if (SensorPoint)
    {
        SensorPoint->SetRelativeLocation(
            FVector(0.0f, -16.0f, 380.0f));
    }

    if (ComputerPointA)
    {
        ComputerPointA->SetRelativeLocation(
            FVector::ZeroVector);
    }

    if (ComputerPointB)
    {
        ComputerPointB->SetRelativeLocation(
            FVector::ZeroVector);
    }

    if (NavPoint)
    {
        NavPoint->SetRelativeLocation(
            FVector::ZeroVector);
    }
}