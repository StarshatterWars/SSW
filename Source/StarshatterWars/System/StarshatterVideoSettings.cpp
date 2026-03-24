#include "StarshatterVideoSettings.h"
#include "Starshatter.h"

UStarshatterVideoSettings* UStarshatterVideoSettings::Get()
{
    return GetMutableDefault<UStarshatterVideoSettings>();
}

void UStarshatterVideoSettings::Load()
{
    ReloadConfig();
    Sanitize();
}

void UStarshatterVideoSettings::Save()
{
    Sanitize();
    SaveConfig();
}

void UStarshatterVideoSettings::Sanitize()
{
    Config.Width = FMath::Max(Config.Width, 320);
    Config.Height = FMath::Max(Config.Height, 240);

    Config.MaxTextureSize = FMath::Clamp(Config.MaxTextureSize, 64, 16384);
    Config.GammaLevel = FMath::Clamp(Config.GammaLevel, 32, 224);
    Config.DustLevel = FMath::Clamp(Config.DustLevel, 0, 3);
    Config.TerrainDetailIndex = FMath::Clamp(Config.TerrainDetailIndex, 0, 3);
}

void UStarshatterVideoSettings::SetConfig(const FStarshatterVideoConfig& InConfig)
{
    Config = InConfig;
    Sanitize();
}

void UStarshatterVideoSettings::ApplyToRuntimeVideo()
{
    // Migration-safe hook
    if (Starshatter* Stars = Starshatter::GetInstance())
    {
        Stars->LoadVideoConfig("video.cfg");
    }
}
