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
    UPROPERTY(BlueprintReadOnly, SaveGame) bool bHasCondition = false;
    UPROPERTY(BlueprintReadOnly, SaveGame) float ConditionPercent = 100;
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 ConditionStatus = 0;
    UPROPERTY(BlueprintReadOnly, SaveGame) float MaintenanceSeconds = 0;
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 SpareCount = 0;
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 JerryCount = 0;
};

USTRUCT(BlueprintType)
struct FHangarSystemCondition
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) int32 SystemIndex = INDEX_NONE;
    UPROPERTY(SaveGame) FName SystemName;
    UPROPERTY(SaveGame) float Availability = 1;
    UPROPERTY(SaveGame) float Safety = 1;
    UPROPERTY(SaveGame) float Stability = 1;
    UPROPERTY(SaveGame) float SafetyOverload = 0;
    UPROPERTY(SaveGame) float NetAvailability = -1;
    UPROPERTY(SaveGame) int32 Status = 0;
};

UENUM(BlueprintType)
enum class EShipStoreCategory : uint8 { Hulls, Skins, Components, Missiles, Drones };

USTRUCT(BlueprintType)
struct FStoredShipEquipment
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, SaveGame) FName CatalogId;
    UPROPERTY(BlueprintReadOnly, SaveGame) EShipStoreCategory Category = EShipStoreCategory::Components;
    UPROPERTY(BlueprintReadOnly, SaveGame) FString Location;
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 Quantity = 0;
    UPROPERTY(BlueprintReadOnly, SaveGame) FHangarComponent ComponentState;
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
    UPROPERTY(BlueprintReadOnly, SaveGame) bool bHasCondition = false;
    UPROPERTY(BlueprintReadOnly, SaveGame) float HullCondition = 1;
    UPROPERTY(SaveGame) TArray<FHangarSystemCondition> SystemConditions;
    UPROPERTY(SaveGame) TArray<int32> RepairQueue;
    UPROPERTY(BlueprintReadOnly, SaveGame) TArray<FHangarComponent> Components;
    UPROPERTY(BlueprintReadOnly, SaveGame) TArray<int32> LoadoutStations;
    UPROPERTY(BlueprintReadOnly, SaveGame) FString LoadoutName;
    UPROPERTY(BlueprintReadOnly, SaveGame) FName SelectedSkin;
    UPROPERTY(SaveGame) TMap<int32,int32> SavedWeaponAmmo;
    UPROPERTY(SaveGame) TMap<int32,FName> SavedWeaponTypes;
    UPROPERTY(SaveGame) FName OrdnanceIssueKey;
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
