#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "AsteroidResource.h"
#include "FreightManifestTypes.h"
#include "GameStructs_economy.h"
#include "GameStructs_products.generated.h"

UENUM(BlueprintType)
enum class ESSWProductionFacilityType : uint8 { Refinery, Factory, Shipyard };

USTRUCT(BlueprintType)
struct FSSWProductionFacility : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FString Facility;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) ESSWProductionFacilityType FacilityType=ESSWProductionFacilityType::Factory;
    // Player service terms; NPC background batches retain their authored recipe.
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ServiceProfile = TEXT("Standard");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bPlayerManufacturing = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) double PlayerMaterialMultiplier = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) double PlayerTimeMultiplier = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 InstallationFeeCredits = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 PerRunFeeCredits = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MaximumPlayerRuns = 1000000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Image = TEXT("Production");
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FString System;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FString Planet;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FString Region;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FString Empire;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bSurface = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName Recipe;
    // Player jobs may use these recipes without replacing automated primary production.
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> AdditionalRecipes;
    bool SupportsRecipe(FName Id) const { return !Id.IsNone() && (Recipe == Id || AdditionalRecipes.Contains(Id)); }
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
    double CapacityTonnes = 0;
};

USTRUCT(BlueprintType)
struct FSSWProductionRecipe : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<ESSWMineral> Minerals;
    // An assembled hull is stock, not a spawned combat ship or a free player grant.
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ShipDesign;

    // Player installation gates use native SSW skill keys. Empty keeps legacy recipes unrestricted.
    // NPC background production does not depend on the player's qualifications.
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName, int32> RequiredSkillLevels;

    bool HasValidSkillRequirements() const
    {
        for (const auto& Requirement : RequiredSkillLevels)
            if (Requirement.Key.IsNone() || Requirement.Value < 1 || Requirement.Value > 5) return false;
        return true;
    }

    // Exact bill of materials. Empty only for ore refining, where Minerals are alternatives.
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FCommodityAmount> Inputs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Image = TEXT("Production");

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ESSWFinishedItem Product = ESSWFinishedItem::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FString CargoItem;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    double OutputQuantity = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FString OutputUnit;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FString OutputDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    double InputTonnes = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    double OutputTonnes = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    double SecondsPerBatch = 0;

    bool CanStart(double Available, double FreeSpace) const
    {
        return (Product != ESSWFinishedItem::None || !CargoItem.IsEmpty()) && InputTonnes > 0 &&
            OutputTonnes > 0 && SecondsPerBatch > 0 &&
            Available >= InputTonnes && FreeSpace >= OutputTonnes;
    }
};

struct FSSWProductionStock
{
    TMap<FName, double> Inventories;
    double RemainingSeconds = 0;
    double ReservedOutput = 0;
    FName PendingOutput;

    double GetTonnes(FName Location) const { return Inventories.FindRef(Location); }
    double& GetInventory(FName Location) { return Inventories.FindOrAdd(Location); }

    double UsedSpace() const
    {
        double Used = ReservedOutput;
        for (const auto& Inventory : Inventories) Used += Inventory.Value;
        return Used;
    }

    double FreeSpace(double Capacity) const { return FMath::Max(0.0, Capacity - UsedSpace()); }
};
