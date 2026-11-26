#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "ShipCommerceAI.h"
#include "ShipDesignRegistry.h"
#include "ShipyardAI.h"
#include "SerializedHullStockAI.h"
#include "StarterCraftOwnership.h"
#include "MissionHullAccessRules.h"
#include "ShipInsuranceSubsystem.h"
#include "EconomyRuntime.h"
#include "PlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
struct FCommerceFixture
{
    const TMap<FName,FShipDesign> Old=ShipDesignRegistry::GetAll();
    FShipDesign Design;
    FS_PlayerGameInfo Pilot;
    FShipCommerceDock Dock;
    FShipCommerceRules Rules;
    FText Why;
    FCommerceFixture()
    {
        Design.ShipName=TEXT("CommerceTestFighter");Design.DisplayName=TEXT("Commerce Test Fighter");Design.Category=EShipCategory::Fighter;
        Design.ShipType=int32(CLASSIFICATION::FIGHTER);Design.ShipEmpire=EShipEmpire::All;Design.BaseCost=10000;Design.Mass=10;Design.Integrity=100;
        auto& Loadout=Design.Loadout.AddDefaulted_GetRef();Loadout.Name=TEXT("STANDARD");Loadout.Stations={1,2,-1};
        ShipDesignRegistry::RegisterDesign(FName(*Design.ShipName),Design);
        Pilot.Name=TEXT("Test pilot");Pilot.Rank=9;
        Pilot.Economy.Wallets.Add(FEconomyLedger::Pilot(),1000000);
        Dock.bAuthorized=true;Dock.Location=TEXT("Station / Janus / Test Yard");Dock.Inventory=TEXT("Test/Dealer");Dock.Dealer=TEXT("NPC/Test Yard");Dock.System=TEXT("Janus");
        Pilot.Economy.Wallets.Add(Dock.Dealer,1000000);
        auto* Store=FEconomyLedger::CreateInventory(Pilot.Economy,Dock.Inventory,FEconomyRuntime::IndustryOwner(),Dock.System,1000);Store->bHullStorage=true;Store->TradingOwner=Dock.Dealer;
    }
    ~FCommerceFixture(){ShipDesignRegistry::Clear();for(const auto& Pair:Old)ShipDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);}
    FPlayerHangarShip Owned()
    {FPlayerHangarShip H;H.HullId=FGuid::NewGuid();H.Design=FName(*Design.ShipName);H.Name=Design.DisplayName;H.Location=Dock.Location;H.HullValue=10000;return H;}
    FGuid Stock()
    {
        const FName Item=DataTableUtils::MakeValidName(Design.ShipName.ToLower()+TEXT(" assembled hull"));
        FEconomyLedger::Deposit(Pilot.Economy,Dock.Inventory,Item,1,1);SerializedHullStockAI::Ensure(Pilot.Economy,Dock.Inventory,FName(*Design.ShipName),1);
        return SerializedHullStockAI::Available(Pilot.Economy,Dock.Inventory,FName(*Design.ShipName),1)[0];
    }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShipSaleProtectionTest,"Starshatter.ShipCommerce.LastFighterAndAuthority",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShipSaleProtectionTest::RunTest(const FString&)
{
    FCommerceFixture F;const auto First=F.Owned();F.Pilot.HangarShips.Add(First);
    TestFalse(TEXT("Last owned fighter is protected"),ShipCommerceAI::CanSell(F.Pilot,First.HullId,F.Dock,F.Rules,F.Why));
    auto Other=F.Owned();Other.bAssigned=true;F.Pilot.HangarShips.Add(Other);
    TestFalse(TEXT("Mission loaner cannot satisfy last fighter protection"),ShipCommerceAI::CanSell(F.Pilot,First.HullId,F.Dock,F.Rules,F.Why));
    F.Pilot.HangarShips.Last().bAssigned=false;F.Pilot.HangarShips.Last().RentalId=FGuid::NewGuid();
    TestFalse(TEXT("Rental does not count as owned spare"),ShipCommerceAI::CanSell(F.Pilot,First.HullId,F.Dock,F.Rules,F.Why));
    F.Pilot.HangarShips.Last().RentalId.Invalidate();
    auto& Spare=F.Pilot.HangarShips.Last();Spare.bHasCondition=true;auto& Reactor=Spare.SystemConditions.AddDefaulted_GetRef();Reactor.SystemName=TEXT("Fusion Reactor");Reactor.Availability=0;
    TestFalse(TEXT("Disabled power on the only spare cannot bypass protection"),ShipCommerceAI::CanSell(F.Pilot,First.HullId,F.Dock,F.Rules,F.Why));
    Spare.SystemConditions.Reset();
    TestTrue(TEXT("Second owned fighter permits sale"),ShipCommerceAI::CanSell(F.Pilot,First.HullId,F.Dock,F.Rules,F.Why));
    F.Pilot.AssignedHangarHull=First.HullId;
    TestFalse(TEXT("Active craft cannot be sold"),ShipCommerceAI::CanSell(F.Pilot,First.HullId,F.Dock,F.Rules,F.Why));F.Pilot.AssignedHangarHull.Invalidate();
    auto Remote=F.Dock;Remote.Location=TEXT("Station / Other");TestFalse(TEXT("Remote hull cannot teleport into sale"),ShipCommerceAI::CanSell(F.Pilot,First.HullId,Remote,F.Rules,F.Why));
    Remote=F.Dock;Remote.bAuthorized=false;TestFalse(TEXT("UI client without authority cannot sell"),ShipCommerceAI::CanSell(F.Pilot,First.HullId,Remote,F.Rules,F.Why));
    const FName Hold(*(TEXT("Ship/")+First.HullId.ToString(EGuidFormats::Digits)));FEconomyLedger::CreateInventory(F.Pilot.Economy,Hold,FEconomyLedger::Pilot(),F.Dock.System,100);FEconomyLedger::Deposit(F.Pilot.Economy,Hold,TEXT("ore"),1,1);
    TestFalse(TEXT("Cargo must be unloaded before sale"),ShipCommerceAI::CanSell(F.Pilot,First.HullId,F.Dock,F.Rules,F.Why));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShipSaleAtomicTest,"Starshatter.ShipCommerce.SaleRollbackAndUsedRepurchase",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShipSaleAtomicTest::RunTest(const FString&)
{
    FCommerceFixture F;auto H=F.Owned();H.bHasCondition=true;H.HullCondition=.7f;auto& Part=H.Components.AddDefaulted_GetRef();Part.ItemId=FGuid::NewGuid();Part.Level=4;Part.InstalledValue=2000;Part.bHasCondition=true;Part.ConditionPercent=50;H.SavedWeaponAmmo.Add(0,2);
    F.Pilot.HangarShips={H,F.Owned()};const FGuid Tx=FGuid::NewGuid();const int64 Before=F.Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot());
    TestFalse(TEXT("Failed persistent sale rolls back"),ShipCommerceAI::Sell(F.Pilot,H.HullId,F.Dock,F.Rules,Tx,100,[]{return false;},F.Why));
    TestFalse(TEXT("Hull retained after save failure"),F.Pilot.HangarShips[0].bSold);TestEqual(TEXT("Wallet retained"),F.Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),Before);
    TestFalse(TEXT("No duplicate stock after rollback"),F.Pilot.Economy.UsedHullStock.Contains(H.HullId));
    TestTrue(TEXT("Sale succeeds"),ShipCommerceAI::Sell(F.Pilot,H.HullId,F.Dock,F.Rules,Tx,100,[]{return true;},F.Why));
    const int64 Paid=CommerceCharges::Quote(ShipCommerceAI::SalePrice(H,F.Rules),ECommercePurpose::MarketSale).Base;
    TestEqual(TEXT("Seller receives net after included charges"),F.Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),Before+Paid);
    TestTrue(TEXT("Replay acknowledges same sale"),ShipCommerceAI::Sell(F.Pilot,H.HullId,F.Dock,F.Rules,Tx,100,[]{return false;},F.Why));
    TestEqual(TEXT("Replay cannot pay twice"),F.Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),Before+Paid);
    TestFalse(TEXT("Sold hull hidden"),StarterCraftOwnership::VisibleInHangar(F.Pilot.HangarShips[0],{}));
    TestNull(TEXT("Sold hull cannot satisfy mission ownership"),MissionHullAccessRules::FindAvailable(F.Pilot,H.Design)==&F.Pilot.HangarShips[0]?&F.Pilot.HangarShips[0]:nullptr);
    TestTrue(TEXT("Same physical used hull can be purchased"),ShipyardAI::Purchase(F.Pilot,F.Design,F.Dock.Inventory,F.Dock.Location,FGuid::NewGuid(),F.Why,H.HullId));
    TestEqual(TEXT("Repurchase replaces title tombstone, no duplicate row"),F.Pilot.HangarShips.Num(),2);
    const auto& Bought=F.Pilot.HangarShips[0];TestFalse(TEXT("Repurchased hull is available"),Bought.bSold);TestEqual(TEXT("Used condition preserved"),Bought.HullCondition,.7f);TestEqual(TEXT("Installed part identity retained"),Bought.Components[0].ItemId,Part.ItemId);TestEqual(TEXT("Resale does not refill missiles"),Bought.SavedWeaponAmmo.FindRef(0),2);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShipRentalLifecycleTest,"Starshatter.ShipCommerce.RentalExpiryReturnAndPersistence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShipRentalLifecycleTest::RunTest(const FString&)
{
    FCommerceFixture F;const FGuid Id=F.Stock(),Tx=FGuid::NewGuid();const int64 Now=1000;
    TestTrue(TEXT("Specific hull rented"),ShipCommerceAI::Rent(F.Pilot,F.Design,Id,F.Dock,F.Rules,3,Now,Tx,F.Why));
    TestEqual(TEXT("One rental agreement"),F.Pilot.ShipRentals.Num(),1);const auto C=F.Pilot.ShipRentals[0];
    TestEqual(TEXT("Lease uses three real days"),C.DueUtc,Now+3LL*86400);TestTrue(TEXT("Mandatory existing Basic premium paid"),C.Premium>0);
    const auto H=F.Pilot.HangarShips[0];TestTrue(TEXT("Lease carries temporary custody marker"),H.RentalId.IsValid());
    TestEqual(TEXT("New rental receives its design's default loadout"),H.LoadoutName,FString(TEXT("STANDARD")));
    TestEqual(TEXT("Rental preserves the default empty weapon station"),H.LoadoutStations[2],-1);
    TestNull(TEXT("Rental cannot satisfy campaign ownership"),MissionHullAccessRules::FindAvailable(F.Pilot,H.Design));
    TestTrue(TEXT("Usable before deadline"),ShipCommerceAI::RentalMayLaunch(F.Pilot,H,C.DueUtc-1));TestFalse(TEXT("Cannot newly launch at expiry"),ShipCommerceAI::RentalMayLaunch(F.Pilot,H,C.DueUtc));
    TestFalse(TEXT("Rental cannot be sold"),ShipCommerceAI::CanSell(F.Pilot,Id,F.Dock,F.Rules,F.Why));
    const int64 Balance=F.Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot());
    TestTrue(TEXT("Rental retry does not charge twice"),ShipCommerceAI::Rent(F.Pilot,F.Design,Id,F.Dock,F.Rules,3,Now,Tx,F.Why));TestEqual(TEXT("Rental retry preserves balance"),F.Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),Balance);
    TStrongObjectPtr<UPlayerSaveGame> Save(NewObject<UPlayerSaveGame>());Save->PlayerInfo=F.Pilot;TArray<uint8> Bytes;TestTrue(TEXT("Rental save serializes"),UGameplayStatics::SaveGameToMemory(Save.Get(),Bytes));
    TStrongObjectPtr<UPlayerSaveGame> Loaded(Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes)));if(!TestNotNull(TEXT("Rental reloads"),Loaded.Get()))return false;
    F.Pilot=Loaded->PlayerInfo;TestEqual(TEXT("Rental legal owner survives reload"),F.Pilot.ShipRentals[0].Lessor,C.Lessor);TestEqual(TEXT("UTC deadline survives logout"),F.Pilot.ShipRentals[0].DueUtc,C.DueUtc);
    F.Pilot.HangarShips[0].bHasCondition=true;F.Pilot.HangarShips[0].HullCondition=.6f;
    TestFalse(TEXT("Failed return save rolls back"),ShipCommerceAI::Return(F.Pilot,Id,F.Dock,FGuid::NewGuid(),C.DueUtc+1,[]{return false;},F.Why));TestFalse(TEXT("Failed return retains custody"),F.Pilot.HangarShips[0].bSold);
    TestTrue(TEXT("Overdue rental may be returned"),ShipCommerceAI::Return(F.Pilot,Id,F.Dock,FGuid::NewGuid(),C.DueUtc+1,[]{return true;},F.Why));
    TestEqual(TEXT("Returned stock has damage, no free repair"),F.Pilot.Economy.UsedHullStock.FindChecked(Id).HullCondition,.6f);
    TestEqual(TEXT("Returned hull keeps its serial ID"),SerializedHullStockAI::Available(F.Pilot.Economy,F.Dock.Inventory,H.Design,1)[0],Id);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRentalLiabilityTest,"Starshatter.ShipCommerce.RentalLossLiabilityAndDebt",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRentalLiabilityTest::RunTest(const FString&)
{
    FCommerceFixture F;const FGuid Id=F.Stock();TestTrue(TEXT("Rent loss-test hull"),ShipCommerceAI::Rent(F.Pilot,F.Design,Id,F.Dock,F.Rules,1,1000,FGuid::NewGuid(),F.Why));
    F.Pilot.AssignedHangarHull=Id;F.Pilot.Economy.Wallets[FEconomyLedger::Pilot()]=100;const auto C=F.Pilot.ShipRentals[0];
    TestFalse(TEXT("Failed loss save rolls back fund and loss"),ShipCommerceAI::SettleRentalLoss(F.Pilot,Id,1001,[]{return false;},F.Why));TestFalse(TEXT("Failed loss leaves hull unresolved"),F.Pilot.HangarShips[0].bLost);
    TestTrue(TEXT("Loss settlement succeeds"),ShipCommerceAI::SettleRentalLoss(F.Pilot,Id,1001,[]{return true;},F.Why));
    TestEqual(TEXT("Insurance pays lessor, pilot receives no windfall"),F.Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),0LL);
    TestEqual(TEXT("Liability is hull minus insurance, less actual payment"),F.Pilot.ShipRentals[0].UnpaidLiability,FMath::Max(0LL,C.HullValue-C.Payout)-100);
    TestTrue(TEXT("Lost rental finalized"),F.Pilot.HangarShips[0].bLost);TestFalse(TEXT("Loss cannot pay twice"),ShipCommerceAI::SettleRentalLoss(F.Pilot,Id,1001,[]{return true;},F.Why));
    F.Pilot.Economy.Wallets[FEconomyLedger::Pilot()]=10000;
    TestTrue(TEXT("Outstanding debt can be repaid"),ShipCommerceAI::PayRentalDebt(F.Pilot,C.Id,FGuid::NewGuid(),[]{return true;},F.Why));TestEqual(TEXT("Debt fully cleared"),F.Pilot.ShipRentals[0].UnpaidLiability,0LL);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommercePolicyTest,"Starshatter.ShipCommerce.AuthoredPolicyAndStarterHistory",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCommercePolicyTest::RunTest(const FString&)
{
    FShipCommerceRules Rules;FString Error;TestTrue(TEXT("Authored JSON policy loads"),FShipCommerceRules::Load(Rules,Error));TestEqual(TEXT("Unsupported rental duration rejected"),ShipCommerceAI::RentalFee(10000,2,Rules),0LL);
    FS_PlayerGameInfo P;P.Name=TEXT("Test");StarterCraftOwnership::Normalize(P);P.HangarShips[0].bSold=true;P.AssignedHangarHull.Invalidate();P.SelectedHangarHull.Invalidate();StarterCraftOwnership::Normalize(P);
    TestEqual(TEXT("Sold starter is proof of issue, no free replacement"),P.HangarShips.Num(),1);TestFalse(TEXT("Sold starter cannot become selected"),P.SelectedHangarHull.IsValid());
    return true;
}
#endif
