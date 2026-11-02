#pragma once
#include "CoreMinimal.h"

// Outcome evidence is independent of score and legacy mission teardown state.
struct FMissionDebriefEvidence
{
    bool bBasicFlightTraining = false;
    bool bDocked = false;
    bool bLost = false;
    int32 RoutePoints = 0;
    int32 CompletedRoutePoints = 0;
    int32 RequiredObjectives = 0;
    int32 CompletedObjectives = 0;
    bool bFailedObjective = false;
};
struct FMissionDebriefResult
{
    bool bSuccess = false;
    FString Outcome;
    FString Reason;
};
FMissionDebriefResult EvaluateMissionDebrief(const FMissionDebriefEvidence& Evidence);
