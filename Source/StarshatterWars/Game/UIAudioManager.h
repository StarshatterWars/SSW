// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "UIAudioManager.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class STARSHATTERWARS_API UUIAudioManager : public UObject
{
	GENERATED_BODY()
	
	public:
    /** Play UI Sounds */
    UFUNCTION(BlueprintCallable, Category = "UI Audio")
    static void PlayClickSound(UObject* WorldContext);

    UFUNCTION(BlueprintCallable, Category = "UI Audio")
    static void PlayHoverSound(UObject* WorldContext);

    /** Stop with fade */
    UFUNCTION(BlueprintCallable, Category = "UI Audio")
    static void StopCurrentSound(float FadeOutDuration = 0.2f);

    UFUNCTION(BlueprintCallable, Category = "UI Audio")
    static void StopSoundEffects();

    /** Sound setup */
    static USoundBase* ClickSound;
    static USoundBase* HoverSound;
    static float UISoundVolume;

private:
    static TArray<UAudioComponent*> SoundPool;
    static TQueue<USoundBase*> SoundQueue;
    static bool bIsPlaying;
    static FTimerHandle QueueTimerHandle;

    static void PlaySoundInternal(UObject* WorldContext, USoundBase* Sound);
    static void OnSoundFinished(UAudioComponent* AudioComp);
    static void ProcessSoundQueue(UObject* WorldContext);
	
};
