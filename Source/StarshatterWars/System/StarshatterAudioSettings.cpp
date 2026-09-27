#include "StarshatterAudioSettings.h"

#include "Engine/Engine.h"
#include "Misc/ConfigCacheIni.h"
#include "Kismet/GameplayStatics.h"

UStarshatterAudioSettings* UStarshatterAudioSettings::Get()
{
    // Use class default object (always exists, config-backed)
    return GetMutableDefault<UStarshatterAudioSettings>();
}

void UStarshatterAudioSettings::Load()
{
    // CDO already loads config automatically, but this forces a reload
    // if the ini changed during runtime.
    ReloadConfig();
    Sanitize();
}

void UStarshatterAudioSettings::Sanitize()
{
    MasterVolume = Clamp01(MasterVolume);
    MusicVolume = Clamp01(MusicVolume);
    EffectsVolume = Clamp01(EffectsVolume);
    VoiceVolume = Clamp01(VoiceVolume);

    // Keep quality index sane (adjust max as you like)
    SoundQuality = FMath::Clamp(SoundQuality, 0, 3);
}

void UStarshatterAudioSettings::Save() const
{
    // Save current object values into config
    const_cast<UStarshatterAudioSettings*>(this)->SaveConfig();
}

void UStarshatterAudioSettings::ApplyToRuntimeAudio(UObject* WorldContextObject) const
{
    // Stub: UE-native runtime routing goes here.
    // Examples you might implement next:
    // - Set SoundClass volumes (Master/Music/SFX/Voice)
    // - Apply a SoundMix override
    // - Apply Audio Modulation parameters
    //
    // Leaving this as a no-op is fine and compiles cleanly.

    (void)WorldContextObject;
}

// -------------------------
// Setters
// -------------------------

void UStarshatterAudioSettings::SetMasterVolume(float V)
{
    MasterVolume = Clamp01(V);
}

void UStarshatterAudioSettings::SetMusicVolume(float V)
{
    MusicVolume = Clamp01(V);
}

void UStarshatterAudioSettings::SetEffectsVolume(float V)
{
    EffectsVolume = Clamp01(V);
}

void UStarshatterAudioSettings::SetVoiceVolume(float V)
{
    VoiceVolume = Clamp01(V);
}

void UStarshatterAudioSettings::SetSoundQuality(int32 Index)
{
    SoundQuality = FMath::Clamp(Index, 0, 3);
}
