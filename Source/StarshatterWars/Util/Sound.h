#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Sound.generated.h"

// ------------------------------------------------------------------
// HRESULT compatibility shim (legacy Starshatter code expects HRESULT,
// S_OK, E_FAIL, E_NOINTERFACE). Avoids pulling Windows headers into UE.
// ------------------------------------------------------------------
#ifndef HRESULT
typedef long HRESULT;
#endif

#ifndef S_OK
#define S_OK ((HRESULT)0L)
#endif

#ifndef E_FAIL
#define E_FAIL ((HRESULT)0x80004005L)
#endif

#ifndef E_NOINTERFACE
#define E_NOINTERFACE ((HRESULT)0x80004002L)
#endif

class Camera; // kept only to preserve your legacy SetListener signature

UCLASS(BlueprintType)
class STARSHATTERWARS_API USound : public UObject
{
    GENERATED_BODY()

public:
    static const char* TYPENAME() { return "Sound"; }

    // ------------------------------------------------------------------
    // Required UE replacement for legacy "SoundCard":
    // Call once early (GameInstance Init is perfect).
    static void SetWorldContext(UObject* InWorldContext);

    // Legacy factories (now load USoundBase assets):
    static USound* CreateStream(const char* InFilename);
    static USound* CreateOggStream(const char* InFilename); // same behavior as CreateStream in UE

    // Legacy listener hook (UE manages listener; we store velocity for future doppler use):
    static void SetListener(const Camera& /*cam*/, const FVector& InVel);

public:
    USound();

    // once per frame:
    virtual void Update();

    // mark for collection:
    virtual void Release();

    // transport operations:
    virtual HRESULT Play();
    virtual HRESULT Rewind();
    virtual HRESULT Pause();
    virtual HRESULT Stop();

    virtual void AddToSoundCard();

    // Stream timing (legacy compatibility)
    virtual double GetTotalTime() const;
    virtual double GetTimeRemaining() const;
    virtual double GetTimeElapsed() const;


    // accessors / mutators:
    int  IsReady()   const { return status == READY; }
    int  IsPlaying() const { return status == PLAYING; }
    int  IsDone()    const { return status == DONE; }
    int  LoopCount() const { return looped; }

    // Legacy duplication: creates a new USound sharing the same asset/settings
    virtual USound* Duplicate();

    virtual DWORD GetFlags()  const { return flags; }
    virtual void  SetFlags(DWORD f) { flags = f; }
    virtual DWORD GetStatus() const { return status; }

    // Legacy volume is centibels (0 .. -10000). We convert to UE linear multiplier.
    virtual long GetVolume() const { return volume; }
    virtual void SetVolume(long v);

    virtual long GetPan() const { return 0; }
    virtual void SetPan(long /*p*/) {}

    // 3D localization:
    virtual const FVector& GetLocation() const { return location; }
    virtual void           SetLocation(const FVector& l);

    virtual const FVector& GetVelocity() const { return velocity; }
    virtual void           SetVelocity(const FVector& v) { velocity = v; }

    virtual float GetMinDistance() const { return min_distance; }
    virtual void  SetMinDistance(float f) { min_distance = f; ApplyAttenuation(); }

    virtual float GetMaxDistance() const { return max_distance; }
    virtual void  SetMaxDistance(float f) { max_distance = f; ApplyAttenuation(); }

    const char* GetFilename() const { return filename_ansi; }
    void        SetFilename(const char* s);

public:
    enum FlagEnum : uint32
    {
        AMBIENT = 0x0000,
        LOCALIZED = 0x0001,
        LOC_3D = 0x0002,
        MEMORY = 0x0000,
        STREAMED = 0x0004,
        ONCE = 0x0000,
        LOOP = 0x0008,
        FREE = 0x0000,
        LOCKED = 0x0010,
        DOPPLER = 0x0020,
        INTERFACE = 0x0040,
        OGGVORBIS = 0x4000,
        RESOURCE = 0x8000
    };

    enum StatusEnum : uint32
    {
        UNINITIALIZED,
        INITIALIZING,
        READY,
        PLAYING,
        DONE
    };

protected:
    double StartTimeSeconds = 0.0;
    bool   bIsPlaying = false;

private:
    void EnsureAssetLoaded();
    void EnsureAudioComponent();
    void ApplyAttenuation();

    static UWorld* GetWorldFromContext();
    static USoundBase* LoadSoundAssetFromString(const FString& InRef);
    static float CentibelsToVolumeMultiplier(long Centibels);

private:
    UPROPERTY(Transient)
    TObjectPtr<USoundBase> SoundAsset = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> AudioComp = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<class USoundAttenuation> AttenuationAsset = nullptr;

private:
    DWORD flags = 0;
    DWORD status = UNINITIALIZED;
    long  volume = 0; // centibels, (0 .. -10000)
    int   looped = 0;

    FVector location = FVector::ZeroVector;
    FVector velocity = FVector::ZeroVector;

    float min_distance = 200.f;
    float max_distance = 4000.f;

    char filename_ansi[64] = { 0 };

private:
    static TWeakObjectPtr<UObject> WorldContext;
    static FVector ListenerVelocity;
};
