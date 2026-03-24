/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025–2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         StarshatterVideoSubsystem.cpp
    AUTHOR:       Carlos Bott
*/

#include "StarshatterVideoSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"

// Unified settings save:
#include "StarshatterSettingsSaveGame.h"
#include "GameStructs.h"

// If you have the ported Starshatter singleton available:
#include "Starshatter.h"

void UStarshatterVideoSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // Boot is orchestrated by StarshatterBootSubsystem (intentionally not auto-booting here)
}

void UStarshatterVideoSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void UStarshatterVideoSubsystem::Boot()
{
    LoadVideoConfig(TEXT("video.cfg"), true);
    ApplySettingsToRuntime();
}

UStarshatterVideoSubsystem* UStarshatterVideoSubsystem::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
        return nullptr;

    const UWorld* World = WorldContextObject->GetWorld();
    if (!World)
        return nullptr;

    return Get(World->GetGameInstance());
}

UStarshatterVideoSubsystem* UStarshatterVideoSubsystem::Get(UGameInstance* GameInstance)
{
    return GameInstance ? GameInstance->GetSubsystem<UStarshatterVideoSubsystem>() : nullptr;
}

// +--------------------------------------------------------------------+
// Path Utilities
// +--------------------------------------------------------------------+

FString UStarshatterVideoSubsystem::GetDefaultConfigDir() const
{
    return FPaths::Combine(
        FPaths::ProjectSavedDir(),
        TEXT("Starshatter"),
        TEXT("Config")
    );
}

FString UStarshatterVideoSubsystem::ResolveConfigPath(const FString& InRelativeOrAbsolutePath) const
{
    if (FPaths::IsRelative(InRelativeOrAbsolutePath))
    {
        return FPaths::Combine(GetDefaultConfigDir(), InRelativeOrAbsolutePath);
    }

    return InRelativeOrAbsolutePath;
}

// +--------------------------------------------------------------------+
// Sanitize
// +--------------------------------------------------------------------+

void UStarshatterVideoSubsystem::SanitizeConfig(FStarshatterVideoConfig& C) const
{
    C.Width = FMath::Max(320, C.Width);
    C.Height = FMath::Max(200, C.Height);

    C.MaxTextureSize = FMath::Clamp(C.MaxTextureSize, 64, 16384);
    C.GammaLevel = FMath::Clamp(C.GammaLevel, 32, 224);

    C.DustLevel = FMath::Clamp(C.DustLevel, 0, 3);
    C.TerrainDetailIndex = FMath::Clamp(C.TerrainDetailIndex, 0, 3);
}

// +--------------------------------------------------------------------+
// Load / Save video.cfg
// +--------------------------------------------------------------------+

