#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerSaveGame.h"
#include "LocalGameStateSubsystem.h"

// Explicit maintenance operation: writes only an isolated verification slot.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccountCheckpointRecoveryTest,"SSW.Maintenance.VerifyAccountCheckpoint",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAccountCheckpointRecoveryTest::RunTest(const FString&)
{
    FString IndexSlot, Account;
    if (!FParse::Value(FCommandLine::Get(),TEXT("RecoveryIndex="),IndexSlot) ||
        !FParse::Value(FCommandLine::Get(),TEXT("RecoveryAccount="),Account))
    { AddInfo(TEXT("Skipped: explicit recovery account and index required.")); return true; }
    auto* Index=Cast<UPlayerSaveIndexGame>(UGameplayStatics::LoadGameFromSlot(IndexSlot,0));
    if(!TestNotNull(TEXT("Preserved profile index"),Index))return false;
    const FString Profile=Index->ActiveSlot;
    if(!TestTrue(TEXT("Index belongs to requested account"),Profile.StartsWith(Account+TEXT("_Profile_"))))return false;
    FS_PlayerGameInfo Player;
    Player.LocalStateRevision=0;
    ULocalUniverseState* World=nullptr;
    if(!TestTrue(TEXT("Complete checkpoint validates and merges"),ULocalGameStateSubsystem::ReadCheckpoint(Player,Profile,0,World)))return false;
    auto* Save=NewObject<UPlayerSaveGame>();
    Save->SaveVersion=2; Save->PlayerInfo=Player;
    const FString Output=TEXT("SSW_RecoveredAccountVerified");
    if(!TestTrue(TEXT("Recovered pilot serialized"),UGameplayStatics::SaveGameToSlot(Save,Output,0)))return false;
    auto* Reload=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromSlot(Output,0));
    if(!TestNotNull(TEXT("Recovered pilot reloads"),Reload))return false;
    TestEqual(TEXT("Points survive round trip"),Reload->PlayerInfo.PlayerPoints,Player.PlayerPoints);
    TestEqual(TEXT("Campaign survives round trip"),Reload->PlayerInfo.Campaign,Player.Campaign);
    TestEqual(TEXT("Skills survive round trip"),Reload->PlayerInfo.Skills.Num(),Player.Skills.Num());
    AddInfo(FString::Printf(TEXT("Verified profile=%s points=%d campaign=%d skills=%d"),*Profile,Player.PlayerPoints,Player.Campaign,Player.Skills.Num()));
    return true;
}
#endif
