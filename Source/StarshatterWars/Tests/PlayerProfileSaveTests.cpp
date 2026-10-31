#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "StarshatterPlayerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerProfileSaveTest,"Starshatter.Player.NamedSaveLoadDelete",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPlayerProfileSaveTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UGameInstance> Owner(NewObject<UGameInstance>());
    TStrongObjectPtr<UStarshatterPlayerSubsystem> Players(NewObject<UStarshatterPlayerSubsystem>(Owner.Get()));
    const FString Base=TEXT("Automation/PlayerProfiles/")+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT("/PlayerSave");
    Players->SetSaveSlot(Base,0);Players->LoadPlayer();Players->ResetToDefaults();
    TestTrue(TEXT("New pilot creation date initialized"),Players->GetPlayerInfo().CreateTime>0);
    TArray<FString> Files={Base,Players->GetPlayerIndexSlot()};
    ON_SCOPE_EXIT {for(const FString& File:Files) UGameplayStatics::DeleteGameInSlot(File,0);};
    auto& Alpha=Players->GetMutablePlayerInfo();Alpha.Name=TEXT("Alpha");Alpha.Callsign=TEXT("One");Alpha.Rank=3;Alpha.MedalsMask=5;Alpha.ChatMacros={TEXT("Alpha macro")};
    TestTrue(TEXT("Existing single-player save written to isolated fixture"),Players->SavePlayer(true));
    const auto AlphaData=Players->GetPlayerInfo();
    auto Saves=Players->GetSavedPlayers();
    if(!TestEqual(TEXT("Existing save imported once"),Saves.Num(),1)) return false;
    const FString AlphaSlot=Saves[0].Slot;Files.Add(AlphaSlot);
    TestEqual(TEXT("Imported pilot label"),Saves[0].DisplayName,FString(TEXT("Alpha")));
    TestEqual(TEXT("Reopening list does not duplicate legacy import"),Players->GetSavedPlayers().Num(),1);
    TestTrue(TEXT("Original authoritative save retained"),UGameplayStatics::DoesSaveGameExist(Base,0));

    auto& Beta=Players->GetMutablePlayerInfo();Beta.Name=TEXT("Beta");Beta.Callsign=TEXT("Two");Beta.Rank=7;Beta.MedalsMask=10;Beta.ChatMacros={TEXT("Beta macro")};
    TestTrue(TEXT("Save second named pilot"),Players->SavePlayerAs(TEXT("Beta")));
    const auto BetaData=Players->GetPlayerInfo();Saves=Players->GetSavedPlayers();
    TestEqual(TEXT("Two pilots are listed"),Saves.Num(),2);
    const FString BetaSlot=Players->GetActiveSavedPlayerSlot();Files.Add(BetaSlot);
    TestTrue(TEXT("Named pilot is a real Unreal SaveGame"),Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromSlot(BetaSlot,0))!=nullptr);
    TestFalse(TEXT("Empty pilot name rejected"),Players->SavePlayerAs(TEXT(" ")));
    TestFalse(TEXT("Duplicate new pilot name rejected"),Players->SavePlayerAs(TEXT("alpha")));
    TestFalse(TEXT("Cannot load unrelated save slot"),Players->LoadSavedPlayer(TEXT("options")));
    TestFalse(TEXT("Cannot delete unrelated save slot"),Players->DeleteSavedPlayer(TEXT("options")));
    TestTrue(TEXT("Rejected operations leave current pilot unchanged"),FS_PlayerGameInfo::StaticStruct()->CompareScriptStruct(&BetaData,&Players->GetPlayerInfo(),0));

    TestTrue(TEXT("Load Alpha"),Players->LoadSavedPlayer(AlphaSlot));
    TestTrue(TEXT("Complete profile, progress, awards and macros restored"),FS_PlayerGameInfo::StaticStruct()->CompareScriptStruct(&AlphaData,&Players->GetPlayerInfo(),0));
    Players->GetMutablePlayerInfo().Callsign=TEXT("Updated Alpha");
    TestTrue(TEXT("Overwrite selected pilot"),Players->SavePlayerAs(TEXT("Alpha"),AlphaSlot));
    TestEqual(TEXT("Overwrite does not create duplicate entry"),Players->GetSavedPlayers().Num(),2);
    TestTrue(TEXT("Switch back to independent Beta save"),Players->LoadSavedPlayer(BetaSlot));
    TestTrue(TEXT("Other pilot was not overwritten"),FS_PlayerGameInfo::StaticStruct()->CompareScriptStruct(&BetaData,&Players->GetPlayerInfo(),0));

    auto* Future=NewObject<UPlayerSaveGame>();Future->SaveVersion=999;Future->PlayerInfo.Name=TEXT("Future");
    TestTrue(TEXT("Future-version fixture saved"),UGameplayStatics::SaveGameToSlot(Future,AlphaSlot,0));
    TestFalse(TEXT("Unsupported future save refused"),Players->LoadSavedPlayer(AlphaSlot));
    TestEqual(TEXT("Failed load preserves current pilot"),Players->GetPlayerInfo().Name,FString(TEXT("Beta")));
    TestTrue(TEXT("Delete selected named save"),Players->DeleteSavedPlayer(AlphaSlot));
    TestFalse(TEXT("Deleted save file no longer exists"),UGameplayStatics::DoesSaveGameExist(AlphaSlot,0));
    TestTrue(TEXT("Other pilot save survives deletion"),UGameplayStatics::DoesSaveGameExist(BetaSlot,0));
    TestEqual(TEXT("Deleted pilot removed from list"),Players->GetSavedPlayers().Num(),1);
    TestTrue(TEXT("Delete remaining saved copy"),Players->DeleteSavedPlayer(BetaSlot));
    TestEqual(TEXT("Empty saved list remains empty"),Players->GetSavedPlayers().Num(),0);
    TestEqual(TEXT("Deleting saved copy does not erase loaded pilot"),Players->GetPlayerInfo().Name,FString(TEXT("Beta")));
    return true;
}
#endif
