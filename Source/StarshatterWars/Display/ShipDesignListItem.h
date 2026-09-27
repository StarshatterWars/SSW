#pragma once
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/Object.h"
#include "ShipDesignListItem.generated.h"

UCLASS()
class STARSHATTERWARS_API UShipDesignListItem : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY() FName RowName;
    UPROPERTY() FString ShipName;
    UPROPERTY() FString DisplayName;
    UPROPERTY() FString Abrv;
    UPROPERTY() FString ShipClass;
    UPROPERTY() FString Description;

    UPROPERTY() TObjectPtr<UDataTable> SourceTable = nullptr;

    FString GetLabel() const
    {
        return Abrv.IsEmpty()
            ? DisplayName
            : FString::Printf(TEXT("%s %s"), *Abrv, *DisplayName);
    }
};