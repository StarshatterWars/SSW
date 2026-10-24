/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe (Unreal Port)
    FILE:         StarshatterAudioSubsystem.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UStarshatterAudioSubsystem
    - GameInstanceSubsystem wrapper around UStarshatterAudioSettings (config-backed CDO).
    - Stable entrypoints used by UI dialogs and boot sequencing:
        * Get(...)
        * Boot()
        * LoadAudioConfig()
        * SaveAudioConfig()
        * ApplySettingsToRuntime()
        * LoadFromSaveGame() / WriteToSaveGame()
*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AudioRouting.h"
#include "StarshatterAudioSubsystem.generated.h"

class AMusicController;
class USoundBase;
class USoundClass;
class USoundMix;
class UAudioComponent;
class UStarshatterAudioSettings;
class UStarshatterSettingsSaveGame;

UCLASS()
class STARSHATTERWARS_API UStarshatterAudioSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

public:
    void Boot();

    static UStarshatterAudioSubsystem* Get(const UObject* WorldContextObject);
    static UStarshatterAudioSubsystem* Get(UGameInstance* GameInstance);

public:
    UStarshatterAudioSettings* GetSettings() const;

    void LoadAudioConfig();
    void SaveAudioConfig();
    void ApplySettingsToRuntime();
    void ApplyRuntimeSettings(const UStarshatterAudioSettings& Settings);
    void RouteAudioComponent(UAudioComponent* Component,EStarshatterAudioChannel Channel);
    float GetChannelGain(EStarshatterAudioChannel Channel) const;
    float GetMasterGain() const { return RuntimeMaster; }

public:
    void LoadFromSaveGame(const UStarshatterSettingsSaveGame* SaveGame);
    void WriteToSaveGame(UStarshatterSettingsSaveGame* SaveGame) const;

public:
    void SetupMusicController();
    AMusicController* GetMusicController();

    void PlayMusic(USoundBase* Music);
    void StopMusic();
    void PlayMenuMusic();

    void PlayUISound(UObject* Context, USoundBase* UISound);
    void PlayHoverSound(UObject* Context);
    void PlayAcceptSound(UObject* Context);
    void PlaySoundFromFile(const FString& AudioPath);

    bool IsSoundPlaying();

public:
    void SetMenuMusic(USoundBase* InMusic) { MenuMusic = InMusic; }
    void SetHoverSound(USoundBase* InSound) { HoverSound = InSound; }
    void SetAcceptSound(USoundBase* InSound) { AcceptSound = InSound; }

private:
    void EnsureRouting();
    void UpdateRoutingMix();
    UPROPERTY(Transient) TObjectPtr<USoundMix> RuntimeMix;
    UPROPERTY(Transient) TMap<FString,TObjectPtr<USoundClass>> RuntimeClasses;
    TMap<USoundClass*,EStarshatterAudioChannel> RuntimeChannels;
    TWeakObjectPtr<UWorld> MixWorld;
    float RuntimeMaster=1.f, RuntimeMusic=1.f, RuntimeEffects=1.f, RuntimeVoice=1.f;
    int32 RuntimeQuality=0;

private:
    UPROPERTY()
    TObjectPtr<AMusicController> MusicController = nullptr;

    UPROPERTY()
    TObjectPtr<USoundBase> MenuMusic = nullptr;

    UPROPERTY()
    TObjectPtr<USoundBase> HoverSound = nullptr;

    UPROPERTY()
    TObjectPtr<USoundBase> AcceptSound = nullptr;
};