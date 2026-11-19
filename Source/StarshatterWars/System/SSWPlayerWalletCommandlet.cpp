#include "SSWPlayerWalletCommandlet.h"
#include "PlayerSaveGame.h"
#include "TransactionAI.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"
#include "UObject/StrongObjectPtr.h"
USSWPlayerWalletCommandlet::USSWPlayerWalletCommandlet(){IsEditor=true;IsClient=false;IsServer=false;LogToConsole=true;}
int32 USSWPlayerWalletCommandlet::Main(const FString& Params)
{
    FString Root,Base;int64 Balance=-1;
    if(!FParse::Value(*Params,TEXT("SaveRoot="),Root) || !FParse::Value(*Params,TEXT("BaseSlot="),Base) || !FParse::Value(*Params,TEXT("Balance="),Balance) || Balance<0 || Base.Contains(TEXT("/")) || Base.Contains(TEXT("\\")) || Base.Contains(TEXT("..")))return 1;
    Root=FPaths::ConvertRelativePathToFull(Root);
    auto Load=[](const FString& File){TArray<uint8> Bytes;return FFileHelper::LoadFileToArray(Bytes,*File)?UGameplayStatics::LoadGameFromMemory(Bytes):nullptr;};
    TStrongObjectPtr<UPlayerSaveIndexGame> Index(Cast<UPlayerSaveIndexGame>(Load(Root/(Base+TEXT("_Profiles.sav")))));
    if(!Index.IsValid() || !Index->ActiveSlot.StartsWith(Base+TEXT("_Profile_")) || Index->ActiveSlot.Contains(TEXT("/")) || Index->ActiveSlot.Contains(TEXT("\\")))return 2;
    const TArray<FString> Files={Root/(Base+TEXT(".sav")),Root/(Index->ActiveSlot+TEXT(".sav"))};
    TArray<TArray<uint8>> Original,Updated;FString Pilot;
    for(const auto& File:Files)
    {
        TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*File))return 3;Original.Add(Bytes);
        TStrongObjectPtr<UPlayerSaveGame> Save(Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes)));
        if(!Save.IsValid() || Save->PlayerInfo.Name.IsEmpty() || (!Pilot.IsEmpty() && Pilot!=Save->PlayerInfo.Name))return 4;Pilot=Save->PlayerInfo.Name;
        auto& E=Save->PlayerInfo.Economy;E.bLegacyWalletMigrated=true;Save->PlayerInfo.InsuranceCredits=0;
        if(!TransactionAI::ReconcileExternalBalance(E,FEconomyLedger::Pilot(),Balance,TEXT("Requested test balance"),TEXT("Test account adjustment")))return 5;
        TArray<uint8> Out;if(!UGameplayStatics::SaveGameToMemory(Save.Get(),Out))return 6;
        TStrongObjectPtr<UPlayerSaveGame> Check(Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Out)));
        if(!Check.IsValid() || TransactionAI::Balance(Check->PlayerInfo.Economy)!=Balance)return 7;Updated.Add(MoveTemp(Out));
    }
    const FString Backup=FPaths::ProjectSavedDir()/TEXT("Backups")/(TEXT("Wallet-")+FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S")));
    IFileManager::Get().MakeDirectory(*Backup,true);
    for(int32 I=0;I<Files.Num();++I)if(!FFileHelper::SaveArrayToFile(Original[I],*(Backup/FPaths::GetCleanFilename(Files[I]))))return 8;
    for(int32 I=0;I<Files.Num();++I)if(!FFileHelper::SaveArrayToFile(Updated[I],*Files[I]))
    {for(int32 J=0;J<Files.Num();++J)FFileHelper::SaveArrayToFile(Original[J],*Files[J]);return 9;}
    UE_LOG(LogTemp,Display,TEXT("[PlayerWallet] Pilot='%s' Balance=%lld CR. Active profile and resume slot verified. Backup=%s"),*Pilot,Balance,*Backup);return 0;
}
