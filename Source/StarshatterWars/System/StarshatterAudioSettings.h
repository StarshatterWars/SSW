/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe (Unreal Port)
    FILE:         StarshatterAudioSettings.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UStarshatterAudioSettings
    - UE-native audio settings container (config-backed).
    - Replaces legacy audio.cfg pipeline for runtime and persistence.
    - Supports Load/Save/Sanitize and runtime apply.
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StarshatterAudioSettings.generated.h"

UCLASS(Config = Game, DefaultConfig)
class STARSHATTERWARS_API UStarshatterAudioSettings : public UObject
{
    GENERATED_BODY()

public:
    // Singleton-like convenience accessor.
    // Returns the class default object (CDO) so it is always valid.
    static UStarshatterAudioSettings* Get();

    // Load from config into this object (CDO usually).
    void Load();

    // Clamp values into sane ranges.
    void Sanitize();

    // Save current values back to config.
    void Save() const;

    // Optional: apply to runtime audio (SoundMix/SoundClass/AudioModulation etc.)
    // You can leave this as a no-op until you wire actual SoundClasses.
    void ApplyToRuntimeAudio(UObject* WorldContextObject) const;

public:
    // -----------------------------
    // Getters
    // -----------------------------
    float GetMasterVolume() const { return MasterVolume; }
    float GetMusicVolume()  const { return MusicVolume; }
    float GetEffectsVolume() const { return EffectsVolume; }
    float GetVoiceVolume()  const { return VoiceVolume; }
    int32 GetSoundQuality() const { return SoundQuality; }

    // -----------------------------
    // Setters (auto-sanitize)
    // -----------------------------
    void SetMasterVolume(float V);
    void SetMusicVolume(float V);
    void SetEffectsVolume(float V);
    void SetVoiceVolume(float V);
    void SetSoundQuality(int32 Index);

private:
    static float Clamp01(float V) { return FMath::Clamp(V, 0.0f, 1.0f); }

private:
    // Stored in DefaultGame.ini (and user Saved/Config overrides)
    // Keep the property names stable once shipped.
    UPROPERTY(Config, EditAnywhere, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MasterVolume = 1.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MusicVolume = 1.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EffectsVolume = 1.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float VoiceVolume = 1.0f;

    // 0..N quality index (you decide what it maps to)
    UPROPERTY(Config, EditAnywhere, Category = "Audio", meta = (ClampMin = "0", ClampMax = "3"))
    int32 SoundQuality = 1;
};
