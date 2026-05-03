/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright(c) 2025 - 2026.

    SUBSYSTEM:    Stars.exe(Unreal Port)
    FILE : StarshatterAudioSubsystem.cpp
    AUTHOR : Carlos Bott
*/

#include "StarshatterAudioSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

#include "MusicController.h"
#include "StarshatterAudioSettings.h"
#include "StarshatterSettingsSaveGame.h"
#include "GameStructs.h"

void UStarshatterAudioSubsystem::Initialize(FSubsystemCollectionBase & Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogTemp, Log, TEXT("[AudioSS] Initialize"));
}

void UStarshatterAudioSubsystem::Deinitialize()
{
    MusicController = nullptr;
    MenuMusic = nullptr;
    HoverSound = nullptr;
    AcceptSound = nullptr;

    UE_LOG(LogTemp, Log, TEXT("[AudioSS] Deinitialize"));

    Super::Deinitialize();
}

void UStarshatterAudioSubsystem::Boot()
{
    LoadAudioConfig();
    ApplySettingsToRuntime();
    SetupMusicController();

    UE_LOG(LogTemp, Log, TEXT("[AudioSS] Boot complete"));
}

UStarshatterAudioSubsystem* UStarshatterAudioSubsystem::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
        return nullptr;

    const UWorld* World = WorldContextObject->GetWorld();
    if (!World)
        return nullptr;

    UGameInstance* GI = World->GetGameInstance();
    return Get(GI);
}

UStarshatterAudioSubsystem* UStarshatterAudioSubsystem::Get(UGameInstance* GameInstance)
{
    return GameInstance
        ? GameInstance->GetSubsystem<UStarshatterAudioSubsystem>()
        : nullptr;
}

UStarshatterAudioSettings* UStarshatterAudioSubsystem::GetSettings() const
{
    return UStarshatterAudioSettings::Get();
}

void UStarshatterAudioSubsystem::LoadAudioConfig()
{
    if (UStarshatterAudioSettings* Settings = GetSettings())
    {
        Settings->Load();

        UE_LOG(LogTemp, Log, TEXT("[AudioSS] Audio config loaded"));
    }
}

void UStarshatterAudioSubsystem::SaveAudioConfig()
{
    if (UStarshatterAudioSettings* Settings = GetSettings())
    {
        Settings->Sanitize();
        Settings->Save();

        UE_LOG(LogTemp, Log, TEXT("[AudioSS] Audio config saved"));
    }
}

void UStarshatterAudioSubsystem::ApplySettingsToRuntime()
{
    if (UStarshatterAudioSettings* Settings = GetSettings())
    {
        UObject* WorldContext = GetGameInstance();
        if (!WorldContext)
        {
            WorldContext = this;
        }

        Settings->ApplyToRuntimeAudio(WorldContext);

        UE_LOG(LogTemp, Log, TEXT("[AudioSS] Audio settings applied to runtime"));
    }
}

void UStarshatterAudioSubsystem::LoadFromSaveGame(const UStarshatterSettingsSaveGame* SaveGame)
{
    if (!SaveGame)
        return;

    UStarshatterAudioSettings* Settings = GetSettings();
    if (!Settings)
        return;

    const FStarshatterAudioConfig& A = SaveGame->Audio;

    Settings->SetMasterVolume(A.MasterVolume);
    Settings->SetMusicVolume(A.MusicVolume);
    Settings->SetEffectsVolume(A.EffectsVolume);
    Settings->SetVoiceVolume(A.VoiceVolume);
    Settings->SetSoundQuality(A.SoundQuality);

    Settings->Sanitize();

    UE_LOG(LogTemp, Log, TEXT("[AudioSS] Loaded audio settings from SaveGame"));
}

void UStarshatterAudioSubsystem::WriteToSaveGame(UStarshatterSettingsSaveGame* SaveGame) const
{
    if (!SaveGame)
        return;

    const UStarshatterAudioSettings* Settings = GetSettings();
    if (!Settings)
        return;

    FStarshatterAudioConfig& A = SaveGame->Audio;

    A.MasterVolume = Settings->GetMasterVolume();
    A.MusicVolume = Settings->GetMusicVolume();
    A.EffectsVolume = Settings->GetEffectsVolume();
    A.VoiceVolume = Settings->GetVoiceVolume();
    A.SoundQuality = Settings->GetSoundQuality();

    UE_LOG(LogTemp, Log, TEXT("[AudioSS] Wrote audio settings to SaveGame"));
}

void UStarshatterAudioSubsystem::SetupMusicController()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AudioSS] SetupMusicController failed: World is null"));
        return;
    }

    for (TActorIterator<AMusicController> It(World); It; ++It)
    {
        MusicController = *It;

        UE_LOG(LogTemp, Log,
            TEXT("[AudioSS] Found existing MusicController: %s"),
            *GetNameSafe(MusicController.Get()));

        return;
    }

    const FVector Location(-1000.0f, 0.0f, 0.0f);

    MusicController = World->SpawnActor<AMusicController>(
        AMusicController::StaticClass(),
        Location,
        FRotator::ZeroRotator
    );

    if (MusicController)
    {
        UE_LOG(LogTemp, Log,
            TEXT("[AudioSS] Spawned MusicController: %s"),
            *GetNameSafe(MusicController.Get()));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[AudioSS] Failed to spawn MusicController"));
    }
}

AMusicController* UStarshatterAudioSubsystem::GetMusicController()
{
    if (MusicController && IsValid(MusicController))
    {
        return MusicController;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<AMusicController> It(World); It; ++It)
    {
        MusicController = *It;
        return MusicController;
    }

    return nullptr;
}

void UStarshatterAudioSubsystem::PlayMusic(USoundBase* Music)
{
    if (!Music)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AudioSS] PlayMusic failed: Music is null"));
        return;
    }

    if (AMusicController* MC = GetMusicController())
    {
        MC->PlayMusic(Music);
    }
}

void UStarshatterAudioSubsystem::StopMusic()
{
    if (AMusicController* MC = GetMusicController())
    {
        MC->StopMusic();
    }
}

void UStarshatterAudioSubsystem::PlayMenuMusic()
{
    PlayMusic(MenuMusic);
}

void UStarshatterAudioSubsystem::PlayUISound(UObject* Context, USoundBase* UISound)
{
    if (!Context || !UISound)
    {
        return;
    }

    UGameplayStatics::PlaySound2D(Context, UISound);
}

void UStarshatterAudioSubsystem::PlayHoverSound(UObject* Context)
{
    PlayUISound(Context, HoverSound);
}

void UStarshatterAudioSubsystem::PlayAcceptSound(UObject* Context)
{
    PlayUISound(Context, AcceptSound);
}

void UStarshatterAudioSubsystem::PlaySoundFromFile(const FString& AudioPath)
{
    USoundBase* Sound = Cast<USoundBase>(
        StaticLoadObject(USoundBase::StaticClass(), nullptr, *AudioPath)
    );

    if (!Sound)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[AudioSS] PlaySoundFromFile failed. Invalid sound path: %s"),
            *AudioPath);

        return;
    }

    if (AMusicController* MC = GetMusicController())
    {
        MC->PlayUISound(Sound);
    }
}

bool UStarshatterAudioSubsystem::IsSoundPlaying()
{
    if (AMusicController* MC = GetMusicController())
    {
        return MC->IsSoundPlaying();
    }

    return false;
}