#pragma once
#include "CoreMinimal.h"
#include "MiningCrystalTypes.generated.h"

USTRUCT()
struct FMiningCrystalEquipment
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FName Crystal;
    UPROPERTY(SaveGame) FName FittedFamily;
    UPROPERTY(SaveGame) double ExtractedTonnes = 0;
    UPROPERTY(SaveGame) double ServiceLifeTonnes = 3000;
    UPROPERTY(SaveGame) bool bSelected = false;
};
