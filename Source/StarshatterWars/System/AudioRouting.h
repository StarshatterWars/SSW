#pragma once
#include "CoreMinimal.h"
class UAudioComponent;
class USoundBase;
enum class EStarshatterAudioChannel : uint8 { Effects, Music, Voice };
namespace SSWAudioRouting
{
    EStarshatterAudioChannel InferChannel(const FString& AssetPath);
    void Route(UAudioComponent* Component,EStarshatterAudioChannel Channel);
    UAudioComponent* Play2D(const UObject* Context,USoundBase* Sound,EStarshatterAudioChannel Channel,float Volume=1.f,bool AutoDestroy=true);
}
