#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "PilotSaveRecovery.h"
#include "LocalGameStateSave.h"
#include "PlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPilotSaveRecoveryTest,"Starshatter.Persistence.PilotRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPilotSaveRecoveryTest::RunTest(const FString&)
{
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("Automation/PilotRecovery")/FGuid::NewGuid().ToString(EGuidFormats::Digits)/TEXT("SaveGames");
    IFileManager::Get().MakeDirectory(*Dir,true);
    const FString Account=TEXT("PlayerSave_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),Profile=Account+TEXT("_Profile_bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    const FString Manifest=TEXT("State_")+FMD5::HashAnsiString(*Profile)+TEXT("_manifest");
    auto Write=[&](USaveGame* Save,const FString& Slot){TArray<uint8> Bytes;return UGameplayStatics::SaveGameToMemory(Save,Bytes) && FFileHelper::SaveArrayToFile(Bytes,*(Dir/(Slot+TEXT(".sav"))));};
    auto ReadBytes=[&](const FString& Path){TArray<uint8> Bytes;FFileHelper::LoadFileToArray(Bytes,*Path);return Bytes;};
    TStrongObjectPtr<ULocalStateManifest> M(NewObject<ULocalStateManifest>());
    M->Generation=FGuid::NewGuid();M->ProfileSlot=Profile;M->SavedUtc=1700000000;
    const FString Base=TEXT("State_")+FMD5::HashAnsiString(*Profile)+TEXT("_")+M->Generation.ToString(EGuidFormats::Digits)+TEXT("_");
    M->Slots={Base+TEXT("pilot"),Base+TEXT("economy"),Base+TEXT("skills"),Base+TEXT("universe")};
    TStrongObjectPtr<ULocalPilotState> Pilot(NewObject<ULocalPilotState>());
    TStrongObjectPtr<ULocalEconomyState> Economy(NewObject<ULocalEconomyState>());
    TStrongObjectPtr<ULocalSkillState> Skills(NewObject<ULocalSkillState>());
    TStrongObjectPtr<ULocalUniverseState> Universe(NewObject<ULocalUniverseState>());
    Pilot->Pilot.Callsign=TEXT("Recovery Cadet");Pilot->Pilot.PlayerPoints=123;Pilot->Pilot.Campaign=1;
    const TArray<ULocalStateArea*> Areas={Pilot.Get(),Economy.Get(),Skills.Get(),Universe.Get()};
    for(int32 I=0;I<4;++I){Areas[I]->Generation=M->Generation;Areas[I]->ProfileSlot=Profile;TestTrue(TEXT("Write checkpoint area"),Write(Areas[I],M->Slots[I]));}
    TestTrue(TEXT("Write manifest"),Write(M.Get(),Manifest));
    TestTrue(TEXT("Write previous manifest"),Write(M.Get(),Manifest+TEXT("_previous")));
    TArray<uint8> Zeros;Zeros.AddZeroed(64);
    for(const FString Slot:{Account,Profile,Account+TEXT("_Profiles")})FFileHelper::SaveArrayToFile(Zeros,*(Dir/(Slot+TEXT(".sav"))));
    FJsonObject List;List.SetStringField(TEXT("action"),TEXT("list"));
    auto Found=PilotSaveRecovery::Execute(List,Dir);
    TestEqual(TEXT("Both generations listed"),Found->GetArrayField(TEXT("checkpoints")).Num(),2);
    FJsonObject Restore;Restore.SetStringField(TEXT("action"),TEXT("restore"));Restore.SetStringField(TEXT("slot"),Manifest+TEXT("_previous"));Restore.SetStringField(TEXT("profile"),Profile);
    Restore.SetStringField(TEXT("savedUtc"),TEXT("wrong date"));
    TestFalse(TEXT("Stale selection rejected"),PilotSaveRecovery::Execute(Restore,Dir)->GetBoolField(TEXT("success")));
    TestTrue(TEXT("Rejected restore leaves damaged original untouched"),ReadBytes(Dir/(Account+TEXT(".sav")))==Zeros);
    Restore.SetStringField(TEXT("savedUtc"),FDateTime::FromUnixTimestamp(M->SavedUtc).ToIso8601());
    Restore.SetStringField(TEXT("profile"),TEXT("another account"));
    TestFalse(TEXT("Different account rejected"),PilotSaveRecovery::Execute(Restore,Dir)->GetBoolField(TEXT("success")));
    Restore.SetStringField(TEXT("profile"),Profile);
    // Newer live manifest must be replaced by the selected previous generation.
    M->SavedUtc+=300;Write(M.Get(),Manifest);
    auto Done=PilotSaveRecovery::Execute(Restore,Dir);
    if(!TestTrue(TEXT("Recovery succeeds"),Done->GetBoolField(TEXT("success"))))return false;
    const FString Backup=Done->GetStringField(TEXT("backup"));
    TestTrue(TEXT("Original primary backed up exactly"),ReadBytes(Backup/(Account+TEXT(".sav")))==Zeros);
    TestTrue(TEXT("Original damaged index backed up exactly"),ReadBytes(Backup/(Account+TEXT("_Profiles.sav")))==Zeros);
    TStrongObjectPtr<UPlayerSaveGame> Reload(Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(ReadBytes(Dir/(Account+TEXT(".sav"))))));
    if(!TestNotNull(TEXT("Restored native save loads"),Reload.Get()))return false;
    TestEqual(TEXT("Progress restored"),Reload->PlayerInfo.PlayerPoints,123);
    TestTrue(TEXT("Pilot and account copies match"),ReadBytes(Dir/(Account+TEXT(".sav")))==ReadBytes(Dir/(Profile+TEXT(".sav"))));
    TestTrue(TEXT("Selected older generation is active"),ReadBytes(Dir/(Manifest+TEXT(".sav")))==ReadBytes(Dir/(Manifest+TEXT("_previous.sav"))));
    FFileHelper::SaveArrayToFile(Zeros,*(Dir/(M->Slots[1]+TEXT(".sav"))));
    TestEqual(TEXT("Incomplete checkpoint hidden"),PilotSaveRecovery::Execute(List,Dir)->GetArrayField(TEXT("checkpoints")).Num(),0);
    TestFalse(TEXT("Corrupt economy cannot restore"),PilotSaveRecovery::Execute(Restore,Dir)->GetBoolField(TEXT("success")));
    Restore.SetStringField(TEXT("slot"),TEXT("../outside"));
    TestFalse(TEXT("Path traversal rejected"),PilotSaveRecovery::Execute(Restore,Dir)->GetBoolField(TEXT("success")));
    return true;
}
#endif
