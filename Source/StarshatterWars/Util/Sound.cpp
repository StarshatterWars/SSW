#include "Sound.h"

#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "UObject/SoftObjectPath.h"

#include "Logging/LogMacros.h"
DEFINE_LOG_CATEGORY_STATIC(LogStarshatterSound, Log, All);

TWeakObjectPtr<UObject> USound::WorldContext = nullptr;
FVector USound::ListenerVelocity = FVector::ZeroVector;

USound::USound()
{
    status = UNINITIALIZED;
    volume = 0;
    flags = 0;
    looped = 0;
    location = FVector::ZeroVector;
    velocity = FVector::ZeroVector;

    FCStringAnsi::Strcpy(filename_ansi, "Sound()");
}

void USound::BeginDestroy()
{
    Release();
    Super::BeginDestroy();
}

void USound::SetWorldContext(UObject* InWorldContext)
{
    WorldContext = InWorldContext;
}

UWorld* USound::GetWorldFromContext()
{
    UObject* Ctx = WorldContext.Get();
    if (!Ctx)
        return nullptr;

    // Works for UGameInstance, UWorld, Actor, Component, etc.
    return Ctx->GetWorld();
}

void USound::SetListener(const Camera& /*cam*/, const FVector& InVel)
{
    ListenerVelocity = InVel;
}

void USound::SetFilename(const char* s)
{
    if (!s) return;
    AssetReference = UTF8_TO_TCHAR(s);

    int n = (int)FCStringAnsi::Strlen(s);
    if (n >= 60)
    {
        FMemory::Memzero(filename_ansi, sizeof(filename_ansi));
        FCStringAnsi::Strcpy(filename_ansi, "...");
        FCStringAnsi::Strcat(filename_ansi, s + n - 59);
        filename_ansi[63] = 0;
    }
    else
    {
        FCStringAnsi::Strcpy(filename_ansi, s);
    }
}

float USound::CentibelsToVolumeMultiplier(long Centibels)
{
    // dB = centibels / 100. linear = 10^(dB/20)
    const float dB = (float)Centibels / 100.0f;
    const float linear = FMath::Pow(10.0f, dB / 20.0f);
    return FMath::Clamp(linear, 0.0f, 10.0f);
}

void USound::SetVolume(long v)
{
    volume = v;
    if (AudioComp)
        AudioComp->SetVolumeMultiplier(CentibelsToVolumeMultiplier(volume));
}

void USound::SetLocation(const FVector& l)
{
    location = l;
    if (AudioComp)
        AudioComp->SetWorldLocation(location);
}

USoundBase* USound::LoadSoundAssetFromString(const FString& InRef)
{
    if (InRef.IsEmpty())
        return nullptr;

    // Accept "/Game/Audio/UI_Click.UI_Click" etc.
    FSoftObjectPath SoftPath(InRef);
    if (SoftPath.IsValid())
    {
        UObject* Obj = SoftPath.TryLoad();
        return Cast<USoundBase>(Obj);
    }

    // Fallback
    return Cast<USoundBase>(StaticLoadObject(USoundBase::StaticClass(), nullptr, *InRef));
}

void USound::EnsureAssetLoaded()
{
    if (SoundAsset)
        return;

    const FString Ref = AssetReference;
    SoundAsset = LoadSoundAssetFromString(Ref);

    if (!SoundAsset)
    {
        UE_LOG(LogStarshatterSound, Warning,
            TEXT("USound: Failed to load sound asset from '%s'."), *Ref);
        status = DONE;
        return;
    }

    status = READY;
}

void USound::ApplyAttenuation()
{
    const bool bIs3D = (flags & LOCALIZED) || (flags & LOC_3D);
    if (!bIs3D)
        return;

    if (!AttenuationAsset)
        AttenuationAsset = NewObject<USoundAttenuation>(this);

    if (!AttenuationAsset)
        return;

    FSoundAttenuationSettings& S = AttenuationAsset->Attenuation;
    S.bAttenuate = true;
    S.bSpatialize = true;

    // Simple sphere attenuation:
    S.AttenuationShape = EAttenuationShape::Sphere;
    S.AttenuationShapeExtents = FVector(FMath::Max(0.0f, min_distance), 0, 0);

    S.FalloffDistance = FMath::Max(0.0f, max_distance - min_distance);
    S.dBAttenuationAtMax = -60.0f;

    if (AudioComp)
        AudioComp->AttenuationSettings = AttenuationAsset;
}

