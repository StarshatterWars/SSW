#include "PilotSaveRecovery.h"
#include "LocalGameStateSubsystem.h"
#include "PlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/StrongObjectPtr.h"

namespace PilotSaveRecovery
{
namespace
{
USaveGame* Load(const FString& Directory,const FString& Slot)
{
    TArray<uint8> Bytes;
    return FFileHelper::LoadFileToArray(Bytes,*(Directory/(Slot+TEXT(".sav"))))
        ? UGameplayStatics::LoadGameFromMemory(Bytes) : nullptr;
}
bool Read(const FString& Directory,const FString& Slot,FS_PlayerGameInfo& Player,FString& Profile,int64& Utc)
{
    if(Slot.IsEmpty() || Slot.Contains(TEXT("/")) || Slot.Contains(TEXT("\\")) || Slot.Contains(TEXT("..")) || Slot.Contains(TEXT(":")))return false;
    TStrongObjectPtr<ULocalStateManifest> M(Cast<ULocalStateManifest>(Load(Directory,Slot)));
    if(!M || M->Slots.Num()!=4 || M->Version!=1 || M->SavedUtc<=0)return false;
    Profile=M->ProfileSlot;
    // Only native account/profile slot names; no path traversal or another file type.
    const FString Prefix=TEXT("PlayerSave_"); const FString Separator=TEXT("_Profile_");
    FString Account,Guid;
    if(!Profile.Split(Separator,&Account,&Guid) || Guid.Len()!=32)return false;
    if(Account!=TEXT("PlayerSave") && (!Account.StartsWith(Prefix) || Account.Len()!=Prefix.Len()+32))return false;
    const FString Hex=(Account==TEXT("PlayerSave")?FString():Account.Mid(Prefix.Len()))+Guid;
    for(TCHAR C:Hex)if(!FChar::IsHexDigit(C))return false;
    if(Slot!=TEXT("State_")+FMD5::HashAnsiString(*Profile)+TEXT("_manifest") &&
       Slot!=TEXT("State_")+FMD5::HashAnsiString(*Profile)+TEXT("_manifest_previous"))return false;
    TArray<TStrongObjectPtr<ULocalStateArea>> Held;
    TArray<ULocalStateArea*> Areas;
    // Validate filenames before opening any area supplied by the manifest.
    const FString Base=TEXT("State_")+FMD5::HashAnsiString(*Profile)+TEXT("_")+M->Generation.ToString(EGuidFormats::Digits)+TEXT("_");
    const TCHAR* Names[]={TEXT("pilot"),TEXT("economy"),TEXT("skills"),TEXT("universe")};
    for(int32 I=0;I<4;++I)
    {
        if(M->Slots[I]!=Base+Names[I])return false;
        Held.Emplace(Cast<ULocalStateArea>(Load(Directory,M->Slots[I]))); Areas.Add(Held.Last().Get());
    }
    if(!ULocalGameStateSubsystem::ValidateGeneration(*M,Areas))return false;
    Player=CastChecked<ULocalPilotState>(Areas[0])->Pilot;
    Player.Economy=CastChecked<ULocalEconomyState>(Areas[1])->Economy;
    Player.Skills=CastChecked<ULocalSkillState>(Areas[2])->Skills;
    Player.SkillQueue=CastChecked<ULocalSkillState>(Areas[2])->Queue;
    Utc=M->SavedUtc; return true;
}
}
TSharedRef<FJsonObject> Execute(const FJsonObject& Request,const FString& Directory)
{
    auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("success"),false);
    FString Action;Request.TryGetStringField(TEXT("action"),Action);
    TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(Directory/TEXT("State_*_manifest*.sav")),true,false);
    TArray<TSharedPtr<FJsonValue>> Checkpoints;
    for(const auto& File:Files)
    {
        FS_PlayerGameInfo P;FString Profile;int64 Utc=0;
        const FString Slot=FPaths::GetBaseFilename(File);
        if(!Read(Directory,Slot,P,Profile,Utc))continue;
        auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("slot"),Slot);
        Item->SetStringField(TEXT("profile"),Profile);Item->SetStringField(TEXT("pilot"),P.Callsign.IsEmpty()?P.Name:P.Callsign);
        Item->SetStringField(TEXT("savedUtc"),FDateTime::FromUnixTimestamp(Utc).ToIso8601());
        Item->SetNumberField(TEXT("campaign"),P.Campaign);Item->SetNumberField(TEXT("points"),P.PlayerPoints);
        Checkpoints.Add(MakeShared<FJsonValueObject>(Item));
    }
    Result->SetArrayField(TEXT("checkpoints"),Checkpoints);
    if(Action==TEXT("list")){Result->SetBoolField(TEXT("success"),true);return Result;}
    FString Selected,ExpectedProfile,ExpectedUtc;
    Request.TryGetStringField(TEXT("slot"),Selected);Request.TryGetStringField(TEXT("profile"),ExpectedProfile);
    Request.TryGetStringField(TEXT("savedUtc"),ExpectedUtc);
    FS_PlayerGameInfo P;FString Profile;int64 Utc=0;
    if(Action!=TEXT("restore") || !Read(Directory,Selected,P,Profile,Utc) || Profile!=ExpectedProfile ||
       FDateTime::FromUnixTimestamp(Utc).ToIso8601()!=ExpectedUtc)
    {Result->SetStringField(TEXT("error"),TEXT("The selected checkpoint changed or failed validation. No saves were replaced."));return Result;}
    FString Account,Unused;Profile.Split(TEXT("_Profile_"),&Account,&Unused);
    TStrongObjectPtr<UPlayerSaveGame> Save(NewObject<UPlayerSaveGame>());Save->SaveVersion=2;Save->PlayerInfo=P;
    TStrongObjectPtr<UPlayerSaveIndexGame> Index(Cast<UPlayerSaveIndexGame>(Load(Directory,Account+TEXT("_Profiles"))));
    if(Index && Index->Version!=1){Result->SetStringField(TEXT("error"),TEXT("This profile index requires a newer game version."));return Result;}
    if(!Index)
    {
        Index.Reset(NewObject<UPlayerSaveIndexGame>());
        // Preserve discoverable sibling profiles when rebuilding a damaged index.
        TArray<FString> Profiles;IFileManager::Get().FindFiles(Profiles,*(Directory/(Account+TEXT("_Profile_*.sav"))),true,false);
        for(const auto& File:Profiles)
        {
            TStrongObjectPtr<UPlayerSaveGame> Other(Cast<UPlayerSaveGame>(Load(Directory,FPaths::GetBaseFilename(File))));
            if(!Other)continue;
            FPlayerSaveSlotInfo Row;Row.Slot=FPaths::GetBaseFilename(File);Row.DisplayName=Other->PlayerInfo.Callsign;
            Index->Players.Add(Row);
        }
    }
    Index->ActiveSlot=Profile;
    if(!Index->Players.ContainsByPredicate([&](const FPlayerSaveSlotInfo& Row){return Row.Slot==Profile;}))
    {FPlayerSaveSlotInfo Row;Row.Slot=Profile;Row.DisplayName=P.Callsign.IsEmpty()?P.Name:P.Callsign;Row.SavedAt=FDateTime::FromUnixTimestamp(Utc);Index->Players.Add(Row);}
    TArray<uint8> PilotBytes,IndexBytes;
    if(!UGameplayStatics::SaveGameToMemory(Save.Get(),PilotBytes) || !UGameplayStatics::SaveGameToMemory(Index.Get(),IndexBytes))return Result;
    TStrongObjectPtr<UPlayerSaveGame> Verified(Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(PilotBytes)));
    TStrongObjectPtr<UPlayerSaveIndexGame> VerifiedIndex(Cast<UPlayerSaveIndexGame>(UGameplayStatics::LoadGameFromMemory(IndexBytes)));
    if(!Verified || !VerifiedIndex || VerifiedIndex->ActiveSlot!=Profile || Verified->PlayerInfo.PlayerPoints!=P.PlayerPoints ||
       Verified->PlayerInfo.LocalStateRevision!=P.LocalStateRevision)return Result;
    const FString Backup=FPaths::GetPath(Directory)/TEXT("PilotRecoveryBackups")/(FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S"))+TEXT("-")+FGuid::NewGuid().ToString(EGuidFormats::Digits));
    Result->SetStringField(TEXT("backup"),Backup);
    if(!IFileManager::Get().MakeDirectory(*Backup,true))return Result;
    // Copy every save, including campaigns and checkpoints, and verify byte for byte.
    IFileManager::Get().FindFiles(Files,*(Directory/TEXT("*.sav")),true,false);
    for(const FString& File:Files)
    {
        TArray<uint8> Before,After;
        if(!FFileHelper::LoadFileToArray(Before,*(Directory/File)) || !FFileHelper::SaveArrayToFile(Before,*(Backup/File)) ||
           !FFileHelper::LoadFileToArray(After,*(Backup/File)) || Before!=After)
        {Result->SetStringField(TEXT("error"),TEXT("Backup verification failed. No saves were replaced."));return Result;}
    }
    TArray<uint8> ManifestBytes;
    if(!FFileHelper::LoadFileToArray(ManifestBytes,*(Directory/(Selected+TEXT(".sav")))))return Result;
    // Selecting an older checkpoint must also select its world/economy generation.
    const TArray<FString> Targets={Account+TEXT(".sav"),Profile+TEXT(".sav"),Account+TEXT("_Profiles.sav"),TEXT("State_")+FMD5::HashAnsiString(*Profile)+TEXT("_manifest.sav")};
    const TArray<TArray<uint8>> Payloads={PilotBytes,PilotBytes,IndexBytes,ManifestBytes};
    int32 Attempted=0;bool Ok=true;
    for(int32 I=0;I<Targets.Num();++I)
    {
        const FString Target=Directory/Targets[I],Temp=Target+TEXT(".recovery.tmp");
        TArray<uint8> Check;
        Attempted=I+1;
        Ok=FFileHelper::SaveArrayToFile(Payloads[I],*Temp) && FFileHelper::LoadFileToArray(Check,*Temp) && Check==Payloads[I] &&
            IFileManager::Get().Move(*Target,*Temp,true,true) && FFileHelper::LoadFileToArray(Check,*Target) && Check==Payloads[I];
        if(!Ok)break;
    }
    if(!Ok)
    {
        bool RolledBack=true;
        for(int32 I=0;I<Attempted;++I)
        {
            const FString Original=Backup/Targets[I],Target=Directory/Targets[I];
            if(IFileManager::Get().FileExists(*Original))RolledBack&=IFileManager::Get().Copy(*Target,*Original,true,true)==COPY_OK;
            else RolledBack&=IFileManager::Get().Delete(*Target,false,true);
        }
        Result->SetStringField(TEXT("error"),RolledBack?TEXT("Restore failed; original saves were put back."):TEXT("Restore failed. Keep the recovery backup and contact support."));return Result;
    }
    Result->SetBoolField(TEXT("success"),true);return Result;
}
bool IsRequested(){return FParse::Param(FCommandLine::Get(),TEXT("SSWPilotRecovery"));}
void RunRequested()
{
    FString Path,Text;FParse::Value(FCommandLine::Get(),TEXT("RecoveryRequest="),Path);
    TSharedPtr<FJsonObject> Request;TSharedRef<FJsonObject> Result=MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"),false);
    if(FFileHelper::LoadFileToString(Text,*Path) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Request) && Request)
        Result=Execute(*Request,FPaths::ProjectSavedDir()/TEXT("SaveGames"));
    FString Output;FJsonSerializer::Serialize(Result,TJsonWriterFactory<>::Create(&Output));
    const bool Written=FFileHelper::SaveStringToFile(Output,*(Path+TEXT(".result.json")));
    // This maintenance instance deliberately never initialized gameplay subsystems.
    // Exit here rather than continuing map/local-player initialization or shutdown.
    FPlatformMisc::RequestExitWithStatus(true,Written && Result->GetBoolField(TEXT("success"))?0:2);
}
}
