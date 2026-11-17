#pragma once
#include "CoreMinimal.h"
#include "GameStructs_insurance.generated.h"

UENUM(BlueprintType)
enum class EShipInsuranceTier : uint8
{
    Uninsured, Basic, Standard, Bronze, Silver, Gold, Platinum, Lifetime
};

USTRUCT(BlueprintType)
struct FShipInsurancePolicy
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) EShipInsuranceTier Tier = EShipInsuranceTier::Uninsured;
    UPROPERTY(BlueprintReadOnly, SaveGame) int64 StartsUtc = 0;
    UPROPERTY(BlueprintReadOnly, SaveGame) int64 ExpiresUtc = 0;
    UPROPERTY(BlueprintReadOnly, SaveGame) int64 InsuredValue = 0;
    UPROPERTY(BlueprintReadOnly, SaveGame) bool bSettled = false;
};

USTRUCT(BlueprintType)
struct FHangarComponent
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, SaveGame) FGuid ItemId;
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 SystemIndex = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 ComponentIndex = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly, SaveGame) FName SystemDesign;
    UPROPERTY(BlueprintReadOnly, SaveGame) FString SystemName;
    UPROPERTY(BlueprintReadOnly, SaveGame) FName Component;
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 Level = 3;
};

USTRUCT(BlueprintType)
struct FPlayerHangarShip
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, SaveGame) FGuid HullId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName Design;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FString Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FString Location;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int64 HullValue = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int64 InstalledComponentValue = 0;
    UPROPERTY(BlueprintReadOnly, SaveGame) bool bCashPurchased = false;
    UPROPERTY(BlueprintReadOnly, SaveGame) bool bAssigned = false;
    UPROPERTY(BlueprintReadOnly, SaveGame) bool bLost = false;
    UPROPERTY(BlueprintReadOnly, SaveGame) FShipInsurancePolicy Insurance;
    UPROPERTY(BlueprintReadOnly, SaveGame) TArray<FHangarComponent> Components;
    UPROPERTY(BlueprintReadOnly, SaveGame) TArray<int32> LoadoutStations;
    UPROPERTY(BlueprintReadOnly, SaveGame) FString LoadoutName;
};

USTRUCT(BlueprintType)
struct FShipInsuranceQuote
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) EShipInsuranceTier Tier = EShipInsuranceTier::Uninsured;
    UPROPERTY(BlueprintReadOnly) int64 Premium = 0;
    UPROPERTY(BlueprintReadOnly) int64 Payout = 0;
    UPROPERTY(BlueprintReadOnly) float Coverage = 0;
};
