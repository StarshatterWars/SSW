#include "ResetCampaignAcceptanceCommandlet.h"
#include "PlayerSaveGame.h"
#include "LocalGameStateSave.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"

UResetCampaignAcceptanceCommandlet::UResetCampaignAcceptanceCommandlet()
{
    IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true;
}

int32 UResetCampaignAcceptanceCommandlet::Main(const FString& Params)
{
    FString Profile;
    if(!FParse::Value(*Params,TEXT("ProfileSlot="),Profile) || !Profile.StartsWith(TEXT("PlayerSave_")) ||
        Profile.Contains(TEXT("/")) || Profile.Contains(TEXT("\\")) || Profile.Contains(TEXT("..")))return 1;
    auto* Primary=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromSlot(Profile,0));
    if(!Primary || Primary->PlayerInfo.Campaign!=2)
    {UE_LOG(LogTemp,Error,TEXT("[CampaignReset] Expected an accepted Campaign 2 account save."));return 2;}
    TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(FPaths::ProjectSavedDir()/TEXT("SaveGames")/(Profile+TEXT("_Profile_*.sav"))),true,false);
    TArray<FString> Slots={Profile};TArray<UPlayerSaveGame*> Saves={Primary};
    int64 Revision=Primary->PlayerInfo.LocalStateRevision;
    for(const FString& File:Files)
    {
        const FString Slot=FPaths::GetBaseFilename(File);
        auto* Save=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot,0));
        if(Save && Save->PlayerInfo.Name==Primary->PlayerInfo.Name && Save->PlayerInfo.Callsign==Primary->PlayerInfo.Callsign && Save->PlayerInfo.Campaign==2)
        {Slots.Add(Slot);Saves.Add(Save);Revision=FMath::Max(Revision,Save->PlayerInfo.LocalStateRevision);}
    }
    if(Saves.Num()!=2){UE_LOG(LogTemp,Error,TEXT("[CampaignReset] Expected exactly one matching active profile; no files changed."));return 3;}
    TArray<FString> Manifests;IFileManager::Get().FindFiles(Manifests,*(FPaths::ProjectSavedDir()/TEXT("SaveGames/State_*_manifest.sav")),true,false);
    for(const auto& File:Manifests)
        if(auto* M=Cast<ULocalStateManifest>(UGameplayStatics::LoadGameFromSlot(FPaths::GetBaseFilename(File),0));M && Slots.Contains(M->ProfileSlot))
            Revision=FMath::Max(Revision,M->PlayerRevision);
    if(Revision==MAX_int64)return 4;
    for(int32 Index=0;Index<Saves.Num();++Index)
    {
        auto& Info=Saves[Index]->PlayerInfo;
        Info.Campaign=1;Info.CampaignRowName=TEXT("Operation Live Fire");Info.PendingFleetDeploymentCampaign=0;Info.LocalStateRevision=Revision+1;
        if(!UGameplayStatics::SaveGameToSlot(Saves[Index],Slots[Index],0))return 5;
        auto* Verify=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromSlot(Slots[Index],0));
        if(!Verify || !FS_PlayerGameInfo::StaticStruct()->CompareScriptStruct(&Info,&Verify->PlayerInfo,0))return 6;
        UE_LOG(LogTemp,Display,TEXT("[CampaignReset] VERIFIED %s: Campaign 1 restored; training, wallet, ships and other data retained. Revision=%lld"),*Slots[Index],Revision+1);
    }
    return 0;
}
