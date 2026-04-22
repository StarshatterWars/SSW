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
    CourierShipActor applies Courier.def layout values
    to ShipActor.
*/

#include "CourierShipActor.h"
#include "Components/SceneComponent.h"

ACourierShipActor::ACourierShipActor()
{
    ApplyCourierDefDefaults();
}

void ACourierShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyCourierDefDefaults();
    Super::OnConstruction(Transform);

    /*
     * Override generated points with exact Courier.def values.
     */

    BridgePointOffset = FVector(0.0f, 216.0f, 34.0f);
    ChasePointOffset = FVector(0.0f, -1000.0f, 200.0f);

    if (BridgePoint)
    {
        BridgePoint->SetRelativeLocation(BridgePointOffset);
    }

    if (FocusPoint)
    {
        FocusPoint->SetRelativeLocation(BridgePointOffset);
    }

    if (ChasePoint)
    {
        ChasePoint->SetRelativeLocation(ChasePointOffset);
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

    /*
     * Exact Courier drive exhaust ports.
     */
    if (MainEnginePoints.Num() >= 4)
    {
        MainEnginePoints[0]->SetRelativeLocation(FVector(-25.0f, 0.0f, -448.0f));
        MainEnginePoints[1]->SetRelativeLocation(FVector(25.0f, 0.0f, -448.0f));
        MainEnginePoints[2]->SetRelativeLocation(FVector(-25.0f, 20.0f, -448.0f));
        MainEnginePoints[3]->SetRelativeLocation(FVector(25.0f, 20.0f, -448.0f));
    }

    /*
     * Courier only has one thruster system location in the def.
     * Keep one real point and let any extras remain unused.
     */
    if (ThrusterPoints.Num() >= 1)
    {
        ThrusterPoints[0]->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
    }

    /*
     * Generic nav lights for now. Courier.def snippet does not
     * expose explicit navlight entries, so these remain authored
     * defaults until you parse navlights from ship design data.
     */
    NavLightDefs.Empty();

    {
        FShipNavLightDef Port;
        Port.LocalOffset = FVector(0.0f, -90.0f, 20.0f);
        Port.Color = FLinearColor::Red;
        Port.Intensity = 3000.0f;
        Port.Radius = 300.0f;
        NavLightDefs.Add(Port);
    }

    {
        FShipNavLightDef Starboard;
        Starboard.LocalOffset = FVector(0.0f, 90.0f, 20.0f);
        Starboard.Color = FLinearColor::Green;
        Starboard.Intensity = 3000.0f;
        Starboard.Radius = 300.0f;
        NavLightDefs.Add(Starboard);
    }

    {
        FShipNavLightDef Dorsal;
        Dorsal.LocalOffset = FVector(-40.0f, 0.0f, 60.0f);
        Dorsal.Color = FLinearColor::White;
        Dorsal.Intensity = 2500.0f;
        Dorsal.Radius = 250.0f;
        NavLightDefs.Add(Dorsal);
    }

    {
        FShipNavLightDef Ventral;
        Ventral.LocalOffset = FVector(-40.0f, 0.0f, -60.0f);
        Ventral.Color = FLinearColor(0.6f, 0.6f, 1.0f);
        Ventral.Intensity = 2000.0f;
        Ventral.Radius = 250.0f;
        NavLightDefs.Add(Ventral);
    }

    RebuildNavLights();
}

void ACourierShipActor::ApplyCourierDefDefaults()
{
    /*
     * Courier.def:
     * chase  = (0, -1000, 200)
     * bridge = (0, 216, 34)
     * drive ports = 4
     * thruster loc = (0, 0, 64)
     */

    NumMainEnginePoints = 4;
    NumThrusterPoints = 1;

    MainEngineSpread = 25.0f;
    ThrusterSpread = 0.0f;

    MainEngineX = -448.0f;
    ThrusterX = 64.0f;

    FocusPointOffset = FVector(0.0f, 216.0f, 34.0f);
    BridgePointOffset = FVector(0.0f, 216.0f, 34.0f);
    ChasePointOffset = FVector(0.0f, -1000.0f, 200.0f);

    /*
     * Structural systems present in the def but not functional yet:
     * reactor/power, drive, thruster, quantum, shield, sensor,
     * computers, nav.
     */

    SetActorScale3D(FVector(1.2f));
}