#pragma once
#include "GameStructs_Contracts.h"
#include "GameStructs_contractstate.generated.h"

UENUM()
enum class EContractState : uint8
{
    Draft, Open, InProgress, AwaitingClaims, Completed, Failed, Expired
};

USTRUCT()
struct FContractTransaction
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FGuid Id;
    UPROPERTY(SaveGame) FSSWContractTerms Terms;
    UPROPERTY(SaveGame) FName Issuer;
    UPROPERTY(SaveGame) FName Acceptor;
    UPROPERTY(SaveGame) FName HighBidder;
    UPROPERTY(SaveGame) int64 HighBid = 0;
    UPROPERTY(SaveGame) int64 Buyout = 0;
    UPROPERTY(SaveGame) FDateTime ExpiresAt;
    UPROPERTY(SaveGame) FDateTime DeliverBy;
    UPROPERTY(SaveGame) bool bSellerClaimed = false;
    UPROPERTY(SaveGame) bool bBuyerClaimed = false;
    UPROPERTY(SaveGame) EContractState State = EContractState::Draft;
};

