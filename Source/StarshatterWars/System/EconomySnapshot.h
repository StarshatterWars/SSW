#pragma once
#include "CoreMinimal.h"
struct FEconomyState;struct FGalaxyTrafficState;class FJsonObject;
namespace SSWEconomySnapshot
{
    TSharedPtr<FJsonObject> Sites(const FEconomyState&,FString&);
    FString SitesSignature(const FEconomyState&);
    TSharedPtr<FJsonObject> Capture(const FEconomyState&,const FGalaxyTrafficState&,FString&);
    bool Decode(const TSharedPtr<FJsonObject>&,FEconomyState&,FGalaxyTrafficState&,TMap<FGuid,FString>&,FString&);
}
