#pragma once

#include "CoreMinimal.h"

namespace SSWModFiles
{
    bool Read(const FString& Path, TArray<uint8>& Bytes);
}
