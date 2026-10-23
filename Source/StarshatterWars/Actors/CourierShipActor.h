/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         CourierShipActor.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    CourierShipActor applies Courier.def defaults to the
    capital-ship presentation base class.
*/
#pragma once

#include "CoreMinimal.h"
#include "CapitalShipActor.h"
#include "CourierShipActor.generated.h"

UCLASS()
class STARSHATTERWARS_API ACourierShipActor : public ACapitalShipActor
{
    GENERATED_BODY()

public:
    ACourierShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    void ApplyCourierDefaults();
    void ApplyCourierFixedPoints();
};
