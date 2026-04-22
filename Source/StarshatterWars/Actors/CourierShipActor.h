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
    generic ShipActor base class.

    This class is intended to be the native parent for
    BP_Courier so that the Blueprint only needs to supply
    visual assets and FX.
*/

#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "CourierShipActor.generated.h"

UCLASS()
class STARSHATTERWARS_API ACourierShipActor : public AShipActor
{
    GENERATED_BODY()

public:
    ACourierShipActor();

    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    void ApplyCourierDefDefaults();
};
