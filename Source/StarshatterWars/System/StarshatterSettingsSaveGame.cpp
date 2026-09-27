/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe (Unreal Port)
    FILE:         StarshatterSettingsSaveGame.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Implements UStarshatterSettingsSaveGame.

    This class is primarily a data container. Validation/sanitization is
    included only for obvious safety clamps so bad values never persist.

    Adds:
      - SaveVersion + CurrentVersion
      - MigrateToCurrentVersion() hook (minimal for now)
      - Keeps SettingsVersion mirrored for UI/debug consistency
*/

#include "StarshatterSettingsSaveGame.h"
#include "GameStructs.h"

UStarshatterSettingsSaveGame::UStarshatterSettingsSaveGame()
{
    // Default metadata:
    SaveVersion = CurrentVersion;
    SettingsVersion = CurrentVersion;
    LastSavedUtc = FDateTime::UtcNow();

    // Struct defaults come from their inline initializers in GameStructs.h
}

bool UStarshatterSettingsSaveGame::MigrateToCurrentVersion()
{
    bool bChanged = false;

    // Treat invalid values as "oldest"
    if (SaveVersion < 0)
    {
        SaveVersion = 0;
        bChanged = true;
    }

    // Incremental migrations:
    while (SaveVersion < CurrentVersion)
    {
        switch (SaveVersion)
        {
        case 0:
            // v0 -> v1 migration placeholder:
            // If you later add/rename fields, do it here.
            SaveVersion = 1;
            bChanged = true;
            break;

        default:
            // Unknown version; force to current to avoid looping.
            SaveVersion = CurrentVersion;
            bChanged = true;
            break;
        }
    }

    // Keep mirror field consistent:
    if (SettingsVersion != SaveVersion)
    {
        SettingsVersion = SaveVersion;
        bChanged = true;
    }

    return bChanged;
}

void UStarshatterSettingsSaveGame::Sanitize()
{
    // Keep version mirror consistent even if caller never migrates:
    if (SaveVersion <= 0)
        SaveVersion = CurrentVersion;

    SettingsVersion = SaveVersion;

    // -------------------------
    // Audio clamps
    // -------------------------
    Audio.MasterVolume = FMath::Clamp(Audio.MasterVolume, 0.0f, 1.0f);
    Audio.MusicVolume = FMath::Clamp(Audio.MusicVolume, 0.0f, 1.0f);
    Audio.EffectsVolume = FMath::Clamp(Audio.EffectsVolume, 0.0f, 1.0f);
    Audio.VoiceVolume = FMath::Clamp(Audio.VoiceVolume, 0.0f, 1.0f);
    Audio.SoundQuality = FMath::Clamp(Audio.SoundQuality, 0, 3);

    // -------------------------
    // Video clamps
    // -------------------------
    Video.Width = FMath::Max(Video.Width, 320);
    Video.Height = FMath::Max(Video.Height, 240);

    Video.MaxTextureSize = FMath::Clamp(Video.MaxTextureSize, 64, 16384);
    Video.GammaLevel = FMath::Clamp(Video.GammaLevel, 32, 224);
    Video.DustLevel = FMath::Clamp(Video.DustLevel, 0, 3);
    Video.TerrainDetailIndex = FMath::Clamp(Video.TerrainDetailIndex, 0, 3);

    // -------------------------
    // Controls clamps
    // -------------------------
    Controls.JoystickIndex = FMath::Max(Controls.JoystickIndex, 0);
    Controls.ThrottleAxis = FMath::Max(Controls.ThrottleAxis, 0);
    Controls.RudderAxis = FMath::Max(Controls.RudderAxis, 0);
    Controls.JoystickSensitivity = FMath::Clamp(Controls.JoystickSensitivity, 0, 10);

    Controls.MouseSensitivity = FMath::Clamp(Controls.MouseSensitivity, 0, 50);
    // Controls.bMouseInvert is already boolean.

    // -------------------------
    // KeyMap sanity (lightweight)
    // -------------------------
    // Remove any bindings with invalid keys to avoid persisting garbage.
    KeyMap.Bindings.RemoveAll([](const FStarshatterInputBinding& B)
        {
            return !B.Key.IsValid();
        });

    // Update timestamp whenever sanitize is called (typical before save).
    LastSavedUtc = FDateTime::UtcNow();
}

FString UStarshatterSettingsSaveGame::GetDefaultSlotName()
{
    return TEXT("StarshatterSettings");
}

int32 UStarshatterSettingsSaveGame::GetDefaultUserIndex()
{
    return 0;
}
