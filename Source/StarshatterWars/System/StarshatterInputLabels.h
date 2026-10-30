#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InputActionValue.h"
#include "EnhancedActionKeyMapping.h"
#include "GameStructs.h"
#include "StarshatterInputLabels.generated.h"
class UInputAction;

/** Shared display labels only. Never changes identifiers, mappings or input behavior. */
UCLASS()
class STARSHATTERWARS_API UStarshatterInputLabels : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="Starshatter|Input")
    static FText GetActionLabel(const UInputAction* Action);
    UFUNCTION(BlueprintPure, Category="Starshatter|Input")
    static FText GetActionTypeLabel(const UInputAction* Action);
    UFUNCTION(BlueprintPure, Category="Starshatter|Input")
    static FText GetBindingKeyLabel(const FEnhancedActionKeyMapping& Mapping);
    UFUNCTION(BlueprintPure, Category="Starshatter|Input")
    static FText GetValueTypeLabel(EInputActionValueType ValueType);
    UFUNCTION(BlueprintPure, Category="Starshatter|Input")
    static FText GetDefaultActionLabel(FName ActionName);
    UFUNCTION(BlueprintPure, Category="Starshatter|Input")
    static FText GetLegacyActionLabel(EStarshatterInputAction Action);
};
