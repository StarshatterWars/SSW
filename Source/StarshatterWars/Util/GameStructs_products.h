#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "AsteroidResource.h"
#include "FreightManifestTypes.h"
#include "GameStructs_products.generated.h"

USTRUCT(BlueprintType)
struct FSSWProductionRecipe : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<ESSWMineral> Minerals;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ESSWFinishedItem Product = ESSWFinishedItem::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    double InputTonnes = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    double OutputTonnes = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    double SecondsPerBatch = 0;

    bool CanStart(double Available, double FreeSpace) const
    {
        return Product != ESSWFinishedItem::None && InputTonnes > 0 &&
            OutputTonnes > 0 && SecondsPerBatch > 0 &&
            Available >= InputTonnes && FreeSpace >= OutputTonnes;
    }
};

struct FSSWProductionStock
{
    double Ore = 0;
    double Product = 0;
    double RemainingSeconds = 0;

    double FreeInput(const FSSWProductionRecipe& Recipe, double Capacity) const
    {
        const double ReservedOutput = RemainingSeconds > 0 ? Recipe.OutputTonnes : 0;
        return FMath::Max(0.0, Capacity - Ore - Product - ReservedOutput);
    }

    bool Advance(const FSSWProductionRecipe& Recipe, double Capacity, double Seconds)
    {
        if (RemainingSeconds <= 0 && Recipe.CanStart(Ore, Capacity - Product - Ore + Recipe.InputTonnes))
        {
            Ore -= Recipe.InputTonnes;
            RemainingSeconds = Recipe.SecondsPerBatch;
        }
        if (RemainingSeconds <= 0) return false;

        RemainingSeconds = FMath::Max(0.0, RemainingSeconds - Seconds);
        if (RemainingSeconds > 0) return false;
        Product += Recipe.OutputTonnes;
        return true;
    }
};
