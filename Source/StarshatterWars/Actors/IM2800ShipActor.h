#pragma once
/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         IM2800ShipActor.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    IM2800ShipActor applies CargoC.def / IM2800 Cargo
    defaults to the generic ShipActor base class.

    This class is presentation-only. Legacy Starshatter runtime
    remains authoritative for gameplay, AI, physics, weapons,
    mission behavior, and simulation.
*/

#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "IM2800ShipActor.generated.h"

UCLASS()
class STARSHATTERWARS_API AIM2800ShipActor : public AShipActor
{
    GENERATED_BODY()

public:
    AIM2800ShipActor();

    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    void ApplyIM2800Defaults();
    void ApplyIM2800FixedPoints();
};