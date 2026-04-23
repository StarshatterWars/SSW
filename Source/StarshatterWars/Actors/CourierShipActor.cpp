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
    MainEngineEmitterRelativeScale = FVector(0.10f, 0.04f, 0.04f);
    MainEngineEmitterRelativeRotation = FRotator(0.0f, 180.0f, 0.0f);

    ThrusterEmitterRelativeScale = FVector(0.35f, 0.35f, 0.35f);
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
        P0.LocalOffset = FVector(-55.0f, 3.1f, -0.3f);
        MainEnginePointDefs.Add(P0);

        FShipPointDef P1;
        P1.LocalOffset = FVector(-55.0f, 3.1f, 2.0f);
        MainEnginePointDefs.Add(P1);

        FShipPointDef P2;
        P2.LocalOffset = FVector(-55.0f, -3.1f, -0.3f);
        MainEnginePointDefs.Add(P2);

        FShipPointDef P3;
        P3.LocalOffset = FVector(-55.0f, -3.1f, 2.2f);
        MainEnginePointDefs.Add(P3);
    }

    /*
     * Courier.def only gives one thruster system location.
     * Build a practical first-pass maneuvering set around it.
     */
    ThrusterPointDefs.Empty();
    {
        /*FShipPointDef T0;
        T0.LocalOffset = FVector(0.0f, 0.0f, 64.0f);
        ThrusterPointDefs.Add(T0);

        FShipPointDef T1;
        T1.LocalOffset = FVector(36.0f, 0.0f, 64.0f);
        ThrusterPointDefs.Add(T1);

        FShipPointDef T2;
        T2.LocalOffset = FVector(-36.0f, 0.0f, 64.0f);
        ThrusterPointDefs.Add(T2);

        FShipPointDef T3;
        T3.LocalOffset = FVector(0.0f, 26.0f, 64.0f);
        ThrusterPointDefs.Add(T3);

        FShipPointDef T4;
        T4.LocalOffset = FVector(0.0f, -26.0f, 64.0f);
        ThrusterPointDefs.Add(T4);

        FShipPointDef T5;
        T5.LocalOffset = FVector(0.0f, 0.0f, 28.0f);
        ThrusterPointDefs.Add(T5);*/
    }

    WeaponMountPointDefs.Empty();
    TurretBasePointDefs.Empty();
    DockPointDefs.Empty();
    LandingPointDefs.Empty();

    /*
     * Nav lights
     */
    NavLightDefs.Empty();

    {
        FShipNavLightDef Port;
        Port.LocalOffset = FVector(0.0f, -14.25f, 2.0f);
        Port.LocalRotation = FRotator::ZeroRotator;
        Port.Color = FLinearColor::Red;
        Port.Intensity = 12000.0f;
        Port.Radius = 400.0f;
        Port.Mode = EShipNavLightMode::Blink;
        Port.BlinkInterval = 0.50f;
        Port.PhaseOffset = 0.00f;
        NavLightDefs.Add(Port);
    }

    {
        FShipNavLightDef Starboard;
        Starboard.LocalOffset = FVector(0.0f, 14.25f, 2.0f);
        Starboard.LocalRotation = FRotator::ZeroRotator;
        Starboard.Color = FLinearColor::Green;
        Starboard.Intensity = 12000.0f;
        Starboard.Radius = 400.0f;
        Starboard.Mode = EShipNavLightMode::Blink;
        Starboard.BlinkInterval = 0.50f;
        Starboard.PhaseOffset = 0.25f;
        NavLightDefs.Add(Starboard);
    }

    {
        FShipNavLightDef Dorsal;
        Dorsal.LocalOffset = FVector(12.5f, 0.0f, 11.5f);
        Dorsal.LocalRotation = FRotator::ZeroRotator;
        Dorsal.Color = FLinearColor::White;
        Dorsal.Intensity = 6000.0f;
        Dorsal.Radius = 300.0f;
        Dorsal.Mode = EShipNavLightMode::Steady;
        Dorsal.BlinkInterval = 1.0f;
        Dorsal.PhaseOffset = 0.0f;
        NavLightDefs.Add(Dorsal);
    }

    {
        FShipNavLightDef Ventral;
        Ventral.LocalOffset = FVector(-4.0f, 0.0f, -3.9f);
        Ventral.LocalRotation = FRotator::ZeroRotator;
        Ventral.Color = FLinearColor(0.6f, 0.6f, 1.0f);
        Ventral.Intensity = 6000.0f;
        Ventral.Radius = 300.0f;
        Ventral.Mode = EShipNavLightMode::Steady;
        Ventral.BlinkInterval = 1.0f;
        Ventral.PhaseOffset = 0.0f;
        NavLightDefs.Add(Ventral);
    }

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