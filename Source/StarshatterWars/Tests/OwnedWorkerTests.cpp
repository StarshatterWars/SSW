#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "NativeEconomyWorker.h"
#include "EconomyRuntime.h"
#include "ShipStoreAI.h"
#include "SerializedHullStockAI.h"
#include "ShipInsuranceSubsystem.h"
#include "HAL/PlatformProcess.h"
#include "HAL/Event.h"
#include "Misc/ScopeExit.h"
namespace
{
struct FOwnedWorkerFixture
{
    FS_PlayerGameInfo Pilot;
    FShipCommerceDock Dock;
    FShipCommerceRules Rules;
    FGuid Target=FGuid::NewGuid(),Spare=FGuid::NewGuid();
    FName Design=TEXT("Worker fighter");
    TUniquePtr<FNativeEconomyWorker> Worker=MakeUnique<FNativeEconomyWorker>();
    FEvent* Done=FPlatformProcess::GetSynchEventFromPool(false);FText Why;
    FOwnedWorkerFixture()
    {
        Pilot.Rank=2;Pilot.Name=TEXT("Pilot");Pilot.AssignedHangarHull=Spare;Pilot.SelectedHangarHull=Target;
        Pilot.Economy.Wallets.Add(FEconomyLedger::Pilot(),1000000);Pilot.Economy.Wallets.Add(TEXT("Dealer"),1000000);
        Dock.bAuthorized=true;Dock.Location=TEXT("Carrier / Test");Dock.System=TEXT("Janus");Dock.Inventory=TEXT("Dealer stock");Dock.Dealer=TEXT("Dealer");
        auto* Store=FEconomyLedger::CreateInventory(Pilot.Economy,Dock.Inventory,FEconomyRuntime::IndustryOwner(),Dock.System,1000);Store->bHullStorage=true;Store->TradingOwner=Dock.Dealer;
        FPlayerHangarShip H;H.HullId=Target;H.Design=Design;H.Name=TEXT("First fighter");H.Location=Dock.Location;H.HullValue=1000;H.bEquipmentValueSeparated=true;
        Pilot.HangarShips.Add(H);H.HullId=Spare;H.Name=TEXT("Spare fighter");Pilot.HangarShips.Add(H);
    }
    ~FOwnedWorkerFixture(){Worker.Reset();FPlatformProcess::ReturnSynchEventToPool(Done);}
    FNativeOwnedCommand Command(EOwnedCommand Action)
    {
        FNativeOwnedCommand C;C.Action=Action;C.Hull=Target;C.Receipt=FGuid::NewGuid();C.Dock=Dock;C.Rules=Rules;C.NowUtc=1000;C.Categories.Add(Design,EShipCategory::Fighter);
        C.Source=FNativeOwnedState::Capture(Pilot);C.PreparedHull=Pilot.HangarShips[0];return C;
    }
    TUniquePtr<FNativeEconomyResult> Run(FNativeOwnedCommand C)
    {
        C.Source=FNativeOwnedState::Capture(Pilot);FNativeEconomyInput I;I.State=Pilot.Economy;I.Revision=1;I.Profile=TEXT("Owned/session1");I.Owned=MakeUnique<FNativeOwnedCommand>(MoveTemp(C));
        if(!Worker->Submit(MoveTemp(I),[this](){Done->Trigger();}) || !Done->Wait(10000))return nullptr;return Worker->TakeResult();
    }
    bool Apply(const FNativeOwnedCommand& C)
    {auto R=Run(C);return R && FNativeEconomyWorker::CommitOwned(MoveTemp(*R),Pilot,1,TEXT("Owned/session1"),[]{return true;},Why);}
    FGuid Stock()
    {
        FEconomyLedger::Deposit(Pilot.Economy,Dock.Inventory,DataTableUtils::MakeValidName(Design.ToString().ToLower()+TEXT(" assembled hull")),1,1);
        SerializedHullStockAI::Ensure(Pilot.Economy,Dock.Inventory,Design,1);
        return SerializedHullStockAI::Available(Pilot.Economy,Dock.Inventory,Design,1)[0];
    }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOwnedSaleWorkerTest,"Starshatter.Domains.Owned.SaleIsolationRollbackAndReadSets",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FOwnedSaleWorkerTest::RunTest(const FString&)
{
    FOwnedWorkerFixture F;auto C=F.Command(EOwnedCommand::Sell);C.Cost=700;auto R=F.Run(C);if(!TestNotNull(TEXT("Owned reply"),R.Get()))return false;
    TestTrue(TEXT("Commerce executes off game thread"),R->bOffGameThread);TestFalse(TEXT("No early hull transfer"),F.Pilot.HangarShips[0].bSold);TestEqual(TEXT("No early wallet credit"),F.Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),1000000LL);
    TestFalse(TEXT("Typed owned result requires saved commit"),FNativeEconomyWorker::Commit(MoveTemp(*R),F.Pilot.Economy,1,TEXT("Owned/session1")));
    TestFalse(TEXT("Wrong profile cannot receive result"),FNativeEconomyWorker::CommitOwned(MoveTemp(*R),F.Pilot,1,TEXT("Owned/session2"),[]{return true;},F.Why));
    F.Pilot.OwnedShipSkins.Add(TEXT("Changed license"));TestFalse(TEXT("Unversioned ownership changes protected"),FNativeEconomyWorker::CommitOwned(MoveTemp(*R),F.Pilot,1,TEXT("Owned/session1"),[]{return true;},F.Why));F.Pilot.OwnedShipSkins.Reset();
    TestFalse(TEXT("Failed save restores title and money"),FNativeEconomyWorker::CommitOwned(MoveTemp(*R),F.Pilot,1,TEXT("Owned/session1"),[]{return false;},F.Why));TestFalse(TEXT("Hull still owned"),F.Pilot.HangarShips[0].bSold);
    R=F.Run(C);if(!R)return false;F.Pilot.Rank=4;
    TestTrue(TEXT("Validated sale saves"),FNativeEconomyWorker::CommitOwned(MoveTemp(*R),F.Pilot,1,TEXT("Owned/session1"),[]{return true;},F.Why));TestTrue(TEXT("Sold title saved"),F.Pilot.HangarShips[0].bSold);
    TestEqual(TEXT("Worker never overwrites career progression"),F.Pilot.Rank,4);TestTrue(TEXT("Sale credits net proceeds"),F.Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot())>1000000);
    TestFalse(TEXT("Sale receipt cannot replay"),F.Apply(C));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOwnedLastFighterWorkerTest,"Starshatter.Domains.Owned.LastFighterAndPhysicalHullPurchase",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FOwnedLastFighterWorkerTest::RunTest(const FString&)
{
    FOwnedWorkerFixture F;auto C=F.Command(EOwnedCommand::Sell);C.Cost=700;F.Pilot.HangarShips[1].bAssigned=true;
    TestFalse(TEXT("Loaner cannot bypass last fighter protection"),F.Apply(C));F.Pilot.HangarShips[1].bAssigned=false;
    TestTrue(TEXT("Spare allows sale"),F.Apply(C));auto Buy=F.Command(EOwnedCommand::Purchase);Buy.Cost=1000;Buy.PreparedHull=F.Pilot.Economy.UsedHullStock.FindChecked(F.Target);
    TestTrue(TEXT("Purchase returns the selected physical hull"),F.Apply(Buy));TestFalse(TEXT("Reacquired hull is available"),F.Pilot.HangarShips[0].bSold);TestEqual(TEXT("Identity preserved"),F.Pilot.HangarShips[0].HullId,F.Target);
    TestEqual(TEXT("No duplicate hangar row"),F.Pilot.HangarShips.Num(),2);TestFalse(TEXT("No duplicate purchase"),F.Apply(Buy));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOwnedRentalWorkerTest,"Starshatter.Domains.Owned.RentalAndReturn",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FOwnedRentalWorkerTest::RunTest(const FString&)
{
    FOwnedWorkerFixture F;const auto Id=F.Stock();auto Rent=F.Command(EOwnedCommand::Rent);Rent.Hull=Id;Rent.PreparedHull.HullId=Id;Rent.BaseCost=1000;Rent.Days=1;Rent.Cost=70;
    TestTrue(TEXT("Funded rental saves"),F.Apply(Rent));TestEqual(TEXT("Fee and basic premium charged once"),F.Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),999930LL);TestEqual(TEXT("One rental contract"),F.Pilot.ShipRentals.Num(),1);
    const auto* H=F.Pilot.HangarShips.FindByPredicate([Id](const auto& Row){return Row.HullId==Id;});if(!H)return false;TestEqual(TEXT("Hull linked to rental"),H->RentalId,Rent.Receipt);
    auto Return=F.Command(EOwnedCommand::Return);Return.Hull=Id;Return.PreparedHull=*H;TestTrue(TEXT("Rental return saves"),F.Apply(Return));
    TestEqual(TEXT("Rental closed"),F.Pilot.ShipRentals[0].State,EShipRentalState::Returned);TestEqual(TEXT("No automatic fee refund"),F.Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),999930LL);
    TestFalse(TEXT("Return replay refused"),F.Apply(Return));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOwnedInsuranceWorkerTest,"Starshatter.Domains.Owned.PolicyAndLifetimeLoss",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FOwnedInsuranceWorkerTest::RunTest(const FString&)
{
    FOwnedWorkerFixture F;auto Policy=F.Command(EOwnedCommand::Policy);Policy.Tier=EShipInsuranceTier::Basic;Policy.Cost=UShipInsuranceSubsystem::Quote(F.Pilot.HangarShips[0],Policy.Tier).Premium;
    TestTrue(TEXT("Policy saved"),F.Apply(Policy));TestEqual(TEXT("Basic premium charged"),F.Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),999950LL);TestFalse(TEXT("Policy cannot charge twice"),F.Apply(Policy));
    F.Pilot.HangarShips[0].bCashPurchased=true;F.Pilot.HangarShips[0].Insurance.InsuredValue=1200;F.Pilot.AssignedHangarHull=F.Target;
    F.Pilot.Economy.CommittedTransactions.Add(F.Target); // Older purchase receipts could equal the hull identity.
    auto Loss=F.Command(EOwnedCommand::Loss);Loss.bValidatedDeath=true;const int64 Before=F.Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot());
    TestTrue(TEXT("Lifetime loss claim saves"),F.Apply(Loss));TestEqual(TEXT("Lifetime hull and equipment payout"),F.Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),Before+1200);
    TestTrue(TEXT("Permanent loss recorded"),F.Pilot.HangarShips[0].bLost);TestFalse(TEXT("Assigned hull cleared"),F.Pilot.AssignedHangarHull.IsValid());TestFalse(TEXT("No repeat payout"),F.Apply(Loss));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOwnedEquipmentWorkerTest,"Starshatter.Domains.Owned.StoredEquipmentFitAndLoadout",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FOwnedEquipmentWorkerTest::RunTest(const FString&)
{
    FOwnedWorkerFixture F;F.Pilot.AssignedHangarHull=F.Target;FHangarComponent Part;Part.SystemDesign=TEXT("Drive");Part.Component=TEXT("Injector");Part.Level=3;Part.ItemId=FGuid::NewGuid();Part.SystemIndex=0;Part.ComponentIndex=0;Part.InstalledValue=100;
    F.Pilot.HangarShips[0].Components.Add(Part);FStoredShipEquipment Stored;Stored.CatalogId=ShipStoreAI::ComponentId(Part.SystemDesign,Part.Component,4);Stored.Category=EShipStoreCategory::Components;Stored.Location=F.Dock.Location;Stored.Quantity=1;Stored.ComponentState=Part;Stored.ComponentState.Level=4;F.Pilot.StoredShipEquipment.Add(Stored);
    auto Fit=F.Command(EOwnedCommand::Fit);Fit.Grade=4;Fit.Slot=0;Fit.Cost=200;TestTrue(TEXT("Owned component fit saves"),F.Apply(Fit));TestEqual(TEXT("New quality installed"),F.Pilot.HangarShips[0].Components[0].Level,4);
    TestEqual(TEXT("No second purchase fee"),F.Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),1000000LL);TestEqual(TEXT("Removed component retained"),ShipStoreAI::Stored(F.Pilot,ShipStoreAI::ComponentId(Part.SystemDesign,Part.Component,3),F.Dock.Location),1);
    FStoredShipEquipment Missiles;Missiles.CatalogId=TEXT("Missile/Test");Missiles.Category=EShipStoreCategory::Missiles;Missiles.Location=F.Dock.Location;Missiles.Quantity=5;F.Pilot.StoredShipEquipment.Add(Missiles);
    auto Load=F.Command(EOwnedCommand::Loadout);Load.Wanted.Add(Missiles.CatalogId,3);Load.Stations={1};Load.Loadout=TEXT("Test missiles");
    TestTrue(TEXT("Finite missile exchange saves"),F.Apply(Load));TestEqual(TEXT("Only requested missiles consumed"),ShipStoreAI::Stored(F.Pilot,Missiles.CatalogId,F.Dock.Location),2);
    auto Over=F.Command(EOwnedCommand::Loadout);Over.Wanted.Add(Missiles.CatalogId,4);TestFalse(TEXT("Insufficient stock blocks loadout"),F.Apply(Over));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOwnedLossRollbackWorkerTest,"Starshatter.Domains.Owned.LossRollbackCargoAndOtherShips",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FOwnedLossRollbackWorkerTest::RunTest(const FString&)
{
    FOwnedWorkerFixture F;F.Pilot.AssignedHangarHull=F.Target;const FName Hold(*(TEXT("Ship/")+F.Target.ToString(EGuidFormats::Digits)));
    FEconomyLedger::CreateInventory(F.Pilot.Economy,Hold,FEconomyLedger::Pilot(),TEXT("Janus"),10);FEconomyLedger::Deposit(F.Pilot.Economy,Hold,TEXT("Ore"),5,1);
    auto Loss=F.Command(EOwnedCommand::Loss);Loss.bValidatedDeath=true;auto R=F.Run(Loss);if(!R)return false;
    TestFalse(TEXT("Loss save failure rolls back"),FNativeEconomyWorker::CommitOwned(MoveTemp(*R),F.Pilot,1,TEXT("Owned/session1"),[]{return false;},F.Why));
    TestFalse(TEXT("Uncommitted loss does not mark hull lost"),F.Pilot.HangarShips[0].bLost);TestEqual(TEXT("Cargo restored on save failure"),FEconomyLedger::Quantity(F.Pilot.Economy,Hold,TEXT("Ore")),5.);
    TestTrue(TEXT("Loss retry saves"),F.Apply(Loss));TestEqual(TEXT("Lost cargo removed"),FEconomyLedger::Quantity(F.Pilot.Economy,Hold,TEXT("Ore")),0.);TestFalse(TEXT("Other fighter retained"),F.Pilot.HangarShips[1].bLost);TestEqual(TEXT("Rank retained"),F.Pilot.Rank,2);return true;
}
#endif
