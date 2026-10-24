#pragma once
#include "CoreMinimal.h"
namespace LegacyBitmapLoader
{
    bool DecodePCX(TConstArrayView<uint8> Bytes,int32& Width,int32& Height,TArray<FColor>& Pixels);
    bool Load(const FString& Path,int32& Width,int32& Height,TArray<FColor>& Pixels);
}
