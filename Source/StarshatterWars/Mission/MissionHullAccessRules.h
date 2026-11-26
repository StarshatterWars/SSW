#pragma once
#include "GameStructs.h"

namespace MissionHullAccessRules
{
    inline const FPlayerHangarShip* FindAvailable(const FS_PlayerGameInfo& PilotState, FName Required)
    {
        if (Required.IsNone()) return nullptr;
        if (const auto* Owned = PilotState.HangarShips.FindByPredicate([Required](const auto& Hull)
            { return Hull.HullId.IsValid() && Hull.Design == Required && !Hull.bAssigned && !Hull.bLost && !Hull.bSold && !Hull.RentalId.IsValid(); })) return Owned;
        // Preserve the issued basic trainer; advanced aircraft still require an owned hull.
        if (Required == FName(TEXT("Falcon")))
            return PilotState.HangarShips.FindByPredicate([&](const auto& Hull)
                { return Hull.HullId.IsValid() && Hull.HullId == PilotState.AssignedHangarHull && Hull.Design == Required && !Hull.bLost && !Hull.bSold && !Hull.RentalId.IsValid(); });
        return nullptr;
    }
}
