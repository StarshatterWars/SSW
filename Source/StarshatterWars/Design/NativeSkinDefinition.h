#pragma once
#include "GameStructs_System.h"
class TermStruct;
namespace NativeSkinDefinition
{
    FSkinMtlCell ParseMaterial(TermStruct* Definition,const char* SourceFile);
    bool LoadAuthoredSkins(const FString& SourceFile,TArray<FShipSkin>& Skins);
}
