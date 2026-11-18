#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "PlayerHullLoss.h"
#include "PlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerHullLossTest,"Starshatter.Player.HullLossPreservesCareerAndHome",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FPlayerHullLossTest::RunTest(const FString&)
{
    FS_PlayerGameInfo Pilot;
    TestEqual(TEXT("New pilot home system"),Pilot.HomeSystem,FName(TEXT("Janus")));
    TestEqual(TEXT("New pilot home sector"),Pilot.HomeRegion,FName(TEXT("Janek")));
    Pilot.Rank=5;Pilot.Empire=2;Pilot.PlayerKills=17;Pilot.MarkMissionCompleted(2,301);
    Pilot.Skills.Add(TEXT("FrigateCommand")).bComplete=true;
    auto& Lost=Pilot.HangarShips.AddDefaulted_GetRef();Lost.HullId=FGuid::NewGuid();Lost.Design=TEXT("Falcon");Lost.bCashPurchased=true;
    const FGuid LostId=Lost.HullId;
    auto& Other=Pilot.HangarShips.AddDefaulted_GetRef();Other.HullId=FGuid::NewGuid();Other.Design=TEXT("Prospector");
    const FGuid OtherId=Other.HullId;
    Pilot.AssignedHangarHull=LostId;Pilot.SelectedHangarHull=OtherId;
    const FName Hold(*(TEXT("Ship/")+LostId.ToString(EGuidFormats::Digits)));
    const FName OtherHold(*(TEXT("Ship/")+OtherId.ToString(EGuidFormats::Digits)));
    auto& Economy=Pilot.Economy;const FName Owner=FEconomyLedger::Pilot();
    Economy.Wallets.Add(Owner,75000);
    FEconomyLedger::CreateInventory(Economy,Hold,Owner,TEXT("Janus"),20);
    FEconomyLedger::CreateInventory(Economy,OtherHold,Owner,TEXT("Janus"),20);
    FEconomyLedger::CreateInventory(Economy,TEXT("Station Storage"),Owner,TEXT("Janus"),100);
    FEconomyLedger::Deposit(Economy,Hold,TEXT("Quartz"),3,1);
    FEconomyLedger::Deposit(Economy,OtherHold,TEXT("Quartz"),4,1);
    FEconomyLedger::Deposit(Economy,TEXT("Station Storage"),TEXT("Quartz"),10,1);
    const FGuid LostReservation=FEconomyLedger::Reserve(Economy,Owner,Hold,TEXT("Quartz"),1);
    const FGuid SafeReservation=FEconomyLedger::Reserve(Economy,Owner,OtherHold,TEXT("Quartz"),1);
    FMiningCrystalEquipment Crystal;Crystal.Crystal=TEXT("FerriteFocus");
    const FName LostOptics(*(Hold.ToString()+TEXT("/Optics/FerriteFocus")));
    const FName SafeOptics(*(OtherHold.ToString()+TEXT("/Optics/FerriteFocus")));
    Economy.MiningOptics.Add(LostOptics,Crystal);Economy.MiningOptics.Add(SafeOptics,Crystal);
    FMiningLease Claim;Claim.Claimant=Hold;Economy.MiningLeases.Add(TEXT("Lost Rock"),Claim);
    Claim.Claimant=OtherHold;Economy.MiningLeases.Add(TEXT("Safe Rock"),Claim);
    TestTrue(TEXT("Validated assigned hull loss finalizes"),FPlayerHullLoss::Finalize(Pilot,LostId));
    TestTrue(TEXT("Lost hull and policy are settled"),Pilot.HangarShips[0].bLost && Pilot.HangarShips[0].Insurance.bSettled);
    TestTrue(TEXT("Cash purchase provenance is retained"),Pilot.HangarShips[0].bCashPurchased);
    TestFalse(TEXT("Other owned hull is untouched"),Pilot.HangarShips[1].bLost);
    TestFalse(TEXT("Lost assignment is cleared"),Pilot.AssignedHangarHull.IsValid());
    TestEqual(TEXT("A different selected hull stays selected"),Pilot.SelectedHangarHull,OtherId);
    TestEqual(TEXT("Lost hull cargo is removed"),FEconomyLedger::Used(Economy.Inventories.FindChecked(Hold)),0.0);
    TestFalse(TEXT("Dying runtime cannot refill the destroyed hold"),FEconomyLedger::Deposit(Economy,Hold,TEXT("Quartz"),1,1));
    TestEqual(TEXT("Other hull cargo survives"),FEconomyLedger::Quantity(Economy,OtherHold,TEXT("Quartz")),4.0);
    TestEqual(TEXT("Station cargo survives"),FEconomyLedger::Quantity(Economy,TEXT("Station Storage"),TEXT("Quartz")),10.0);
    TestFalse(TEXT("Destroyed hold reservation released"),Economy.Reservations.Contains(LostReservation));
    TestTrue(TEXT("Other hold reservation preserved"),Economy.Reservations.Contains(SafeReservation));
    TestFalse(TEXT("Destroyed optics removed"),Economy.MiningOptics.Contains(LostOptics));
    TestTrue(TEXT("Other hull optics preserved"),Economy.MiningOptics.Contains(SafeOptics));
    TestFalse(TEXT("Destroyed miner releases asteroid claim"),Economy.MiningLeases.Contains(TEXT("Lost Rock")));
    TestTrue(TEXT("Other miner claim remains"),Economy.MiningLeases.Contains(TEXT("Safe Rock")));
    TestEqual(TEXT("Loss cleanup does not reset or pay the wallet"),Economy.Wallets.FindRef(Owner),75000LL);
    TestEqual(TEXT("Rank survives destruction"),Pilot.Rank,5);
    TestEqual(TEXT("Faction survives destruction"),Pilot.Empire,2);
    TestEqual(TEXT("Combat record survives destruction"),Pilot.PlayerKills,17);
    TestTrue(TEXT("Learned skills survive destruction"),Pilot.Skills.FindChecked(TEXT("FrigateCommand")).bComplete);
    TestTrue(TEXT("Completed mission progression survives destruction"),Pilot.HasCompletedMission(2,301));
    TestFalse(TEXT("Repeated finalization cannot settle twice"),FPlayerHullLoss::Finalize(Pilot,LostId));
    auto* Save=NewObject<UPlayerSaveGame>();Save->PlayerInfo=Pilot;
    Save->PlayerInfo.HomeSystem=TEXT("Solus");Save->PlayerInfo.HomeRegion=TEXT("Meridian");
    TArray<uint8> Bytes;
    if(!TestTrue(TEXT("Home and loss state serialize"),UGameplayStatics::SaveGameToMemory(Save,Bytes)))return false;
    const auto* Loaded=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if(!TestNotNull(TEXT("Home and loss state restore"),Loaded))return false;
    TestEqual(TEXT("Saved home system is preserved"),Loaded->PlayerInfo.HomeSystem,FName(TEXT("Solus")));
    TestEqual(TEXT("Saved home sector is preserved"),Loaded->PlayerInfo.HomeRegion,FName(TEXT("Meridian")));
    TestTrue(TEXT("Lost hull cannot resurrect after reload"),Loaded->PlayerInfo.HangarShips[0].bLost);
    return true;
}
#endif
