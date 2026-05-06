/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         CourierShipActor.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    CourierShipActor applies Courier.def values to the
    ShipActor base class.

    This class remains structural only. It does not
    implement gameplay systems or animated subsystem logic.
*/

#include "CourierShipActor.h"

ACourierShipActor::ACourierShipActor()
{
    ApplyCourierDefaults();
}

void ACourierShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyCourierDefaults();

    Super::OnConstruction(Transform);

    ApplyCourierFixedPoints();
}

void ACourierShipActor::ApplyCourierDefaults()
{
    /*
     * Courier.def values:
     *
     * scale:   1.2
     * chase:   (0, -1000, 200)
     * bridge:  (0, 216, 34)
     * drive:   loc (0, 10, -370)
     * ports:   (-25,  0,-448)
     *          ( 25,  0,-448)
     *          (-25, 20,-448)
     *          ( 25, 20,-448)
     * thruster loc: (0, 0, 64)
     * quantum loc:  (0, 0, -120)
     * shield loc:   (0, 20, -80)
     * sensor loc:   (0, 0, 180)
     * computer 1:   (20, 16, 80)
     * computer 2:   (-20, -16, 80)
     * nav loc:      (0, 16, 60)
     */

    bAutoRebuildGeneratedComponents = true;
    bRebuildOnConstruction = true;

    /*
     * Turn FX on.
     * Assign the Niagara systems in BP_Courier defaults.
     */
    bEnableMainEngineEmitters = true;
    bEnableThrusterEmitters = true;

    /*
     * These are safe first-pass defaults.
     * Adjust once you see how the Niagara system is authored.
     */
    MainEngineEmitterRelativeScale = FVector(1.00f, 0.40f, 0.40f);
    MainEngineEmitterRelativeRotation = FRotator(0.0f, 180.0f, 0.0f);

    ThrusterEmitterRelativeScale = FVector(0.50f, 0.20f, 0.20f);
    ThrusterEmitterRelativeRotation = FRotator(0.0f, 0.0f, 0.0f);

    /*
     * Camera and framing points
     */
    FocusPointOffset = FVector(0.0f, 216.0f, 34.0f);
    BridgePointOffset = FVector(0.0f, 216.0f, 34.0f);
    ChasePointOffset = FVector(0.0f, -1000.0f, 200.0f);

    /*
     * Explicit generated point counts
     */
    NumMainEnginePoints = 4;
    NumThrusterPoints = 0;
    NumWeaponMountPoints = 0;
    NumTurretBasePoints = 0;
    NumDockPoints = 0;
    NumLandingPoints = 0;

    /*
     * Exact engine points from Courier.def drive ports
     */
    MainEnginePointDefs.Empty();
    {
        FShipPointDef P0;
        P0.LocalOffset = FVector(-550.0f, 31.f, -3.0f);
        MainEnginePointDefs.Add(P0);

        FShipPointDef P1;
        P1.LocalOffset = FVector(-550.0f, 31.0f, 20.0f);
        MainEnginePointDefs.Add(P1);

        FShipPointDef P2;
        P2.LocalOffset = FVector(-550.0f, -31.01f, -3.0f);
        MainEnginePointDefs.Add(P2);

        FShipPointDef P3;
        P3.LocalOffset = FVector(-550.0f, -31.0f, 22.0f);
        MainEnginePointDefs.Add(P3);
    }

    /*
     * Courier.def only gives one thruster system location.
     * Build a practical first-pass maneuvering set around it.
     */
    ThrusterPointDefs.Empty();
    {
        // central forward
        {
            FShipPointDef T;
            T.LocalOffset = FVector(460.0f, 0.0f, 10.0f);
            T.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
            ThrusterPointDefs.Add(T);
        }

        // central aft
        {
            FShipPointDef T;
            T.LocalOffset = FVector(-480.0f, 0.0f, 25.f);
            T.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
            ThrusterPointDefs.Add(T);
        }

        // port
        {
            FShipPointDef T;
            T.LocalOffset = FVector(0.0f, -140.0f, 20.0f);
            T.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
            ThrusterPointDefs.Add(T);
        }

        // starboard
        {
            FShipPointDef T;
            T.LocalOffset = FVector(0.0f, 140.0f, 20.0f);
            T.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
            ThrusterPointDefs.Add(T);
        }

        // dorsal
        {
            FShipPointDef T;
            T.LocalOffset = FVector(4.0f, 0.0f, 115.0f);
            T.LocalRotation = FRotator(90.0f, 0.0f, 0.0f);
            ThrusterPointDefs.Add(T);
        }

        // ventral
        {
            FShipPointDef T;
            T.LocalOffset = FVector(-2.0f, 0.0f, -55.0f);
            T.LocalRotation = FRotator(-90.0f, 0.0f, 0.0f);
            ThrusterPointDefs.Add(T);
        }
    }

    WeaponMountPointDefs.Empty();
    TurretBasePointDefs.Empty();
    DockPointDefs.Empty();
    LandingPointDefs.Empty();

    BuildNavLightsFromRuntime();
    SetActorScale3D(FVector(1.2f, 1.2f, 1.2f));
}

void ACourierShipActor::ApplyCourierFixedPoints()
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
        DriveCenterPoint->SetRelativeLocation(FVector(0.0f, 10.0f, -370.0f));
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