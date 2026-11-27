#pragma once
#include "CoreMinimal.h"
#include "GameStructs_skills.h"
#include "GameStructs_economy.h"

enum class ESkillCommand : uint8 { Book, Advanced, Queue, Cancel, Instant, Advance };
// Catalog, authority and station permission are frozen by the game-thread adapter.
struct FNativeSkillCommand
{
    ESkillCommand Action=ESkillCommand::Advance;
    FGuid Receipt,Entry;
    FName Skill,Empire;
    int32 Rank=0,Target=1;
    int64 NowUtc=0,MaximumCost=0;
    TMap<FName,FSkillDefinition> Definitions;
    TMap<FName,FSkillProgress> Source;
    TArray<FSkillTrainingEntry> SourceQueue;
};
struct FNativeSkillReply
{
    bool bSucceeded=false,bChanged=false;
    FText Reason;
    TMap<FName,FSkillProgress> Source,Skills;
    TArray<FSkillTrainingEntry> SourceQueue,Queue;
};
class SkillCommandAI
{
public:
    static FNativeSkillReply Execute(FEconomyState&,const FNativeSkillCommand&);
};
