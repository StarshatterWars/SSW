#pragma once
#include "CoreMinimal.h"

struct FSSWMissionArcStep
{
    int32 MissionId = INDEX_NONE;
    FName AgentId, Chapter, ChoiceGroup;
    TArray<int32> NextMissionIds;
    FString Title, Issuer, System, Region, Brief;
};
struct FSSWMissionArc
{
    FName Id;
    FString Title, Empire, Description;
    TArray<FSSWMissionArcStep> Steps;
};
struct FSSWMissionArcProgress
{
    const FSSWMissionArc* Arc = nullptr;
    TArray<int32> Available;
    bool bComplete=false;
    int32 Selected = INDEX_NONE, Current = INDEX_NONE, Completed = 0;
    bool bEligible = true, bSelectedComplete = false;
};

// Chapter graphs reuse authored regional missions. They do not
// create campaign progression, rewards, mission copies or an independent save.
class FMissionArcCatalog
{
public:
    static const FMissionArcCatalog& Get();
    bool LoadJson(const FString& Json, FText& Error);
    bool IsLoaded() const { return bLoaded; }
    const TArray<FSSWMissionArc>& GetArcs() const { return Arcs; }
    FSSWMissionArcProgress Evaluate(int32 MissionId, TFunctionRef<bool(int32)> Completed, TFunction<bool(int32)> Chosen = {}) const;
    FText Requirement(int32 MissionId, TFunctionRef<bool(int32)> Completed, TFunction<bool(int32)> Chosen = {}) const;
    FText Status(int32 MissionId, TFunctionRef<bool(int32)> Completed, TFunction<bool(int32)> Chosen = {}) const;
private:
    bool bLoaded = false;
    TArray<FSSWMissionArc> Arcs;
};
