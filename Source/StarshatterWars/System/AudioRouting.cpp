#include "AudioRouting.h"
#include "StarshatterAudioSubsystem.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
EStarshatterAudioChannel SSWAudioRouting::InferChannel(const FString& Path)
{
    if(Path.Contains(TEXT("/Music/")))return EStarshatterAudioChannel::Music;
    if(Path.Contains(TEXT("/Vox/")) || Path.Contains(TEXT("/Ranks/")) || Path.Contains(TEXT("/Awards/Rank")))return EStarshatterAudioChannel::Voice;
    return EStarshatterAudioChannel::Effects;
}
void SSWAudioRouting::Route(UAudioComponent* Component,EStarshatterAudioChannel Channel)
{
    if(Component)if(auto* Audio=UStarshatterAudioSubsystem::Get(Component))Audio->RouteAudioComponent(Component,Channel);
}
UAudioComponent* SSWAudioRouting::Play2D(const UObject* Context,USoundBase* Sound,EStarshatterAudioChannel Channel,float Volume,bool AutoDestroy)
{
    if(!Context || !Sound)return nullptr;
    UAudioComponent* Component=UGameplayStatics::CreateSound2D(Context,Sound,Volume,1.f,0.f,nullptr,false,AutoDestroy);
    if(Component){Route(Component,Channel);Component->Play();}
    return Component;
}
