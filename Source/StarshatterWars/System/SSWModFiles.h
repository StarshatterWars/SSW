#pragma once

#include "CoreMinimal.h"

namespace SSWModFiles
{
    bool Read(const FString& Path, TArray<uint8>& Bytes);
    // Read JSON from the highest-priority enabled pack; DEF requires explicit migration.
    bool ReadMission(const FString& Path, TArray<uint8>& Bytes, bool& bJson);
}
