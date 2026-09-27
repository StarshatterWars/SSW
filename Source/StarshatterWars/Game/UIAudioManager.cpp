// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "UIAudioManager.h"

#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"


// Static definitions
USoundBase* UUIAudioManager::ClickSound = nullptr;
USoundBase* UUIAudioManager::HoverSound = nullptr;
float UUIAudioManager::UISoundVolume = 1.0f;
TArray<UAudioComponent*> UUIAudioManager::SoundPool;
TQueue<USoundBase*> UUIAudioManager::SoundQueue;
bool UUIAudioManager::bIsPlaying = false;
FTimerHandle UUIAudioManager::QueueTimerHandle;

void UUIAudioManager::PlayClickSound(UObject* WorldContext)
{
    if (ClickSound)
        SoundQueue.Enqueue(ClickSound);

    ProcessSoundQueue(WorldContext);
}

void UUIAudioManager::PlayHoverSound(UObject* WorldContext)
{
    if (HoverSound)
        SoundQueue.Enqueue(HoverSound);

    ProcessSoundQueue(WorldContext);
}

void UUIAudioManager::ProcessSoundQueue(UObject* WorldContext)
{
    if (bIsPlaying || SoundQueue.IsEmpty())
        return;

    USoundBase* NextSound;
    if (SoundQueue.Dequeue(NextSound))
    {
        bIsPlaying = true;
        PlaySoundInternal(WorldContext, NextSound);
    }
}

void UUIAudioManager::PlaySoundInternal(UObject* WorldContext, USoundBase* Sound)
{
    if (!Sound || !WorldContext) return;

    UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContext);
    UAudioComponent* AudioComp = nullptr;

    // Reuse or create a new AudioComponent
    for (UAudioComponent* Comp : SoundPool)
    {
        if (!Comp->IsPlaying())
        {
            AudioComp = Comp;
            break;
        }
    }

    if (!AudioComp)
    {
        AudioComp = NewObject<UAudioComponent>(World);
        AudioComp->RegisterComponent();
        SoundPool.Add(AudioComp);
    }

    AudioComp->SetSound(Sound);
    AudioComp->SetVolumeMultiplier(UISoundVolume);
    AudioComp->Play();

    // Set a delegate or timer to trigger the next sound in queue
    float Duration = Sound->GetDuration();
    if (Duration <= 0.f) Duration = 0.1f;

    FTimerDelegate TimerDel;
    TimerDel.BindLambda([WorldContext]()
        {
            bIsPlaying = false;
            ProcessSoundQueue(WorldContext);
        });

    World->GetTimerManager().SetTimer(QueueTimerHandle, TimerDel, Duration, false);
}

void UUIAudioManager::StopCurrentSound(float FadeOutDuration)
{
    for (UAudioComponent* Comp : SoundPool)
    {
        if (Comp && Comp->IsPlaying())
        {
            Comp->FadeOut(FadeOutDuration, 0.0f);
        }
    }

    SoundQueue.Empty();
    bIsPlaying = false;
}

void UUIAudioManager::StopSoundEffects()
{

}



