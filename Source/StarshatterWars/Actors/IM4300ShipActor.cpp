/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         IM4300ShipActor.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    IM4300ShipActor applies CargoB.def values to the
    ShipActor base class.

    This class remains structural only. It does not
    implement gameplay systems or animated subsystem logic.
*/

#include "IM4300ShipActor.h"

AIM4300ShipActor::AIM4300ShipActor()
{
    ApplyIM4300Defaults();
}

void AIM4300ShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyIM4300Defaults();

    Super::OnConstruction(Transform);

    ApplyIM4300FixedPoints();
}

void AIM4300ShipActor::ApplyIM4300Defaults()
{
    /*
     * CargoB.def / IM4300 values:
     *
     * scale:       2.5
     * chase:       (0, -1000, 200)
     * bridge:      (0, 216, 34)
     * drive loc:   (0, 30, -350)
     * drive port:  (-92, 0, -488)
     * drive port:  (94, 0, -488)
     * thruster:    (0, 0, 64)
     * quantum:     (0, 0, -120)
     * shield:      (0, 20, -80)
     * sensor:      (0, 0, 180)
     * computer 1:  (20, 16, 80)
     * computer 2:  (-20, -16, 80)
     * nav:         (0, 16, 60)
     */

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
        FVector(1.00f, 0.50f, 0.50f);

    MainEngineEmitterRelativeRotation =
        FRotator(0.0f, 180.0f, 0.0f);

    /*
     * Camera and framing points
     */
    FocusPointOffset =
        FVector(0.0f, 216.0f, 34.0f);

    BridgePointOffset =
        FVector(0.0f, 216.0f, 34.0f);

    ChasePointOffset =
        FVector(0.0f, -1000.0f, 200.0f);

    /*
     * No weapon/dock/landing points defined in CargoB.def.
     */
    NumWeaponMountPoints = 0;
    NumTurretBasePoints = 0;
    NumDockPoints = 0;
    NumLandingPoints = 0;

    WeaponMountPointDefs.Empty();
    TurretBasePointDefs.Empty();
    DockPointDefs.Empty();
    LandingPointDefs.Empty();

    /*
     * IM4300 / CargoB.def scale.
     */
    SetActorScale3D(FVector(2.5f, 2.5f, 2.5f));
}

void AIM4300ShipActor::ApplyIM4300FixedPoints()
{
    if (FocusPoint)
    {
        FocusPoint->SetRelativeLocation(
            FVector(0.0f, 216.0f, 34.0f));
    }

    if (BridgePoint)
    {
        BridgePoint->SetRelativeLocation(
            FVector(0.0f, 216.0f, 34.0f));
    }

    if (ChasePoint)
    {
        ChasePoint->SetRelativeLocation(
            FVector(0.0f, -1000.0f, 200.0f));
    }

    if (DriveCenterPoint)
    {
        DriveCenterPoint->SetRelativeLocation(
            FVector(0.0f, 30.0f, -350.0f));
    }

    if (QuantumPoint)
    {
        QuantumPoint->SetRelativeLocation(
            FVector(0.0f, 0.0f, -120.0f));
    }

    if (ShieldPoint)
    {
        ShieldPoint->SetRelativeLocation(
            FVector(0.0f, 20.0f, -80.0f));
    }

    if (SensorPoint)
    {
        SensorPoint->SetRelativeLocation(
            FVector(0.0f, 0.0f, 180.0f));
    }

    if (ComputerPointA)
    {
        ComputerPointA->SetRelativeLocation(
            FVector(20.0f, 16.0f, 80.0f));
    }

    if (ComputerPointB)
    {
        ComputerPointB->SetRelativeLocation(
            FVector(-20.0f, -16.0f, 80.0f));
    }

    if (NavPoint)
    {
        NavPoint->SetRelativeLocation(
            FVector(0.0f, 16.0f, 60.0f));
    }
}