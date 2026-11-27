#pragma once
#include "CoreMinimal.h"
struct FEconomyState;
namespace SSWEconomyOwnership
{
    // Deliberate allowlist. Unrecognized owners stay in their existing domain.
    bool IsNpcOwner(const FEconomyState&,FName Owner);
    bool MatchesNpc(const FEconomyState&,const FEconomyState&);
    FEconomyState ExtractNpc(const FEconomyState&);
    // Replaces only the explicitly NPC-owned projection. Player assets/receipts are retained.
    void ApplyNpcProjection(FEconomyState& PlayerView,const FEconomyState& NpcView);
}
