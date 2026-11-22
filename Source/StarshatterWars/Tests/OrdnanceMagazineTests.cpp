#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "OrdnanceMagazine.h"
#include "ShipStoreAI.h"
#include "EconomyRuntime.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOrdnanceStoreTest,"Starshatter.ShipStore.PlayerAndCarrierMissileInventories",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOrdnanceStoreTest::RunTest(const FString&)
{
    FS_PlayerGameInfo Pilot;FText Why;const FName Magazine(TEXT("Ordnance/Carrier/Orion")),Archer(TEXT("Missile/Archer")),Javelin(TEXT("Missile/Javelin"));
    const FString Dock=TEXT("Carrier / Orion");
    FEconomyLedger::CreateInventory(Pilot.Economy,Magazine,FEconomyRuntime::IndustryOwner(),TEXT("Janus"),100);
    FEconomyLedger::Deposit(Pilot.Economy,Magazine,Archer,12,.01);FEconomyLedger::Deposit(Pilot.Economy,Magazine,Javelin,10,.01);
    TMap<FName,int32> FourArcher;FourArcher.Add(Archer,4);
    TestTrue(TEXT("Training draws initial missiles from carrier"),OrdnanceMagazine::Exchange(Pilot,Dock,Magazine,{},FourArcher,Why));
    TestEqual(TEXT("Carrier magazine debited"),FEconomyLedger::Quantity(Pilot.Economy,Magazine,Archer),8.0);
    TestTrue(TEXT("Selecting an unchanged full loadout costs no extra missiles"),OrdnanceMagazine::Exchange(Pilot,Dock,Magazine,FourArcher,FourArcher,Why));
    TestEqual(TEXT("No duplicate debit"),FEconomyLedger::Quantity(Pilot.Economy,Magazine,Archer),8.0);
    TMap<FName,int32> TwoArcher;TwoArcher.Add(Archer,2);
    TestTrue(TEXT("Only expended missiles are replenished"),OrdnanceMagazine::Exchange(Pilot,Dock,Magazine,TwoArcher,FourArcher,Why));
    TestEqual(TEXT("Carrier supplies the missing two"),FEconomyLedger::Quantity(Pilot.Economy,Magazine,Archer),6.0);
    TMap<FName,int32> FourJavelin;FourJavelin.Add(Javelin,4);
    TestTrue(TEXT("Changing loadout returns unused old missiles"),OrdnanceMagazine::Exchange(Pilot,Dock,Magazine,FourArcher,FourJavelin,Why));
    TestEqual(TEXT("Unused Archers return to carrier"),FEconomyLedger::Quantity(Pilot.Economy,Magazine,Archer),10.0);
    TestEqual(TEXT("New Javelins leave carrier"),FEconomyLedger::Quantity(Pilot.Economy,Magazine,Javelin),6.0);
    TMap<FName,int32> TooMany;TooMany.Add(Javelin,100);
    TestFalse(TEXT("A depleted carrier cannot create free ammunition"),OrdnanceMagazine::Exchange(Pilot,Dock,Magazine,FourJavelin,TooMany,Why));
    TestEqual(TEXT("Shortage is atomic"),FEconomyLedger::Quantity(Pilot.Economy,Magazine,Javelin),6.0);
    TestFalse(TEXT("Free flight never silently uses carrier stock"),OrdnanceMagazine::Exchange(Pilot,Dock,NAME_None,{},FourArcher,Why));
    auto& Stores=Pilot.StoredShipEquipment.AddDefaulted_GetRef();Stores.CatalogId=Archer;Stores.Category=EShipStoreCategory::Missiles;Stores.Location=Dock;Stores.Quantity=5;
    TestTrue(TEXT("Free flight uses purchased local player stock"),OrdnanceMagazine::Exchange(Pilot,Dock,NAME_None,TwoArcher,FourArcher,Why));
    TestEqual(TEXT("Only player store debited in free flight"),ShipStoreAI::Stored(Pilot,Archer,Dock),3);
    TestEqual(TEXT("Carrier unchanged by player-store loadout"),FEconomyLedger::Quantity(Pilot.Economy,Magazine,Archer),10.0);
    TestFalse(TEXT("Remote player stock cannot load here"),OrdnanceMagazine::Exchange(Pilot,TEXT("Station / Janek"),NAME_None,{},FourArcher,Why));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIMagazineRefillTest,"Starshatter.ShipStore.AIFiniteMagazineReplenishment",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAIMagazineRefillTest::RunTest(const FString&)
{
    FEconomyState State;const FName Magazine(TEXT("Ordnance/Carrier/Orion")),Item(TEXT("Missile/Archer"));
    FEconomyLedger::CreateInventory(State,Magazine,FEconomyRuntime::IndustryOwner(),TEXT("Janus"),10);
    FEconomyLedger::Deposit(State,Magazine,Item,5,.01);
    const int32 Loaded=OrdnanceMagazine::ReplenishCount(State,Magazine,Item,0,4);
    TestEqual(TEXT("Fresh AI aircraft consumes four carrier missiles"),Loaded,4);
    TestEqual(TEXT("Carrier retains only one"),FEconomyLedger::Quantity(State,Magazine,Item),1.0);
    TestEqual(TEXT("Repeated checks cannot debit a full aircraft"),OrdnanceMagazine::ReplenishCount(State,Magazine,Item,Loaded,4),4);
    TestEqual(TEXT("A partially empty aircraft gets only real stock"),OrdnanceMagazine::ReplenishCount(State,Magazine,Item,2,4),3);
    TestEqual(TEXT("Depleted magazine stays empty"),FEconomyLedger::Quantity(State,Magazine,Item),0.0);
    TestEqual(TEXT("No implicit replenishment when empty"),OrdnanceMagazine::ReplenishCount(State,Magazine,Item,0,4),0);
    return true;
}
#endif
