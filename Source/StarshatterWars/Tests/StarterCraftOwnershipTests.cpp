#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "StarterCraftOwnership.h"
#include "PlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStarterCraftOwnershipTest,"Starshatter.Player.StarterFalconOwnership",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStarterCraftOwnershipTest::RunTest(const FString&)
{
 FS_PlayerGameInfo Pilot;Pilot.Name=TEXT("Starter fixture");Pilot.Economy.Wallets.Add(FEconomyLedger::Pilot(),5000);
 TestTrue(TEXT("New pilot gets an owned Falcon"),StarterCraftOwnership::Normalize(Pilot));
 TestEqual(TEXT("Exactly one starter"),Pilot.HangarShips.Num(),1);
 const FGuid Starter=Pilot.HangarShips[0].HullId;
 TestFalse(TEXT("Starter is owned, not loaned"),Pilot.HangarShips[0].bAssigned);
 TestEqual(TEXT("Starter display name is Falcon"),Pilot.HangarShips[0].Name,FString(TEXT("Falcon")));
 TestFalse(TEXT("Included starter is not a cash purchase with lifetime insurance"),Pilot.HangarShips[0].bCashPurchased);
 TestEqual(TEXT("Starting credits preserved"),Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),5000LL);
 TestFalse(TEXT("Repeated startup cannot grant another Falcon"),StarterCraftOwnership::Normalize(Pilot));
 FS_PlayerGameInfo Existing;Existing.Name=TEXT("Existing fixture");
 FPlayerHangarShip Loan;Loan.HullId=FGuid::NewGuid();Loan.Design=TEXT("Falcon");Loan.bAssigned=true;Loan.Name=TEXT("Delta 1");Loan.Location=TEXT("Carrier / Archon");Loan.LoadoutStations={1,2,3};Loan.HullCondition=.75f;
 Existing.HangarShips.Add(Loan);Existing.AssignedHangarHull=Loan.HullId;Existing.SelectedHangarHull=Loan.HullId;
 for(int I=0;I<4;++I){auto Spare=Loan;Spare.HullId=FGuid::NewGuid();Existing.HangarShips.Add(Spare);}
 FPlayerHangarShip Stormhawk;Stormhawk.HullId=FGuid::NewGuid();Stormhawk.Design=TEXT("Stormhawk");Stormhawk.Location=TEXT("Station / Janus / Janek Station");Existing.HangarShips.Add(Stormhawk);
 TestTrue(TEXT("Old account migrates duplicate trainers"),StarterCraftOwnership::Normalize(Existing));
 TestEqual(TEXT("Only starter Falcon and purchased Stormhawk remain"),Existing.HangarShips.Num(),2);
 TestEqual(TEXT("Active starter retains cargo/hull identity"),Existing.HangarShips[0].HullId,Loan.HullId);
 TestEqual(TEXT("Starter retains parked location"),Existing.HangarShips[0].Location,Loan.Location);
 TestEqual(TEXT("Old mission callsign becomes Falcon in owned hangar"),Existing.HangarShips[0].Name,FString(TEXT("Falcon")));
 TestTrue(TEXT("Starter retains loadout"),Existing.HangarShips[0].LoadoutStations==Loan.LoadoutStations);
 TestEqual(TEXT("Starter retains damage"),Existing.HangarShips[0].HullCondition,.75f);
 TestEqual(TEXT("Purchased Stormhawk remains at its own base"),Existing.HangarShips[1].Location,Stormhawk.Location);
 TestFalse(TEXT("Migration is idempotent"),StarterCraftOwnership::Normalize(Existing));
 FPlayerHangarShip Purchased=Existing.HangarShips[0];Purchased.HullId=FGuid::NewGuid();Existing.HangarShips.Add(Purchased);
 StarterCraftOwnership::Normalize(Existing);
 TestEqual(TEXT("Additional purchased Falcons are not deleted"),Existing.HangarShips.Num(),3);
 auto* Save=NewObject<UPlayerSaveGame>();Save->PlayerInfo=Existing;TArray<uint8> Bytes;
 TestTrue(TEXT("Owned starter serializes"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
 auto* Reload=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
 if(!TestNotNull(TEXT("Owned starter reloads"),Reload))return false;
 TestFalse(TEXT("Reload does not grant again"),StarterCraftOwnership::Normalize(Reload->PlayerInfo));
 Pilot.HangarShips[0].bLost=true;Pilot.AssignedHangarHull.Invalidate();Pilot.SelectedHangarHull.Invalidate();
 TestFalse(TEXT("Lost starter does not receive a free replacement on login"),StarterCraftOwnership::Normalize(Pilot));
 TestEqual(TEXT("Loss history remains"),Pilot.HangarShips.Num(),1);
 TestFalse(TEXT("Lost fighter is not visible in hangar"),StarterCraftOwnership::VisibleInHangar(Pilot.HangarShips[0],Starter));
 TestFalse(TEXT("Inactive loaner is not visible in hangar"),StarterCraftOwnership::VisibleInHangar(Loan,FGuid()));
 TestTrue(TEXT("Current mission loaner remains visible"),StarterCraftOwnership::VisibleInHangar(Loan,Loan.HullId));
 return true;
}
#endif
