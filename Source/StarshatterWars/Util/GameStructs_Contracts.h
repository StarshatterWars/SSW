#pragma once

#include "CoreMinimal.h"
#include "GameStructs_Contracts.generated.h"

UENUM(BlueprintType)
enum class ESSWContractType : uint8
{
    Courier,
    ItemExchange,
    Auction
};

USTRUCT(BlueprintType)
struct FSSWContractItem
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract")
    FName ItemId;

    // Uses the cargo catalog unit for discrete goods or bulk cargo.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract")
    double Quantity = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract")
    FName Unit;
};

USTRUCT(BlueprintType)
struct FSSWContractTerms
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract")
    ESSWContractType Type = ESSWContractType::Courier;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract")
    bool bPrivate = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract")
    FName AssignedToId;

    // Time allowed to accept the offer, then time allowed to fulfill it.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract")
    int32 OfferExpiryMinutes = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract")
    int32 CompletionDays = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract")
    FName PickupStationId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract")
    FName DestinationStationId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract")
    int64 PriceCredits = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract")
    int64 RewardCredits = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract")
    int64 CollateralCredits = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract")
    FString Title;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract")
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract")
    TArray<FSSWContractItem> OfferedItems;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract")
    FName PickupDivisionId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract")
    TArray<FSSWContractItem> RequestedItems;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract")
    bool bOnBehalfOfCorporation = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contract")
    bool bMultiContract = false;
};
