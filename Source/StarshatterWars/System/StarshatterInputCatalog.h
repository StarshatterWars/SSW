#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameStructs.h" // EStarshatterInputAction
#include "StarshatterInputCatalog.generated.h"

class UInputAction;
class UInputMappingContext;

UCLASS()
class STARSHATTERWARS_API UStarshatterInputCatalog : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Category = "Starshatter|EnhancedInput")
    TObjectPtr<UInputMappingContext> ContextGameplay = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Starshatter|EnhancedInput")
    TObjectPtr<UInputMappingContext> ContextUI = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Starshatter|EnhancedInput")
    TObjectPtr<UInputMappingContext> ContextMenu = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Starshatter|EnhancedInput")
    TMap<EStarshatterInputAction, TObjectPtr<UInputAction>> Actions;
};
