#pragma once

#include "CoreMinimal.h"

namespace SSWModFiles
{
    bool Read(const FString& Path, TArray<uint8>& Bytes);
    // Choose within the highest-priority pack before choosing JSON over DEF.
    bool ReadMission(const FString& Path, TArray<uint8>& Bytes, bool& bJson);
}
