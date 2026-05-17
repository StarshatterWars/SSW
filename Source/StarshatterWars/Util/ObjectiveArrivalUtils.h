/*Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    GAME
    FILE:         ObjectiveArrivalUtils.h
    AUTHOR:       Carlos Bott
    ORIGINAL:     John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Shared Objective Arrival Evaluation Helpers
*/

#pragma once

#include "CoreMinimal.h"
#include "GameStructs.h"

class Ship;

class FObjectiveArrivalUtils
{
public:

    static FObjectiveArrivalSettings MakeSettings(
        EObjectiveArrivalType ArrivalType,
        const Ship* RuntimeShip);

    static FObjectiveArrivalState EvaluateArrival(
        const FVector& ShipLocation,
        const FVector& ShipVelocity,
        const FVector& ObjectiveLocation,
        const FObjectiveArrivalSettings& Settings);

private:

    static float GetShipSizeScale(
        const Ship* RuntimeShip);
}; #pragma once
