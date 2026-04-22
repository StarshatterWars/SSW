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

     /*
      * Keep base class automated. The child provides exact
      * point definitions, so fallback spread logic will not
      * be used for engines/thrusters.
      */
    bAutoRebuildGeneratedComponents = true;
    bRebuildOnConstruction = true;

    /*
     * Camera and framing points
     */
    FocusPointOffset = FVector(0.0f, 216.0f, 34.0f);
    BridgePointOffset = FVector(0.0f, 216.0f, 34.0f);
    ChasePointOffset = FVector(0.0f, -1000.0f, 200.0f);

    /*
     * Explicit engine/thruster counts
     */
    NumMainEnginePoints = 4;
    NumThrusterPoints = 1;
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
        P0.LocalOffset = FVector(-25.0f, 0.0f, -448.0f);
        MainEnginePointDefs.Add(P0);

        FShipPointDef P1;
        P1.LocalOffset = FVector(25.0f, 0.0f, -448.0f);
        MainEnginePointDefs.Add(P1);

        FShipPointDef P2;
        P2.LocalOffset = FVector(-25.0f, 20.0f, -448.0f);
        MainEnginePointDefs.Add(P2);

        FShipPointDef P3;
        P3.LocalOffset = FVector(25.0f, 20.0f, -448.0f);
        MainEnginePointDefs.Add(P3);
    }

    /*
     * Courier.def has one thruster system location.
     */
    ThrusterPointDefs.Empty();
    {
        FShipPointDef T0;
        T0.LocalOffset = FVector(0.0f, 0.0f, 64.0f);
        ThrusterPointDefs.Add(T0);
    }

    /*
     * No explicit weapon/turret/dock/landing layout authored yet.
     */
    WeaponMountPointDefs.Empty();
    TurretBasePointDefs.Empty();
    DockPointDefs.Empty();
    LandingPointDefs.Empty();

    /*
     * Courier.def snippet does not expose navlight entries.
     * Keep a tailored first-pass set here that BP_Courier
     * can edit later in defaults.
     */
    NavLightDefs.Empty();

    {
        FShipNavLightDef Port;
        Port.LocalOffset = FVector(0.0f, -14.25f, 2.0f);
        Port.LocalRotation = FRotator::ZeroRotator;
        Port.Color = FLinearColor::Red;
        Port.Intensity = 12000.0f;
        Port.Radius = 400.0f;
        Port.bBlink = true;
        Port.BlinkInterval = 0.50f;
        NavLightDefs.Add(Port);
    }

    {
        FShipNavLightDef Starboard;
        Starboard.LocalOffset = FVector(0.0f, 14.25f, 2.0f);
        Starboard.LocalRotation = FRotator::ZeroRotator;
        Starboard.Color = FLinearColor::Green;
        Starboard.Intensity = 12000.0f;
        Starboard.Radius = 400.0f;
        Starboard.bBlink = true;
        Starboard.BlinkInterval = 0.50f;
        NavLightDefs.Add(Starboard);
    }

    {
        FShipNavLightDef Dorsal;
        Dorsal.LocalOffset = FVector(12.5f, 0.0f, 11.5f);
        Dorsal.LocalRotation = FRotator::ZeroRotator;
        Dorsal.Color = FLinearColor::White;
        Dorsal.Intensity = 6000.0f;
        Dorsal.Radius = 300.0f;
        Dorsal.bBlink = false;
        Dorsal.BlinkInterval = 1.0f;
        NavLightDefs.Add(Dorsal);
    }

    {
        FShipNavLightDef Ventral;
        Ventral.LocalOffset = FVector(-4.0f, 0.0f, -3.9f);
        Ventral.LocalRotation = FRotator::ZeroRotator;
        Ventral.Color = FLinearColor(0.6f, 0.6f, 1.0f);
        Ventral.Intensity = 6000.0f;
        Ventral.Radius = 300.0f;
        Ventral.bBlink = false;
        Ventral.BlinkInterval = 1.0f;
        NavLightDefs.Add(Ventral);
    }

    /*
     * Courier.def scale
     */
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