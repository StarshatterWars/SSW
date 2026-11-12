#include "SSWMissionMods.h"

#include "HAL/FileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"

namespace
{
    constexpr const TCHAR* Section = TEXT("Starshatter.MissionMods");
}

FString SSWMissionMods::Directory()
{
    return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Mods"));
}

TArray<SSWMissionMods::FPack> SSWMissionMods::Discover()
{
    TArray<FString> Names;
    IFileManager::Get().FindFiles(Names, *(Directory() / TEXT("*")), false, true);
    Names.Sort();

    TArray<FPack> Packs;
    for (const FString& Name : Names)
    {
        const FString Folder = Directory() / Name / TEXT("Missions");
        FPack Pack;
        IFileManager::Get().FindFiles(Pack.Missions, *(Folder / TEXT("*.def")), true, false);
        if (Pack.Missions.IsEmpty()) continue;
        Pack.Name = Name;
        Pack.Missions.Sort();
        for (FString& File : Pack.Missions) File = Folder / File;
        if (GConfig) GConfig->GetBool(Section, *Name, Pack.bEnabled, GGameUserSettingsIni);
        Packs.Add(MoveTemp(Pack));
    }
    return Packs;
}

void SSWMissionMods::SetEnabled(const FString& Name, bool bEnabled)
{
    if (!GConfig) return;
    GConfig->SetBool(Section, *Name, bEnabled, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}