bool UStarshatterVideoSubsystem::LoadVideoConfig(const FString& InRelativeOrAbsolutePath, bool bCreateIfMissing)
{
    const FString AbsPath = ResolveConfigPath(InRelativeOrAbsolutePath);
    const FString Dir = FPaths::GetPath(AbsPath);

    IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
    PF.CreateDirectoryTree(*Dir);

    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *AbsPath))
    {
        if (bCreateIfMissing)
        {
            // Write defaults immediately so future loads succeed:
            SaveVideoConfig(InRelativeOrAbsolutePath);
        }
        return false;
    }

    TArray<FString> Lines;
    Text.ParseIntoArrayLines(Lines);

    TMap<FString, FString> KV;

    for (FString Line : Lines)
    {
        Line = Line.TrimStartAndEnd();
        if (Line.IsEmpty())
            continue;

        if (Line.Equals(TEXT("VIDEO"), ESearchCase::IgnoreCase))
            continue;

        if (Line.StartsWith(TEXT("#")) || Line.StartsWith(TEXT("//")))
            continue;

        FString Key, Value;
        if (Line.Split(TEXT("="), &Key, &Value) || Line.Split(TEXT(":"), &Key, &Value))
        {
            KV.Add(Key.TrimStartAndEnd(), Value.TrimStartAndEnd());
        }
    }

    auto GetKV = [&](const TCHAR* K) -> const FString*
        {
            return KV.Find(FString(K));
        };

    // Map legacy keys -> CurrentConfig
    if (const FString* V = GetKV(TEXT("width")))                  ParseInt(*V, CurrentConfig.Width);
    if (const FString* V = GetKV(TEXT("height")))                 ParseInt(*V, CurrentConfig.Height);
    if (const FString* V = GetKV(TEXT("fullscreen")))             ParseBool(*V, CurrentConfig.bFullscreen);

    if (const FString* V = GetKV(TEXT("max_tex")))                ParseInt(*V, CurrentConfig.MaxTextureSize);
    if (const FString* V = GetKV(TEXT("gamma")))                  ParseInt(*V, CurrentConfig.GammaLevel);

    if (const FString* V = GetKV(TEXT("shadows")))                ParseBool(*V, CurrentConfig.bShadows);
    if (const FString* V = GetKV(TEXT("spec_maps")))              ParseBool(*V, CurrentConfig.bSpecularMaps);
    if (const FString* V = GetKV(TEXT("bump_maps")))              ParseBool(*V, CurrentConfig.bBumpMaps);

    if (const FString* V = GetKV(TEXT("flare")))                  ParseBool(*V, CurrentConfig.bLensFlare);
    if (const FString* V = GetKV(TEXT("corona")))                 ParseBool(*V, CurrentConfig.bCorona);
    if (const FString* V = GetKV(TEXT("nebula")))                 ParseBool(*V, CurrentConfig.bNebula);

    if (const FString* V = GetKV(TEXT("dust")))                   ParseInt(*V, CurrentConfig.DustLevel);

    if (const FString* V = GetKV(TEXT("terrain_detail_level")))   ParseInt(*V, CurrentConfig.TerrainDetailIndex);
    if (const FString* V = GetKV(TEXT("terrain_texture_enable"))) ParseBool(*V, CurrentConfig.bTerrainTextures);

    SanitizeConfig(CurrentConfig);
    return true;
}

bool UStarshatterVideoSubsystem::SaveVideoConfig(const FString& InRelativeOrAbsolutePath) const
{
    const FString AbsPath = ResolveConfigPath(InRelativeOrAbsolutePath);
    const FString Dir = FPaths::GetPath(AbsPath);

    IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
    PF.CreateDirectoryTree(*Dir);

    // Work on a sanitized copy:
    FStarshatterVideoConfig C = CurrentConfig;
    // const method: sanitize via local copy logic:
    C.Width = FMath::Max(320, C.Width);
    C.Height = FMath::Max(200, C.Height);
    C.MaxTextureSize = FMath::Clamp(C.MaxTextureSize, 64, 16384);
    C.GammaLevel = FMath::Clamp(C.GammaLevel, 32, 224);
    C.DustLevel = FMath::Clamp(C.DustLevel, 0, 3);
    C.TerrainDetailIndex = FMath::Clamp(C.TerrainDetailIndex, 0, 3);

    TArray<FString> Lines;

    Lines.Add(TEXT("VIDEO"));
    Lines.Add(TEXT(""));

    WriteLine(Lines, TEXT("width"), FString::FromInt(C.Width));
    WriteLine(Lines, TEXT("height"), FString::FromInt(C.Height));
    WriteLine(Lines, TEXT("depth"), TEXT("32"));
    Lines.Add(TEXT(""));

    WriteLine(Lines, TEXT("max_tex"), FString::FromInt(C.MaxTextureSize));
    WriteLine(Lines, TEXT("primary3D"), TEXT("true"));
    WriteLine(Lines, TEXT("gamma"), FString::FromInt(C.GammaLevel));
    Lines.Add(TEXT(""));

    WriteLine(Lines, TEXT("terrain_detail_level"), FString::FromInt(C.TerrainDetailIndex));
    WriteLine(Lines, TEXT("terrain_texture_enable"), C.bTerrainTextures ? TEXT("true") : TEXT("false"));
    Lines.Add(TEXT(""));

    WriteLine(Lines, TEXT("shadows"), C.bShadows ? TEXT("true") : TEXT("false"));
    WriteLine(Lines, TEXT("spec_maps"), C.bSpecularMaps ? TEXT("true") : TEXT("false"));
    WriteLine(Lines, TEXT("bump_maps"), C.bBumpMaps ? TEXT("true") : TEXT("false"));

    // Legacy supports bias but your struct doesn't; keep a default:
    WriteLine(Lines, TEXT("bias"), TEXT("0.0"));
    Lines.Add(TEXT(""));

    WriteLine(Lines, TEXT("flare"), C.bLensFlare ? TEXT("true") : TEXT("false"));
    WriteLine(Lines, TEXT("corona"), C.bCorona ? TEXT("true") : TEXT("false"));
    WriteLine(Lines, TEXT("nebula"), C.bNebula ? TEXT("true") : TEXT("false"));
    WriteLine(Lines, TEXT("dust"), FString::FromInt(C.DustLevel));

    return FFileHelper::SaveStringToFile(FString::Join(Lines, TEXT("\n")) + TEXT("\n"), *AbsPath);
}

