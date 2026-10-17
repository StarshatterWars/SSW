#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CampaignSlotSaveGame.generated.h"

UCLASS()
class STARSHATTERWARS_API UCampaignSlotSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY() FString SlotName;
    UPROPERTY() FString CampaignName;
    UPROPERTY() FString SaveText;
};

UCLASS()
class STARSHATTERWARS_API UCampaignSaveIndex : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY() TArray<FString> SlotNames;
};