void USound::EnsureAudioComponent()
{
    if (AudioComp)
        return;

    UWorld* World = GetWorldFromContext();
    if (!World)
    {
        UE_LOG(LogStarshatterSound, Warning,
            TEXT("USound: No WorldContext set. Call USound::SetWorldContext(GameInstance) early."));
        status = DONE;
        return;
    }

    const bool bIs3D = (flags & LOCALIZED) || (flags & LOC_3D);
    const bool bIsUI = (flags & INTERFACE) != 0;

    AudioComp = UGameplayStatics::CreateSound2D(World, SoundAsset,
        CentibelsToVolumeMultiplier(volume), 1.f, 0.f, nullptr, false, false);

    if (!AudioComp)
    {
        UE_LOG(LogStarshatterSound, Warning, TEXT("USound: Failed to spawn AudioComponent."));
        status = DONE;
        return;
    }

    AudioComp->bIsUISound = bIsUI;

    AudioComp->bAutoDestroy = false;
    AudioComp->bAllowSpatialization = bIs3D;
    AudioComp->SetWorldLocation(location);
    AudioComp->OnAudioFinishedNative.AddWeakLambda(this, [this](UAudioComponent*) { HandleFinished(); });

    if (bIs3D)
        ApplyAttenuation();
}

USound* USound::CreateFromAsset(USoundBase* Asset, const char* Name, DWORD Flags)
{
    if (!Asset) return nullptr;
    USound* S=NewObject<USound>(GetTransientPackage());
    S->SetFilename(Name); S->SoundAsset=Asset; S->flags=Flags; S->status=READY;
    S->AddToRoot(); S->bOwnsLegacyRoot=true;
    return S;
}

USound* USound::CreateStream(const char* InFilename)
{
    if (!InFilename || !InFilename[0])
        return nullptr;

    USound* S = NewObject<USound>(GetTransientPackage());
    S->SetFilename(InFilename);

    // Keep legacy behavior: detect ".ogg" and route to CreateOggStream
    const int32 Len = FCStringAnsi::Strlen(InFilename);
    if (Len >= 4)
    {
        const char c1 = InFilename[Len - 3];
        const char c2 = InFilename[Len - 2];
        const char c3 = InFilename[Len - 1];

        const bool bIsOgg =
            (c1 == 'o' || c1 == 'O') &&
            (c2 == 'g' || c2 == 'G') &&
            (c3 == 'g' || c3 == 'G');

        if (bIsOgg)
            return CreateOggStream(InFilename);
    }

    S->status = INITIALIZING;
    S->EnsureAssetLoaded();
    if (!S->IsReady()) return nullptr;
    S->AddToRoot(); S->bOwnsLegacyRoot=true;
    return S;
}

USound* USound::CreateOggStream(const char* InFilename)
{
    // UE-native: treat Ogg the same way (as an imported asset reference)
    if (!InFilename || !InFilename[0])
        return nullptr;

    USound* S = NewObject<USound>(GetTransientPackage());
    S->SetFilename(InFilename);
    S->flags |= OGGVORBIS;

    S->status = INITIALIZING;
    S->EnsureAssetLoaded();
    if (!S->IsReady()) return nullptr;
    S->AddToRoot(); S->bOwnsLegacyRoot=true;
    return S;
}

void USound::Update()
{
    if (AudioComp)
    {
        if (AudioComp->IsPlaying())
            status = PLAYING;
        else if (status == PLAYING)
            status = DONE;

        if ((flags & LOCALIZED) || (flags & LOC_3D))
            AudioComp->SetWorldLocation(location);
    }
}

