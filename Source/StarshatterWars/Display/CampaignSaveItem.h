#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CampaignSaveItem.generated.h"

UCLASS()
class STARSHATTERWARS_API UCampaignSaveItem : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly)
    FString Name;

    static UCampaignSaveItem* Make(UObject* Outer, const FString& InName)
    {
        UCampaignSaveItem* Item = NewObject<UCampaignSaveItem>(Outer);
        Item->Name = InName;
        return Item;
    }
};

