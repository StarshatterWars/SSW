/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         IM2800ShipActor.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    IM2800ShipActor applies CargoC.def values to the
    ShipActor base class.

    This class remains structural only. It does not
    implement gameplay systems or animated subsystem logic.
*/

#include "IM2800ShipActor.h"

AIM2800ShipActor::AIM2800ShipActor()
{
    ApplyIM2800Defaults();
}

void AIM2800ShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyIM2800Defaults();

    Super::OnConstruction(Transform);

    ApplyIM2800FixedPoints();
}

void AIM2800ShipActor::ApplyIM2800Defaults()
{
    /*
     * CargoC.def / IM2800 values:
     *
     * scale:       2.5
     * chase:       (0, -1000, 200)
     * bridge:      (0, 216, 34)
     * drive loc:   (0, 30, -250)
     * drive port:  (-92, 29, -325)
     * drive port:  (94, 29, -325)
     * drive port:  (-92, -43, -176)
     * drive port:  (94, -43, -176)
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

    bEnableMainEngineEmitters = true;
    bEnableThrusterEmitters = true;

    MainEngineEmitterRelativeScale =
        FVector(1.00f, 0.50f, 0.50f);

    MainEngineEmitterRelativeRotation =
        FRotator(0.0f, 180.0f, 0.0f);

    FocusPointOffset =
        FVector(0.0f, 216.0f, 34.0f);

    BridgePointOffset =
        FVector(0.0f, 216.0f, 34.0f);

    ChasePointOffset =
        FVector(0.0f, -1000.0f, 200.0f);

    NumWeaponMountPoints = 0;
    NumTurretBasePoints = 0;
    NumDockPoints = 0;
    NumLandingPoints = 0;

    WeaponMountPointDefs.Empty();
    TurretBasePointDefs.Empty();
    DockPointDefs.Empty();
    LandingPointDefs.Empty();

    SetActorScale3D(FVector(2.5f, 2.5f, 2.5f));
}

void AIM2800ShipActor::ApplyIM2800FixedPoints()
{
    if (FocusPoint)
    {
        FocusPoint->SetRelativeLocation(FVector(0.0f, 216.0f, 34.0f));
    }

    if (BridgePoint)
    {
        BridgePoint->SetRelativeLocation(FVector(0.0f, 216.0f, 34.0f));
    }

    if (ChasePoint)
    {
        ChasePoint->SetRelativeLocation(FVector(0.0f, -1000.0f, 200.0f));
    }

    if (DriveCenterPoint)
    {
        DriveCenterPoint->SetRelativeLocation(FVector(0.0f, 30.0f, -250.0f));
    }

    if (QuantumPoint)
    {
        QuantumPoint->SetRelativeLocation(FVector(0.0f, 0.0f, -120.0f));
    }

    if (ShieldPoint)
    {
        ShieldPoint->SetRelativeLocation(FVector(0.0f, 20.0f, -80.0f));
    }

    if (SensorPoint)
    {
        SensorPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));
    }

    if (ComputerPointA)
    {
        ComputerPointA->SetRelativeLocation(FVector(20.0f, 16.0f, 80.0f));
    }

    if (ComputerPointB)
    {
        ComputerPointB->SetRelativeLocation(FVector(-20.0f, -16.0f, 80.0f));
    }

    if (NavPoint)
    {
        NavPoint->SetRelativeLocation(FVector(0.0f, 16.0f, 60.0f));
    }
}