#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "ShipStoreAI.h"
#include "ShipStoreDlg.h"
#include "ShipSkinDlg.h"
#include "PlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "ScopedTestWorldContext.h"
#include "Misc/ScopeExit.h"
#include "Widgets/Text/STextBlock.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShipStoreTransactionTest,"Starshatter.ShipStore.PurchaseStorageAndRollback",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShipStoreTransactionTest::RunTest(const FString&)
{
    FS_PlayerGameInfo Pilot;Pilot.Economy.Wallets.Add(FEconomyLedger::Pilot(),50000);
    FShipStoreOffer Offer;Offer.Id=ShipStoreAI::ComponentId(TEXT("Fusion Reactor"),TEXT("Power Regulator"),4);
    Offer.SystemDesign=TEXT("Fusion Reactor");Offer.Component=TEXT("Power Regulator");Offer.Grade=4;
    Offer.Category=EShipStoreCategory::Components;Offer.Price=100;
    FText Why;const FString Dock=TEXT("Carrier / CV6-Orion");const auto Receipt=FGuid::NewGuid();
    TestFalse(TEXT("Flight cannot purchase"),ShipStoreAI::Purchase(Pilot,Offer,3,TEXT(""),true,Receipt,[]{return true;},Why));
    TestFalse(TEXT("Clients cannot mutate authoritative inventory"),ShipStoreAI::Purchase(Pilot,Offer,3,Dock,false,Receipt,[]{return true;},Why));
    TestFalse(TEXT("Negative quantity rejected"),ShipStoreAI::CanPurchase(Pilot,Offer,-1,Dock,true,Why));
    auto Huge=Offer;Huge.Price=MAX_int64;
    TestFalse(TEXT("Price multiplication cannot overflow"),ShipStoreAI::CanPurchase(Pilot,Huge,2,Dock,true,Why));
    TestTrue(TEXT("Bulk purchase succeeds"),ShipStoreAI::Purchase(Pilot,Offer,3,Dock,true,Receipt,[]{return true;},Why));
    TestEqual(TEXT("Exact total charged"),Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),int64(49700));
    TestEqual(TEXT("Equipment remains at delivery dock"),ShipStoreAI::Stored(Pilot,Offer.Id,Dock),3);
    TestEqual(TEXT("Remote stations do not share equipment"),ShipStoreAI::Stored(Pilot,Offer.Id,TEXT("Station / Janek")),0);
    TestEqual(TEXT("Wallet receipt records item quantity"),Pilot.Economy.Journal.Last().Quantity,3.0);
    TestFalse(TEXT("Replayed receipt cannot buy twice"),ShipStoreAI::Purchase(Pilot,Offer,3,Dock,true,Receipt,[]{return true;},Why));
    TestFalse(TEXT("Failed save rolls back"),ShipStoreAI::Purchase(Pilot,Offer,2,Dock,true,FGuid::NewGuid(),[]{return false;},Why));
    TestEqual(TEXT("Failed save restores credits"),Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),int64(49700));
    TestEqual(TEXT("Failed save restores equipment"),ShipStoreAI::Stored(Pilot,Offer.Id,Dock),3);
    TestEqual(TEXT("Failed save restores wallet journal"),Pilot.Economy.Journal.Num(),1);
    FHangarComponent Part;
    TestFalse(TEXT("Cannot fit equipment at another station"),ShipStoreAI::TakeComponent(Pilot,Offer.SystemDesign,Offer.Component,4,TEXT("Station / Janek"),Part));
    TestTrue(TEXT("Fitting consumes a purchased component"),ShipStoreAI::TakeComponent(Pilot,Offer.SystemDesign,Offer.Component,4,Dock,Part));
    TestEqual(TEXT("Fitting consumes one unit"),ShipStoreAI::Stored(Pilot,Offer.Id,Dock),2);
    TestEqual(TEXT("Fitting never charges twice"),Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),int64(49700));
    Part.bHasCondition=true;Part.ConditionPercent=41;Part.MaintenanceSeconds=17;
    ShipStoreAI::StoreComponent(Pilot,Part,Dock);
    // Consume pristine stock first, then inspect the preserved removed part.
    ShipStoreAI::TakeComponent(Pilot,Offer.SystemDesign,Offer.Component,4,Dock,Part);
    ShipStoreAI::TakeComponent(Pilot,Offer.SystemDesign,Offer.Component,4,Dock,Part);
    TestTrue(TEXT("Removed component remains recoverable"),ShipStoreAI::TakeComponent(Pilot,Offer.SystemDesign,Offer.Component,4,Dock,Part));
    TestEqual(TEXT("Storage does not repair damaged equipment"),Part.ConditionPercent,41.f);
    TestEqual(TEXT("Repair progress survives storage"),Part.MaintenanceSeconds,17.f);
    FShipStoreOffer Skin;Skin.Id=TEXT("Falcon_NavalBlue");Skin.Category=EShipStoreCategory::Skins;Skin.Price=5000;
    TestTrue(TEXT("Store unlocks skin license"),ShipStoreAI::Purchase(Pilot,Skin,1,Dock,true,FGuid::NewGuid(),[]{return true;},Why));
    TestTrue(TEXT("Skin license available to application panel"),Pilot.OwnedShipSkins.Contains(Skin.Id));
    TestFalse(TEXT("Owned licenses cannot be charged twice"),ShipStoreAI::CanPurchase(Pilot,Skin,1,Dock,true,Why));
    Offer.Id=TEXT("Missile/Archer");Offer.Category=EShipStoreCategory::Missiles;
    TestTrue(TEXT("Missiles purchase into dock storage"),ShipStoreAI::Purchase(Pilot,Offer,4,Dock,true,FGuid::NewGuid(),[]{return true;},Why));
    Offer.Id=TEXT("Drone/MiningDrone");Offer.Category=EShipStoreCategory::Drones;
    TestTrue(TEXT("Drones purchase into dock storage"),ShipStoreAI::Purchase(Pilot,Offer,2,Dock,true,FGuid::NewGuid(),[]{return true;},Why));
    FEconomyLedger::CreateInventory(Pilot.Economy,TEXT("Ship/MiningTest"),FEconomyLedger::Pilot(),TEXT("Janus"),1);
    TestFalse(TEXT("Failed cargo save restores stored drones"),ShipStoreAI::TransferMiningDrone(Pilot,Dock,TEXT("Ship/MiningTest"),1,true,[]{return false;},Why));
    TestEqual(TEXT("Rolled back cargo transfer restores dock quantity"),ShipStoreAI::Stored(Pilot,Offer.Id,Dock),2);
    TestTrue(TEXT("Purchased spare drone can enter cargo"),ShipStoreAI::TransferMiningDrone(Pilot,Dock,TEXT("Ship/MiningTest"),1,true,[]{return true;},Why));
    TestEqual(TEXT("Cargo transfer consumes one stored drone"),ShipStoreAI::Stored(Pilot,Offer.Id,Dock),1);
    auto* Save=NewObject<UPlayerSaveGame>();Save->PlayerInfo=Pilot;TArray<uint8> Bytes;
    TestTrue(TEXT("Equipment and wallet serialize together"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
    auto* Reload=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if(TestNotNull(TEXT("Equipment profile reloads"),Reload))
    {
        TestEqual(TEXT("Drone quantity survives restart"),ShipStoreAI::Stored(Reload->PlayerInfo,Offer.Id,Dock),1);
        TestTrue(TEXT("Skin ownership survives restart"),Reload->PlayerInfo.OwnedShipSkins.Contains(Skin.Id));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShipStoreTabsTest,"Starshatter.ShipStore.FiveTabsAndFinalPriceColumns",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShipStoreTabsTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false);FScopedSSWTestWorldContext Context(World);
    ON_SCOPE_EXIT{World->DestroyWorld(false);};
    auto* Screen=NewObject<UShipStoreDlg>(World);Screen->Initialize();const auto Widget=Screen->TakeWidget();
    TSet<FString> Texts;
    TFunction<void(const TSharedRef<SWidget>&,int32)> Visit;
    Visit=[&](const TSharedRef<SWidget>& Item,int32 Depth)
    {
        if(Depth>100)return;
        if(Item->GetType()==FName(TEXT("STextBlock")))Texts.Add(StaticCastSharedRef<STextBlock>(Item)->GetText().ToString());
        if(auto* Children=Item->GetChildren())for(int32 Index=0;Index<Children->Num();++Index)Visit(Children->GetChildAt(Index),Depth+1);
    };
    Visit(Widget,0);
    for(const TCHAR* Label:{TEXT("HULLS"),TEXT("SKINS"),TEXT("COMPONENTS"),TEXT("MISSILES"),TEXT("DRONES"),TEXT("BASE CR"),TEXT("TAX / FEES"),TEXT("TOTAL CR")})
        TestTrue(*FString::Printf(TEXT("Store presents %s"),Label),Texts.Contains(Label));
    return true;
}
#endif