void USound::Release()
{
    flags &= ~LOCKED;
    bReleaseWhenFinished = false;
    if (IsValid(AudioComp)) AudioComp->OnAudioFinishedNative.RemoveAll(this);
    Stop();
    if (IsValid(AudioComp)) AudioComp->DestroyComponent();
    AudioComp = nullptr;
    AttenuationAsset = nullptr;
    SoundAsset = nullptr;
    if (bOwnsLegacyRoot) { bOwnsLegacyRoot=false; RemoveFromRoot(); }
    status = DONE;
    bIsPlaying = false;
}

HRESULT USound::Play()
{
    EnsureAssetLoaded();
    if (!SoundAsset) return E_FAIL;
    if (!IsValid(AudioComp)) AudioComp=nullptr;
    EnsureAudioComponent();
    if (!AudioComp || !SoundAsset)
        return E_FAIL;

    AudioComp->SetSound(SoundAsset);
    AudioComp->SetPaused(false);
    AudioComp->Play();

    StartTimeSeconds = FPlatformTime::Seconds();
    bIsPlaying = true;

    status = PLAYING;
    return S_OK;
}

HRESULT USound::PlayAndRelease()
{
    bReleaseWhenFinished=true;
    const HRESULT Result=Play();
    if (Result!=S_OK) Release();
    return Result;
}

void USound::HandleFinished()
{
    if ((flags & LOOP) && bIsPlaying && IsValid(AudioComp))
    {
        AudioComp->Play();
        return;
    }
    status=DONE; bIsPlaying=false;
    if (bReleaseWhenFinished) Release();
}

HRESULT USound::Stop()
{
    bIsPlaying = false;
    if (IsValid(AudioComp))
        AudioComp->Stop();
    status = DONE;
    return S_OK;
}

HRESULT USound::Pause()
{
    if (AudioComp)
        AudioComp->SetPaused(true);

    bIsPlaying = false;
    status = READY;
    return S_OK;
}

HRESULT USound::Rewind()
{
    if (!AudioComp)
        return E_NOINTERFACE;

    AudioComp->Stop();
    AudioComp->Play(0.0f);
    status = READY;
    return S_OK;
}

USound* USound::Duplicate()
{
    // Create a new USound UObject
    USound* NewSound = NewObject<USound>(GetTransientPackage());
    if (!NewSound)
        return nullptr;

    // Copy basic state
    NewSound->flags = flags;
    NewSound->status = READY;
    NewSound->volume = volume;
    NewSound->looped = 0;
    NewSound->location = location;
    NewSound->velocity = velocity;
    NewSound->min_distance = min_distance;
    NewSound->max_distance = max_distance;

    // Copy filename (legacy behavior)
    FCStringAnsi::Strcpy(NewSound->filename_ansi, filename_ansi);
    NewSound->AssetReference = AssetReference;
    NewSound->AddToRoot(); NewSound->bOwnsLegacyRoot=true;

    // Share the same sound asset (this is correct in UE)
    NewSound->SoundAsset = SoundAsset;

    // DO NOT copy AudioComponent or Attenuation instance
    // Each duplicate must spawn its own AudioComponent on Play()

    return NewSound;
}

void USound::AddToSoundCard()
{
    // Legacy compatibility:
    // In Starshatter this registered the sound with the hardware mixer.
    // In UE we just ensure the asset is loaded so Play() is fast and the sound becomes READY.
    EnsureAssetLoaded();

    // If the sound is meant to be 3D, apply attenuation defaults early:
    if (IsReady() && ((flags & LOCALIZED) || (flags & LOC_3D)))
    {
        ApplyAttenuation();
    }
}

double USound::GetTotalTime() const
{
    if (SoundAsset)
    {
        const float Duration = SoundAsset->GetDuration();
        // UE returns INDEFINITELY_LOOPING_DURATION (-1.f) for looping sounds
        if (Duration > 0.f)
            return (double)Duration;
    }

    return 0.0;
}

double USound::GetTimeRemaining() const
{
    const double Total = GetTotalTime();
    if (Total <= 0.0)
        return 0.0;

    const double Elapsed = GetTimeElapsed();
    const double Remaining = Total - Elapsed;

    return Remaining > 0.0 ? Remaining : 0.0;
}

double USound::GetTimeElapsed() const
{
    if (!bIsPlaying)
        return 0.0;

    return FPlatformTime::Seconds() - StartTimeSeconds;
}