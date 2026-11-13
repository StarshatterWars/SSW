#pragma once
#include "CoreMinimal.h"

/** DEF filename is the stable asset key; legacy names/models are lookup aliases. */
namespace ShipVisualCatalog
{
    const TCHAR* DedicatedStationIcon(const FString& DesignName);
struct FEntry { const TCHAR* Definition; const TCHAR* Design; const TCHAR* Model; const TCHAR* NativeClass; const TCHAR* Kind; };
STARSHATTERWARS_API TConstArrayView<FEntry> Entries();
STARSHATTERWARS_API FString DefinitionName(const FString& DesignName, const FString& ModelName = FString());
STARSHATTERWARS_API FString CanonicalClassPath(const FString& DesignName, const FString& ModelName = FString());
STARSHATTERWARS_API FString IconFolder(const FString& DesignName);
STARSHATTERWARS_API UClass* Resolve(const FString& DesignName, const FString& ModelName = FString());
}
