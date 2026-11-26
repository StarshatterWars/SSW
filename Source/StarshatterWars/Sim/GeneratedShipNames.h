#pragma once
#include "CoreMinimal.h"

class STARSHATTERWARS_API FGeneratedShipNames
{
public:
 bool Load(const FString& JsonFile,FString& Error);
 FString ForHull(FGuid Id)const;
private:
 TArray<FString> Prefixes,Suffixes;
};
