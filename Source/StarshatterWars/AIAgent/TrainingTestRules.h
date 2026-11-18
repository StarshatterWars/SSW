#pragma once
#include "AgentAI.h"
// Test-session progress never consults or clears a pilot's saved campaign history.
struct FTrainingTestRules
{
    static bool IsLesson(int32 Id) { return Id >= 101 && Id <= 107; }
    static int32 RequiredRank(int32 Id) { return Id == 104 ? 1 : Id == 105 ? 3 : Id == 106 ? 4 : Id == 107 ? 6 : 0; }
    static FName RequiredSkill(int32 Id)
    {
        return Id == 105 ? FName(TEXT("frigate_operations")) : Id == 106 ? FName(TEXT("destroyer_operations")) :
            Id == 107 ? FName(TEXT("carrier_operations")) : NAME_None;
    }
    static bool Unlocked(int32 Id, int32 Rank, bool bHasSkill, bool /*bTestSession*/)
    {
        return IsLesson(Id) && (Rank >= RequiredRank(Id) && bHasSkill);
    }
    static TArray<int32> Eligible(const AgentAI& Agent, FName Pilot)
    {
        if (!Agent.GetQueue(Pilot).IsEmpty()) return {};
        for (int32 Id : {101, 102, 103, 104, 105, 106, 107}) if (!Agent.IsCompleted(Pilot, Id)) return {Id};
        return {};
    }
};
