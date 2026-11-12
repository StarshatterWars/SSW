#pragma once

#include "CoreMinimal.h"
#include "AsteroidResource.generated.h"

// Append new resources to preserve existing serialized IDs.
UENUM(BlueprintType)
enum class ESSWMineral : uint8
{
    None = 0,
    Olivine, Pyroxene, Plagioclase, Magnetite, Troilite, Kamacite, Taenite,
    Chromite, Ilmenite, Spinel, Apatite, Merrillite, Graphite, Diamond,
    Serpentine, Saponite, Calcite, Dolomite, Pyrite, Pentlandite, Schreibersite,
    Hematite, Quartz, Rutile, Cassiterite, Chalcopyrite, Sphalerite, Galena,
    Cobaltite, Scheelite, Wolframite, Monazite, Zircon, Uraninite, WaterIce, Dilithium, Quantium40
};

UENUM(BlueprintType)
enum class ESSWAsteroidType : uint8
{
    Carbonaceous,
    Stony,
    Metallic
};

UENUM(BlueprintType)
enum class ESSWAsteroidRarity : uint8
{
    Common,
    Uncommon,
    Rare,
    Exotic
};

USTRUCT(BlueprintType)
struct FSSWAsteroidResource
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resources", meta=(ClampMin="0"))
    double AmountTonnes = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resources")
    ESSWMineral Mineral = ESSWMineral::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resources")
    ESSWAsteroidType Type = ESSWAsteroidType::Carbonaceous;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resources")
    ESSWAsteroidRarity Rarity = ESSWAsteroidRarity::Common;

    FText GetMineralName() const;
    FText GetRarityName() const;
    int32 GetValuePerTonne() const;
    FText GetTypeName() const;
};
