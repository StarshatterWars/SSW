/*Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    GAME
    FILE:         ObjectiveArrivalUtils.cpp
    AUTHOR:       Carlos Bott
    ORIGINAL:     John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Shared Objective Arrival Evaluation Helpers
*/

#include "ObjectiveArrivalUtils.h"
#include "Ship.h"

float
FObjectiveArrivalUtils::GetShipSizeScale(
    const Ship* RuntimeShip)
{
    if (!RuntimeShip)
    {
        return 1.0f;
    }

    const float Radius =
        FMath::Max(
            1.0f,
            static_cast<float>(RuntimeShip->GetRadius()));

    if (Radius >= 1500.0f)
    {
        return 3.0f;
    }

    if (Radius >= 750.0f)
    {
        return 2.0f;
    }

    if (Radius >= 300.0f)
    {
        return 1.5f;
    }

    return 1.0f;
}

FObjectiveArrivalSettings
FObjectiveArrivalUtils::MakeSettings(
    EObjectiveArrivalType ArrivalType,
    const Ship* RuntimeShip)
{
    const float SizeScale =
        GetShipSizeScale(RuntimeShip);

    FObjectiveArrivalSettings Settings;
    Settings.ArrivalType =
        ArrivalType;

    switch (ArrivalType)
    {
    case EObjectiveArrivalType::Dock:
        Settings.ArrivalRadius = 2500.0f * SizeScale;
        Settings.BrakeRadius = 18000.0f * SizeScale;
        Settings.StationKeepingRadius = 1000.0f * SizeScale;
        Settings.CompletionRadius = 1500.0f * SizeScale;
        Settings.MaxArrivalSpeed = 35.0f;
        break;

    case EObjectiveArrivalType::Farcaster:
        Settings.ArrivalRadius = 600.0f;
        Settings.BrakeRadius = 8000.0f;
        Settings.StationKeepingRadius = 2500.0f * SizeScale;
        Settings.CompletionRadius = 4000.0f * SizeScale;
        Settings.MaxArrivalSpeed = 75.0f;
        break;
     
    case EObjectiveArrivalType::Patrol:
        Settings.ArrivalRadius = 8000.0f * SizeScale;
        Settings.BrakeRadius = 20000.0f * SizeScale;
        Settings.StationKeepingRadius = 5000.0f * SizeScale;
        Settings.CompletionRadius = 7000.0f * SizeScale;
        Settings.MaxArrivalSpeed = 250.0f;
        break;

    case EObjectiveArrivalType::Formation:
        Settings.ArrivalRadius = 4000.0f * SizeScale;
        Settings.BrakeRadius = 12000.0f * SizeScale;
        Settings.StationKeepingRadius = 2000.0f * SizeScale;
        Settings.CompletionRadius = 3000.0f * SizeScale;
        Settings.MaxArrivalSpeed = 150.0f;
        break;

    case EObjectiveArrivalType::Generic:
    default:
        Settings.ArrivalRadius = 5000.0f * SizeScale;
        Settings.BrakeRadius = 15000.0f * SizeScale;
        Settings.StationKeepingRadius = 2500.0f * SizeScale;
        Settings.CompletionRadius = 4000.0f * SizeScale;
        Settings.MaxArrivalSpeed = 150.0f;
        break;
    }

    return Settings;
}

FObjectiveArrivalState
FObjectiveArrivalUtils::EvaluateArrival(
    const FVector& ShipLocation,
    const FVector& ShipVelocity,
    const FVector& ObjectiveLocation,
    const FObjectiveArrivalSettings& Settings)
{
    FObjectiveArrivalState State;

    State.Distance =
        FVector::Dist(
            ShipLocation,
            ObjectiveLocation);

    State.Speed =
        ShipVelocity.Size();

    State.bInsideBrakeRadius =
        State.Distance <= Settings.BrakeRadius;

    State.bInsideArrivalRadius =
        State.Distance <= Settings.ArrivalRadius;

    State.bInsideStationKeepingRadius =
        State.Distance <= Settings.StationKeepingRadius;

    State.bComplete =
        State.Distance <= Settings.CompletionRadius &&
        State.Speed <= Settings.MaxArrivalSpeed;

    if (State.bInsideBrakeRadius)
    {
        const float BrakeAlpha =
            FMath::Clamp(
                State.Distance /
                FMath::Max(Settings.BrakeRadius, 1.0f),
                0.0f,
                1.0f);

        State.DesiredThrottleScale =
            FMath::Clamp(
                BrakeAlpha,
                0.0f,
                1.0f);
    }
    else
    {
        State.DesiredThrottleScale =
            1.0f;
    }

    if (State.bInsideArrivalRadius)
    {
        State.DesiredThrottleScale =
            0.0f;
    }

    return State;
}