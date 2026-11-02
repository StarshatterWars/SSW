#include "MissionDebriefResult.h"

FMissionDebriefResult EvaluateMissionDebrief(const FMissionDebriefEvidence& E)
{
    if (E.bLost) return {false,TEXT("FAILED"),TEXT("Player ship lost or destroyed in a collision.")};
    if (E.bFailedObjective) return {false,TEXT("FAILED"),TEXT("A primary objective failed or was aborted.")};
    if (E.bBasicFlightTraining)
    {
        const bool Success = E.bDocked && E.RoutePoints == 5 && E.CompletedRoutePoints == E.RoutePoints;
        return {Success,Success ? TEXT("SUCCESS") : TEXT("INCOMPLETE"),
            FString::Printf(TEXT("Training route: %d/%d waypoints complete. Recovery: %s."),
                E.CompletedRoutePoints,E.RoutePoints,E.bDocked ? TEXT("docked safely") : TEXT("not docked"))};
    }
    if (E.RequiredObjectives > 0)
    {
        const bool Success = E.CompletedObjectives == E.RequiredObjectives;
        return {Success,Success ? TEXT("SUCCESS") : TEXT("INCOMPLETE"),
            FString::Printf(TEXT("Primary objectives: %d/%d complete."),E.CompletedObjectives,E.RequiredObjectives)};
    }
    return {false,TEXT("NOT ASSESSED"),TEXT("No measurable primary objectives are defined. Points and recovery do not establish success.")};
}
