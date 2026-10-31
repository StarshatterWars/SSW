#pragma once

#include "CoreMinimal.h"
#include "GameStructs.h"

class UInputMappingContext;
struct FEnhancedActionKeyMapping;
class KeyMap;
class APlayerController;

// Changes only runtime copies. Authored mapping assets and their triggers/modifiers stay intact.
namespace StarshatterInputBindings
{
    STARSHATTERWARS_API UInputMappingContext* SupplementalContext(UObject* Owner);
    STARSHATTERWARS_API bool IsModifierDown(APlayerController* Controller,const FString& Modifier);
    STARSHATTERWARS_API FString Id(const FEnhancedActionKeyMapping& Mapping);
    STARSHATTERWARS_API UInputMappingContext* RuntimeContext(UInputMappingContext* Source, UObject* Owner);
    STARSHATTERWARS_API void RegisterContext(UInputMappingContext* Context);
    STARSHATTERWARS_API void Refresh();
    STARSHATTERWARS_API FString Category(const FString& ActionName);
    STARSHATTERWARS_API void ApplyLegacy(KeyMap& Mapping, const FStarshatterKeyboardConfig& Config, UObject* Owner=nullptr);
}
