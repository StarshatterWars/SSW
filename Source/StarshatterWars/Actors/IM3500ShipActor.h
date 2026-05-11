/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         IM3500ShipActor.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    IM3500ShipActor applies CargoA.def / IM3500 Freighter
    defaults to the generic ShipActor base class.

    This class is presentation-only. Legacy Starshatter runtime
    remains authoritative for gameplay, AI, physics, weapons,
    mission behavior, and simulation.
*/

#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "IM3500ShipActor.generated.h"

UCLASS()
class STARSHATTERWARS_API AIM3500ShipActor : public AShipActor
{
    GENERATED_BODY()

public:
    AIM3500ShipActor();

    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    void ApplyIM3500Defaults();
    void ApplyIM3500FixedPoints();
};
