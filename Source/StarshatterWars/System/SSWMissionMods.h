#pragma once

#include "CoreMinimal.h"

namespace SSWMissionMods
{
    struct FPack
    {
        FString Name;
        TArray<FString> Missions;
        bool bEnabled = false;
    };

    FString Directory();
    TArray<FPack> Discover();
    void SetEnabled(const FString& Name, bool bEnabled);
}