// +--------------------------------------------------------------------+
// Config Access
// +--------------------------------------------------------------------+

void UStarshatterVideoSubsystem::SetConfig(const FStarshatterVideoConfig& NewConfig)
{
    CurrentConfig = NewConfig;
    SanitizeConfig(CurrentConfig);
}

// +--------------------------------------------------------------------+
// Deferred Change Handling
// +--------------------------------------------------------------------+

void UStarshatterVideoSubsystem::RequestChangeVideo(const FStarshatterVideoConfig& NewPendingConfig)
{
    PendingConfig = NewPendingConfig;
    SanitizeConfig(PendingConfig);

    bPendingChange = true;
    OnVideoChangeRequested.Broadcast(PendingConfig);
}

bool UStarshatterVideoSubsystem::ConsumePendingChange(FStarshatterVideoConfig& OutPending)
{
    if (!bPendingChange)
        return false;

    OutPending = PendingConfig;
    CurrentConfig = PendingConfig;
    bPendingChange = false;

    return true;
}

// +--------------------------------------------------------------------+
// Runtime Apply Hook (no-arg)
// +--------------------------------------------------------------------+

void UStarshatterVideoSubsystem::ApplySettingsToRuntime()
{
    // Persist current config to disk first:
    SaveVideoConfig(TEXT("video.cfg"));

    if (Starshatter* Stars = Starshatter::GetInstance())
    {
        // Migration-safe behavior:
        // - If your core has RequestChangeVideo(), call it when you're ready.
        // - Otherwise just reload the config.
        //
        // Keep it conservative: reload cfg (stable) and optionally request a change if you want:
        Stars->LoadVideoConfig("video.cfg");

        // If you DO have mode-change handling in core, flip this on later:
        // Stars->RequestChangeVideo();
    }
    else
    {
        // No core yet: broadcast intent for whoever cares (renderer/UI).
        OnVideoChangeRequested.Broadcast(CurrentConfig);
    }
}

// +--------------------------------------------------------------------+
// SaveGame bridging
// +--------------------------------------------------------------------+

void UStarshatterVideoSubsystem::LoadFromSaveGame(const UStarshatterSettingsSaveGame* SaveGame)
{
    if (!SaveGame)
        return;

    CurrentConfig = SaveGame->Video;
    SanitizeConfig(CurrentConfig);
}

void UStarshatterVideoSubsystem::WriteToSaveGame(UStarshatterSettingsSaveGame* SaveGame) const
{
    if (!SaveGame)
        return;

    SaveGame->Video = CurrentConfig;
}

// +--------------------------------------------------------------------+
// Parsing Helpers
// +--------------------------------------------------------------------+

bool UStarshatterVideoSubsystem::ParseBool(const FString& Value, bool& OutBool)
{
    const FString V = Value.TrimStartAndEnd().ToLower();
    OutBool = (V == TEXT("1") || V == TEXT("true") || V == TEXT("yes") || V == TEXT("on"));
    return true;
}

bool UStarshatterVideoSubsystem::ParseInt(const FString& Value, int32& OutInt)
{
    OutInt = FCString::Atoi(*Value);
    return true;
}

bool UStarshatterVideoSubsystem::ParseFloat(const FString& Value, float& OutFloat)
{
    OutFloat = FCString::Atof(*Value);
    return true;
}

void UStarshatterVideoSubsystem::WriteLine(TArray<FString>& Lines, const FString& Key, const FString& Value)
{
    Lines.Add(Key + TEXT(": ") + Value);
}
