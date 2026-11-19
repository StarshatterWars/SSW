#pragma once
#include "CoreMinimal.h"

class UDataTable;
namespace AwardsJsonCatalog
{
    // Full native row serialization retains authored IDs, eligibility and media.
    // Both tables validate before either table is changed.
    bool Import(const FString& Path, UDataTable& Ranks, UDataTable& Medals, FString& Error);
    bool Export(const FString& Path, const UDataTable& Ranks, const UDataTable& Medals, FString& Error);
}
