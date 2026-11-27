#pragma once
#include "CoreMinimal.h"
// Process-wide writer fence. Tests/commandlets use isolated value-state workers.
namespace SSWEconomyAuthority
{
    bool RequiresService();
    bool MayAdvanceLocally();
    bool BeginHandoff(FString& Reason);
    FString MarkerPath();
}
