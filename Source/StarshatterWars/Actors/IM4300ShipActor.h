/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         IM4300ShipActor.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    IM4300ShipActor applies CargoB.def / IM4300 Freighter
    defaults to the generic ShipActor base class.

    This class is presentation-only. Legacy Starshatter runtime
    remains authoritative for gameplay, AI, physics, weapons,
    mission behavior, and simulation.
*/

#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "IM4300ShipActor.generated.h"

UCLASS()
class STARSHATTERWARS_API AIM4300ShipActor : public AShipActor
{
    GENERATED_BODY()

public:
    AIM4300ShipActor();

    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    void ApplyIM4300Defaults();
    void ApplyIM4300FixedPoints();
};