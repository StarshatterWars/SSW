#pragma once
#include "CoreMinimal.h"
#include "GameStructs_economy.h"
class AwardCreditAI
{
public:
 static const AwardCreditAI& Get();
 bool LoadJson(const FString&,FText& Error);
 int64 Credits(bool Rank,int32 Id)const;
 bool Grant(FEconomyState&,bool Rank,int32 Id)const;
private:
 TMap<int32,int64> Ranks,Medals;
};
