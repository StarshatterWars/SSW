#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/SoftObjectPtr.h"
#include "GameStructs_Assets.generated.h"

UENUM(BlueprintType)
enum class EStarshatterAssetDomain : uint8
{
    UI     UMETA(DisplayName = "UI"),
    Data   UMETA(DisplayName = "Data"),
    Audio  UMETA(DisplayName = "Audio"),
    Visual UMETA(DisplayName = "Visual"),
    Other  UMETA(DisplayName = "Other"),
};

USTRUCT(BlueprintType)
struct FS_MasterAssetRow : public FTableRowBase
{
    GENERATED_BODY()

    // Optional: domain for human organization / validation
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EStarshatterAssetDomain Domain = EStarshatterAssetDomain::Other;

    // The asset reference (Soft) - can be DataTable, Widget BP class, Sound, etc.
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UObject> Asset;

    // Optional notes for you in editor
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FString Notes;
};
