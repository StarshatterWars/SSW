#pragma once
#include "CoreMinimal.h"
struct FEconomyServiceEndpoint
{
    FString Url=TEXT("http://127.0.0.1:17881");
    bool bAutoStartLocal=true;
    bool bLoopback=true;
    bool bValid=true;
    int64 Revision=0;
    FString Error;
    static FString SettingsPath();
    static FEconomyServiceEndpoint Load(const FString& Path=FString());
    static bool Normalize(const FString&,FString& Url,bool& Loopback,FString& Error);
};
