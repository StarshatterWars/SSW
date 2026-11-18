#pragma once
#include "CoreMinimal.h"
#include "GameStructs_contractstate.h"
#include "GameStructs_economy.generated.h"

UENUM()
enum class EIndustryJobState : uint8 { Queued, Running, Blocked, Completed, Cancelled };

// Quantities are catalog units; mass is tonnes per unit. IDs survive saves.
USTRUCT(BlueprintType)
struct FCommodityAmount
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame) FName Item;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame) double Quantity = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame) double UnitTonnes = 1;
};
USTRUCT()
struct FEconomyItem
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FGuid Id;
    UPROPERTY(SaveGame) FName Type;
    UPROPERTY(SaveGame) double Quantity = 0;
    UPROPERTY(SaveGame) double UnitTonnes = 1;
};
USTRUCT()
struct FEconomyInventory
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FGuid Id;
    UPROPERTY(SaveGame) FName Owner;
    UPROPERTY(SaveGame) FName System;
    UPROPERTY(SaveGame) bool bMarket = false;
    UPROPERTY(SaveGame) double CapacityTonnes = 0;
    UPROPERTY(SaveGame) double ReservedOutputTonnes = 0;
    UPROPERTY(SaveGame) TArray<FEconomyItem> Items;
    UPROPERTY(SaveGame) TSet<FName> ManifestItems;
};
USTRUCT()
struct FEconomyReservation
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FGuid Id;
    UPROPERTY(SaveGame) FName Inventory;
    UPROPERTY(SaveGame) FName Owner;
    UPROPERTY(SaveGame) FName Item;
    UPROPERTY(SaveGame) double Quantity = 0;
};
USTRUCT()
struct FBlueprintInstance
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FGuid Id;
    UPROPERTY(SaveGame) FName Owner;
    UPROPERTY(SaveGame) FName Recipe;
    UPROPERTY(SaveGame) FName Facility;
    UPROPERTY(SaveGame) bool bOriginal = true;
    UPROPERTY(SaveGame) int32 RunsRemaining = 0;
    UPROPERTY(SaveGame) double MaterialEfficiency = 0;
    UPROPERTY(SaveGame) double TimeEfficiency = 0;
};
USTRUCT()
struct FMiningLease
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FName Claimant;
    UPROPERTY(SaveGame) double ExpiresAt = 0;
};
USTRUCT()
struct FIndustryJob
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FGuid Id;
    UPROPERTY(SaveGame) FName Owner;
    UPROPERTY(SaveGame) FName Facility;
    UPROPERTY(SaveGame) FName Input;
    UPROPERTY(SaveGame) FName Output;
    UPROPERTY(SaveGame) FName Recipe;
    UPROPERTY(SaveGame) FGuid Blueprint;
    UPROPERTY(SaveGame) int32 Runs = 1;
    UPROPERTY(SaveGame) TArray<FCommodityAmount> Materials;
    UPROPERTY(SaveGame) TArray<FCommodityAmount> Products;
    UPROPERTY(SaveGame) double Duration = 0;
    UPROPERTY(SaveGame) double Remaining = 0;
    UPROPERTY(SaveGame) EIndustryJobState State = EIndustryJobState::Queued;
};
USTRUCT()
struct FEconomyTransaction
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FGuid Id;
    UPROPERTY(SaveGame) FName Reason;
    UPROPERTY(SaveGame) FName From;
    UPROPERTY(SaveGame) FName To;
    UPROPERTY(SaveGame) int64 Credits = 0;
};
USTRUCT()
struct FEconomyState
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) TMap<FName, FEconomyInventory> Inventories;
    UPROPERTY(SaveGame) TMap<FName, int64> Wallets;
    UPROPERTY(SaveGame) TMap<FGuid, FEconomyReservation> Reservations;
    UPROPERTY(SaveGame) TMap<FName, FContractTransaction> Contracts;
    UPROPERTY(SaveGame) TArray<FIndustryJob> Jobs;
    UPROPERTY(SaveGame) TMap<FGuid, FBlueprintInstance> Blueprints;
    UPROPERTY(SaveGame) TMap<FGuid, FGuid> BlueprintReceipts;
    UPROPERTY(SaveGame) TMap<FName, FMiningLease> MiningLeases;
    UPROPERTY(SaveGame) TArray<FEconomyTransaction> Journal;
    // Retained separately from bounded display history: retries cannot pay twice.
    UPROPERTY(SaveGame) TSet<FGuid> CommittedTransactions;
    UPROPERTY(SaveGame) TSet<FName> InitializedMarkets;
    UPROPERTY(SaveGame) bool bLegacyWalletMigrated = false;
};

// Called by authoritative simulation services. UI reads state; never ticks industry.
class STARSHATTERWARS_API FEconomyLedger
{
public:
    static FName Pilot() { return FName(TEXT("Pilot")); }
    static FEconomyInventory* CreateInventory(FEconomyState&, FName Key, FName Owner, FName System, double Capacity);
    static bool ReconcileManifest(FEconomyState&, FName Inventory, const TArray<FCommodityAmount>&);
    static double Used(const FEconomyInventory&);
    static double Quantity(const FEconomyState&, FName Inventory, FName Item);
    static double Available(const FEconomyState&, FName Inventory, FName Item);
    static bool Deposit(FEconomyState&, FName Inventory, FName Item, double Quantity, double UnitTonnes);
    static bool Withdraw(FEconomyState&, FName Owner, FName Inventory, FName Item, double Quantity);
    static FGuid Reserve(FEconomyState&, FName Owner, FName Inventory, FName Item, double Quantity);
    static bool Release(FEconomyState&, FName Owner, FGuid Reservation);
    static bool Transfer(FEconomyState&, FName Owner, FName Source, FName Destination, FName Item, double Quantity, FGuid Reservation = FGuid());
    static bool Pay(FEconomyState&, FName From, FName To, int64 Credits, FGuid Transaction, FName Reason);
    static bool Trade(FEconomyState&, FName Buyer, FName Seller, FName Source, FName Destination, FName Item, double Quantity, int64 TotalPrice, FGuid Transaction);
    static FGuid QueueJob(FEconomyState&, FName Owner, FName Facility, FName Input, FName Output, FName Recipe, const TArray<FCommodityAmount>& Materials, const TArray<FCommodityAmount>& Products, double Duration);
    static FGuid GrantBlueprint(FEconomyState&, FName Owner, FName Recipe, bool bOriginal, int32 Runs, double MaterialEfficiency = 0, double TimeEfficiency = 0);
    static FGuid QueueBlueprintJob(FEconomyState&, FName Owner, FName Facility, FName Input, FName Output,
        FGuid Blueprint, int32 Runs, const TArray<FCommodityAmount>& Materials, const TArray<FCommodityAmount>& Products, double Duration);
    // Shared read-only preflight for installation, job UI and authoritative processing.
    static bool CanStartJob(const FEconomyState&, const FIndustryJob&, bool bOperational, FText& Reason, bool bCheckFacilitySlot = true);
    static bool CancelJob(FEconomyState&, FName Owner, FGuid Id);
    static int32 AdvanceJobs(FEconomyState&, double Seconds, TFunctionRef<bool(FName)> Operational);
    static double SystemQuantity(const FEconomyState&, FName System, FName Item);
private:
    static FGuid QueueAuthorizedJob(FEconomyState&, FName Owner, FName Facility, FName Input, FName Output, FName Recipe,
        const TArray<FCommodityAmount>& Materials, const TArray<FCommodityAmount>& Products, double Duration);
};
