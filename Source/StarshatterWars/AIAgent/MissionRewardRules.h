#pragma once
#include "GameStructs_economy.h"
struct FMissionRewardQuote
{
    int64 Base=0,Recovery=0,Objectives=0,Time=0;
    int32 CompletionPoints=25;
    int64 Total() const { return Base+Recovery+Objectives+Time; }
};
struct FMissionRewardPolicy
{
    int64 Base=25000,RecoveryBonus=0,ObjectiveBonus=0,TimeBonus=0;
    int32 TimeLimitSeconds=0,CompletionPoints=25;
};
class FMissionRewardCatalog
{
public:
    static const FMissionRewardCatalog& Get();
    bool LoadJson(const FString&,FText& Error);
    FMissionRewardQuote Quote(int32 Campaign,int32 Mission,bool Docked,int32 Secondary,double Elapsed) const;
    bool Award(FEconomyState&,int32 Campaign,int32 Mission,const FMissionRewardQuote&,bool Success,bool FirstCompletion,bool TestReplay) const;
private:
    FMissionRewardPolicy Default;
    TMap<FName,FMissionRewardPolicy> Policies;
};
