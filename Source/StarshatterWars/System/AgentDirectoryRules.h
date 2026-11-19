#pragma once
#include "CoreMinimal.h"

enum class EAgentDirectoryScope : uint8 { SameSystem, OneJump, EmpireWide };

namespace AgentDirectoryRules
{
inline bool IncludesSystem(FName Origin, FName Candidate, const TSet<FName>& Neighbors, EAgentDirectoryScope Scope)
{
    if (Candidate.IsNone()) return false;
    if (Scope == EAgentDirectoryScope::EmpireWide) return true;
    if (Origin.IsNone()) return false;
    return Candidate == Origin || (Scope == EAgentDirectoryScope::OneJump && Neighbors.Contains(Candidate));
}
}
