/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         Station1Actor.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Station1Actor applies FreightXfer.def / Freight Xfer
    defaults to the generic ShipActor base class.

    This class is presentation-only. Legacy Starshatter runtime
    remains authoritative for gameplay, AI, physics, weapons,
    mission behavior, docking, flight decks, and simulation.
*/

#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "Station1Actor.generated.h"

UCLASS()
class STARSHATTERWARS_API AStation1Actor : public AShipActor
{
    GENERATED_BODY()

public:
    AStation1Actor();

    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    void ApplyStation1Defaults();
    void ApplyStation1FixedPoints();
};
