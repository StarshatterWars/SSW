#pragma once

#include "CoreMinimal.h"
#include "FreightManifestTypes.generated.h"

// Append new entries to preserve DataTable values.
UENUM(BlueprintType)
enum class ESSWFinishedItem : uint8
{
    None = 0,
    Avionics,
    FinishedElectronics,
    MedicalSupplies,
    MachineParts,
    SealedDocuments,
    MilitaryRations,
    MilitarySpares,
    FieldMedicalSupplies
};

USTRUCT(BlueprintType)
struct FSSWCargoCapacity
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
    double Tonnes = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
    int32 PassengerBerths = 0;
};

USTRUCT(BlueprintType)
struct FSSWCargoOption
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FString Item;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ESSWFinishedItem FinishedItem = ESSWFinishedItem::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bPassengers = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FString Unit;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Minimum = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Maximum = 0;
};
