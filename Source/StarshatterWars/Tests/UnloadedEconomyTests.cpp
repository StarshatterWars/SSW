#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EconomyAI.h"
#include "ShipyardAI.h"
#include "ProductionCatalog.h"
#include "CargoCatalogSubsystem.h"
#include "EconomyRuntime.h"
#include "TrafficAI.h"
#include "PlayerSaveGame.h"
#include "LocalGameStateSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/SecureHash.h"
#include "SkillsSubsystem.h"
#include "GalaxyTrafficSubsystem.h"
#include "Engine/GameInstance.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnloadedIndustryTest,"Starshatter.Economy.UnloadedSystems.MineHaulRefineAndRestart",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FUnloadedIndustryTest::RunTest(const FString&)
{
    // Intentionally no world, level, station actor or ship actor in this test.
    auto* Save=NewObject<UPlayerSaveGame>();auto& State=Save->PlayerInfo.Economy;auto& Traffic=Save->PlayerInfo.Traffic;
    const FName Ore=DataTableUtils::MakeValidName(TEXT("Magnetite")),Owner=FEconomyRuntime::IndustryOwner();
    const FName Source(TEXT("Janus/Belt/Ore")),Input(TEXT("Solus/Works/Input")),Output(TEXT("Solus/Works/Output"));
    TestTrue(TEXT("Mining operates without a loaded field"),EconomyAI::AdvanceMine(State,TEXT("Deposit"),Source,TEXT("Janus"),Ore,10,2,20,5));
    TestEqual(TEXT("Mining depletes finite native reserve"),State.BackgroundOreRemaining.FindChecked(TEXT("Deposit")),0.0);
    TestFalse(TEXT("Repeated ticks cannot respawn depleted ore"),EconomyAI::AdvanceMine(State,TEXT("Deposit"),Source,TEXT("Janus"),Ore,10,2,20,5));
    FEconomyRuntime::EnsureWarehouse(State,Input,TEXT("Solus"),100);
    FGalaxyTrafficEndpoint A;A.Id=TEXT("Janus/Belt/Mine");A.Sector=TEXT("Janus/Belt");A.System=TEXT("Janus");
    FGalaxyTrafficEndpoint B;B.Id=TEXT("Solus/Orbital/Works");B.Sector=TEXT("Solus/Orbital");B.System=TEXT("Solus");
    const auto Id=TrafficAI::Schedule(Traffic,State,A,B,{A.Sector,B.Sector},Source,Input,Ore,8,1);
    if(!TestTrue(TEXT("Unloaded systems reserve a real cargo journey"),Id.IsValid()))return false;
    TrafficAI::Advance(Traffic.Journeys[0],State,3,false,false,10,1);
    TrafficAI::Advance(Traffic.Journeys[0],State,8,false,false,10,1);
    TestEqual(TEXT("Pickup removes only loaded ore"),FEconomyLedger::Quantity(State,Source,Ore),2.0);
    TestEqual(TEXT("No early destination credit"),FEconomyLedger::Quantity(State,Input,Ore),0.0);
    // Simulate the display adapter's physical hold, then unload its entire scene.
    const FName NativeHold(TEXT("Ship/AutomationFreighter"));
    FEconomyLedger::CreateInventory(State,NativeHold,Owner,TEXT("Janus"),1000);
    const FName PreviousHold=Traffic.Journeys[0].Hold;
    TestTrue(TEXT("Visible ship takes ownership of the same cargo"),FEconomyLedger::Transfer(State,Owner,PreviousHold,NativeHold,Ore,8));
    State.Inventories.Remove(PreviousHold);Traffic.Journeys[0].Hold=NativeHold;
    TestTrue(TEXT("Unloading the scene retains cargo in native records"),TrafficAI::AbstractCargo(Traffic.Journeys[0],State,TEXT("Janus")));
    TestFalse(TEXT("Removed display ship leaves no stale hold"),State.Inventories.Contains(NativeHold));
    TestEqual(TEXT("Scene unload retains precisely eight tonnes"),FEconomyLedger::Quantity(State,Traffic.Journeys[0].Hold,Ore),8.0);
    const FString Slot=TEXT("SSW_Automation_Unloaded_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    if(!TestTrue(TEXT("In-flight native state saves to disk"),UGameplayStatics::SaveGameToSlot(Save,Slot,0)))return false;
    auto* Loaded=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot,0));UGameplayStatics::DeleteGameInSlot(Slot,0);
    if(!TestNotNull(TEXT("Reload restores native state"),Loaded))return false;
    auto& E=Loaded->PlayerInfo.Economy;auto& Job=Loaded->PlayerInfo.Traffic.Journeys[0];
    TestEqual(TEXT("Ship identity survives restart"),Job.Id,Id);
    TestEqual(TEXT("Cargo aboard survives restart"),FEconomyLedger::Quantity(E,Job.Hold,Ore),8.0);
    TestEqual(TEXT("Depletion survives restart"),E.BackgroundOreRemaining.FindChecked(TEXT("Deposit")),0.0);
    TrafficAI::Advance(Job,E,10,false,false,10,1);TrafficAI::Advance(Job,E,10,false,false,10,1);
    TestEqual(TEXT("Route arrival alone does not unload"),FEconomyLedger::Quantity(E,Input,Ore),0.0);
    TestTrue(TEXT("Unloading in an unloaded system completes"),TrafficAI::Advance(Job,E,8,false,false,10,1));
    TestEqual(TEXT("Exactly eight tonnes delivered"),FEconomyLedger::Quantity(E,Input,Ore),8.0);
    TrafficAI::Advance(Job,E,100,false,false,10,1);
    TestEqual(TEXT("Retry cannot duplicate delivery"),FEconomyLedger::Quantity(E,Input,Ore),8.0);
    FSSWProductionRecipe Recipe;Recipe.CargoItem=TEXT("industrial concentrate");Recipe.InputTonnes=2;Recipe.OutputTonnes=1;Recipe.OutputQuantity=1;Recipe.SecondsPerBatch=10;
    FCommodityAmount Required;Required.Item=Ore;Required.Quantity=2;Recipe.Inputs={Required};
    FSSWProductionFacility Site;Site.Facility=TEXT("Works");Site.System=TEXT("Solus");Site.Recipe=TEXT("Refine");Site.CapacityTonnes=100;
    TMap<FName,FSSWProductionFacility> Sites;Sites.Add(TEXT("WorksId"),Site);TMap<FName,FSSWProductionRecipe> Recipes;Recipes.Add(Site.Recipe,Recipe);
    E.OfflineFacilities.Add(TEXT("WorksId"));EconomyAI::AdvanceFacilities(E,Sites,Recipes,10);
    TestEqual(TEXT("An offline facility cannot manufacture"),E.Jobs.Num(),0);
    E.OfflineFacilities.Reset();EconomyAI::AdvanceFacilities(E,Sites,Recipes,10);
    TestEqual(TEXT("Factory without an actor creates concentrate"),FEconomyLedger::Quantity(E,Output,DataTableUtils::MakeValidName(Recipe.CargoItem)),1.0);
    TestEqual(TEXT("Factory consumes the real delivery"),FEconomyLedger::Quantity(E,Input,Ore),6.0);
    FEconomyState HullState;
    FEconomyRuntime::EnsureWarehouse(HullState,Input,TEXT("Solus"),100);FEconomyLedger::Deposit(HullState,Input,Ore,8,1);
    auto HullRecipe=Recipe;HullRecipe.ShipDesign=TEXT("Falcon");HullRecipe.CargoItem=TEXT("falcon assembled hull");Recipes[Site.Recipe]=HullRecipe;
    EconomyAI::AdvanceFacilities(HullState,Sites,Recipes,10);
    TestEqual(TEXT("Ordinary factory cannot build a ship"),HullState.Jobs.Num(),0);
    Sites.FindChecked(TEXT("WorksId")).FacilityType=ESSWProductionFacilityType::Shipyard;
    EconomyAI::AdvanceFacilities(HullState,Sites,Recipes,10);
    TestEqual(TEXT("Shipyard produces one assembled hull"),FEconomyLedger::Quantity(HullState,Output,DataTableUtils::MakeValidName(HullRecipe.CargoItem)),1.0);
    TestTrue(TEXT("Completed hulls are not fractional freighter loads"),HullState.Inventories.FindChecked(Output).bHullStorage);
    TestEqual(TEXT("NPC work never debits the pilot"),E.Wallets.FindRef(FEconomyLedger::Pilot()),0LL);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarketSnapshotTest,"Starshatter.Economy.UnloadedSystems.QuotesAndExecutedVolume",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FMarketSnapshotTest::RunTest(const FString&)
{
    FEconomyState S;const FName System(TEXT("Janus")),Item(TEXT("Ore"));
    TestTrue(TEXT("Quote sample records supply and demand"),EconomyAI::Snapshot(S,System,Item,100,10,50,1000));
    const auto* H=EconomyAI::History(S,System,Item);TestEqual(TEXT("Quote alone has zero traded volume"),H->Samples[0].TradeVolume,0.0);
    EconomyAI::RecordTrade(S,System,Item,2,100);EconomyAI::RecordTrade(S,System,Item,3,180);
    TestFalse(TEXT("Samples are bounded to five-minute cadence"),EconomyAI::Snapshot(S,System,Item,95,10,50,1100));
    TestTrue(TEXT("Next sample includes executed trades"),EconomyAI::Snapshot(S,System,Item,95,10,50,1300));
    H=EconomyAI::History(S,System,Item);TestEqual(TEXT("Volume is actual quantity"),H->Samples.Last().TradeVolume,5.0);
    TestEqual(TEXT("Value is actual settled credits"),H->Samples.Last().TradeValue,280.0);
    TestEqual(TEXT("Trade count is separate from volume"),H->Samples.Last().TradeCount,2);
    EconomyAI::Snapshot(S,System,Item,0,10,50,1600);
    TestEqual(TEXT("Depleted market remains visible"),H->Samples.Last().Supply,0.0);
    TestEqual(TEXT("Old trades are not counted twice"),H->Samples.Last().TradeVolume,0.0);
    for(int32 I=0;I<400;++I)EconomyAI::Snapshot(S,System,Item,0,10,50,1900+I*300);
    TestEqual(TEXT("History retention is bounded"),H->Samples.Num(),288);
    TestFalse(TEXT("Invalid snapshots rejected"),EconomyAI::Snapshot(S,System,Item,-1,10,50,200000));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStateGenerationTest,"Starshatter.Persistence.CoherentGenerationValidation",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FStateGenerationTest::RunTest(const FString&)
{
    auto* M=NewObject<ULocalStateManifest>();M->Generation=FGuid::NewGuid();M->ProfileSlot=TEXT("AutomationProfile");
    TArray<ULocalStateArea*> Areas{NewObject<ULocalPilotState>(),NewObject<ULocalEconomyState>(),NewObject<ULocalSkillState>(),NewObject<ULocalUniverseState>()};
    const TCHAR* Names[]={TEXT("pilot"),TEXT("economy"),TEXT("skills"),TEXT("universe")};
    for(int32 I=0;I<4;++I){Areas[I]->Generation=M->Generation;Areas[I]->ProfileSlot=M->ProfileSlot;M->Slots.Add(TEXT("State_")+FMD5::HashAnsiString(*M->ProfileSlot)+TEXT("_")+M->Generation.ToString(EGuidFormats::Digits)+TEXT("_")+Names[I]);}
    TestTrue(TEXT("Complete matching generation accepted"),ULocalGameStateSubsystem::ValidateGeneration(*M,Areas));
    Areas[1]->Generation=FGuid::NewGuid();TestFalse(TEXT("Mixed economy generation rejected"),ULocalGameStateSubsystem::ValidateGeneration(*M,Areas));Areas[1]->Generation=M->Generation;
    M->Slots[0]=TEXT("../anotherprofile");TestFalse(TEXT("Unexpected slot paths rejected"),ULocalGameStateSubsystem::ValidateGeneration(*M,Areas));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimedTrainingTest,"Starshatter.Economy.TimedSkillsOfflineQueue",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FTimedTrainingTest::RunTest(const FString&)
{
    const FName Id(TEXT("basic"));TMap<FName,FSkillDefinition> Definitions;FSkillDefinition D;D.Name=Id;D.MaximumLevel=10;Definitions.Add(Id,D);
    TMap<FName,FSkillProgress> Learned;Learned.FindOrAdd(Id).Id=FGuid::NewGuid();TArray<FSkillTrainingEntry> Q;FText Why;const int64 Now=100000;
    for(int32 Level=1;Level<=5;++Level)TestTrue(TEXT("Each standard level gets a queue entry"),FSkillTrainingRules::Queue(Definitions,Learned,Q,Id,10,NAME_None,Now,Why));
    TestEqual(TEXT("Five distinct level entries"),Q.Num(),5);
    TestFalse(TEXT("Standard book stops at V"),FSkillTrainingRules::Queue(Definitions,Learned,Q,Id,10,NAME_None,Now,Why));
    TestTrue(TEXT("Offline end times complete all elapsed levels"),FSkillTrainingRules::Advance(Definitions,Learned,Q,Q.Last().FinishesUtc+1));
    TestEqual(TEXT("Offline progress reaches V"),FSkillTrainingRules::Level(Learned.Find(Id)),5);
    Learned.FindChecked(Id).bAdvancedBookOwned=true;
    TestTrue(TEXT("Advanced book unlocks VI"),FSkillTrainingRules::Queue(Definitions,Learned,Q,Id,10,NAME_None,Now+100000,Why));
    FEconomyState Economy;Economy.Wallets.Add(FEconomyLedger::Pilot(),1000000);
    TestTrue(TEXT("Instant training charges and completes one level"),FSkillTrainingRules::InstantTrain(Economy,Definitions,Learned,Q,Q[0].Id,Now+100000,Why));
    TestEqual(TEXT("Level VI instant price follows doubling curve"),Economy.Wallets.FindRef(FEconomyLedger::Pilot()),984000LL);
    TestEqual(TEXT("Instant training grants VI"),FSkillTrainingRules::Level(Learned.Find(Id)),6);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNativeEconomyCatalogTest,"Starshatter.Economy.UnloadedSystems.NativeCatalogsWithoutLevel",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FNativeEconomyCatalogTest::RunTest(const FString&)
{
    auto* Game=NewObject<UGameInstance>();auto* Core=NewObject<UGalaxyTrafficSubsystem>(Game);
    TestTrue(TEXT("Native catalogs load without world actors or data tables"),Core->LoadEconomyCatalogs());
    TestTrue(TEXT("All empire production locations present"),Core->GetFacilityCount()>30);
    TestTrue(TEXT("Background mining exists throughout the galaxy"),Core->GetBackgroundMineCount()>40);
    auto* Cargo=NewObject<UCargoCatalogSubsystem>(Game);Cargo->LoadCatalog();TSet<FName> Items;
    Cargo->ForEachDefinition([&](FName Id,const auto&){Items.Add(Id);});
    for(const auto& Pair:SSWProductionCatalog::Recipes())for(const auto& Input:Pair.Value.Inputs)
        TestTrue(*FString::Printf(TEXT("Recipe %s input %s resolves a real cargo ID"),*Pair.Key.ToString(),*Input.Item.ToString()),Items.Contains(Input.Item));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCFreightBudgetTest,"Starshatter.Economy.NPCBudgetsEscrowAndRecovery",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FNPCFreightBudgetTest::RunTest(const FString&)
{
    FEconomyState E;FGalaxyTrafficState T;const FName Owner=FEconomyRuntime::IndustryOwner(),Item(TEXT("ore")),Source(TEXT("Janus/Mine/Stock")),Drop(TEXT("Solus/Works/Input"));
    FEconomyLedger::CreateInventory(E,Source,Owner,TEXT("Janus"),100);FEconomyLedger::CreateInventory(E,Drop,Owner,TEXT("Solus"),100);FEconomyLedger::Deposit(E,Source,Item,20,1);
    E.Inventories[Source].TradingOwner=TEXT("Mine business");E.Inventories[Source].AskPrices.Add(Item,10);
    E.Inventories[Drop].TradingOwner=TEXT("Works business");E.Inventories[Drop].BidPrices.Add(Item,20);E.Wallets.Add(TEXT("Works business"),1000);
    FGalaxyTrafficEndpoint A;A.Id=TEXT("Mine");A.Sector=TEXT("Janus/Belt");A.System=TEXT("Janus");FGalaxyTrafficEndpoint B;B.Id=TEXT("Works");B.Sector=TEXT("Solus/Orbit");B.System=TEXT("Solus");
    const auto Id=TrafficAI::Schedule(T,E,A,B,{A.Sector,B.Sector},Source,Drop,Item,8,1);
    TestTrue(TEXT("Budget-backed haul accepted"),Id.IsValid());TestEqual(TEXT("Buyer escrows agreed cost"),E.Wallets.FindRef(TEXT("Works business")),920LL);
    TestEqual(TEXT("Supplier not paid before arrival"),E.Wallets.FindRef(TEXT("Mine business")),0LL);
    auto& J=T.Journeys[0];for(int32 I=0;I<5;++I)TrafficAI::Advance(J,E,10,false,false,10,1);
    TestEqual(TEXT("Supplier paid once on unload"),E.Wallets.FindRef(TEXT("Mine business")),80LL);
    TrafficAI::Advance(J,E,100,false,false,10,1);TestEqual(TEXT("Duplicate tick cannot repay supplier"),E.Wallets.FindRef(TEXT("Mine business")),80LL);
    TestEqual(TEXT("Pilot funds untouched"),E.Wallets.FindRef(FEconomyLedger::Pilot()),0LL);
    const auto Lost=TrafficAI::Schedule(T,E,A,B,{A.Sector,B.Sector},Source,Drop,Item,4,1);TestTrue(TEXT("Second haul accepted"),Lost.IsValid());
    auto& Failure=T.Journeys.Last();TrafficAI::Advance(Failure,E,10,false,false,10,1);TrafficAI::Advance(Failure,E,10,false,false,10,1);
    E.Inventories[Drop].bOperational=false;TrafficAI::Advance(Failure,E,60,false,false,10,1);
    TMap<FName,FGalaxyTrafficSector> Graph;FGalaxyTrafficSector N;N.Id=A.Sector;N.Neighbors={B.Sector};Graph.Add(N.Id,N);N.Id=B.Sector;N.Neighbors={A.Sector};Graph.Add(N.Id,N);
    TMap<FName,FGalaxyTrafficEndpoint> Points;Points.Add(A.Id,A);Points.Add(B.Id,B);
    TestEqual(TEXT("Offline destination triggers a real return route"),TrafficAI::RecoverBlocked(T,E,Graph,Points),1);
    TestTrue(TEXT("Returning state retained"),Failure.bReturning);TestEqual(TEXT("Undelivered buyer escrow refunded"),E.Wallets.FindRef(TEXT("Works business")),920LL);
    for(int32 I=0;I<5;++I)TrafficAI::Advance(Failure,E,100,false,false,10,1);
    TestEqual(TEXT("Undelivered cargo returned to supplier"),FEconomyLedger::Quantity(E,Source,Item),12.0);
    TestEqual(TEXT("Failed destination receives no extra cargo"),FEconomyLedger::Quantity(E,Drop,Item),8.0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShipyardFulfillmentTest,"Starshatter.Economy.ShipyardStockPurchaseAndGates",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FShipyardFulfillmentTest::RunTest(const FString&)
{
    FS_PlayerGameInfo Pilot;Pilot.Rank=0;Pilot.Empire=0;Pilot.Economy.Wallets.Add(FEconomyLedger::Pilot(),1000000);
    FShipDesign Hull;Hull.ShipName=TEXT("Falcon");Hull.DisplayName=TEXT("Falcon");Hull.BaseCost=574000;Hull.ShipType=2;Hull.Category=EShipCategory::Fighter;Hull.ShipEmpire=EShipEmpire::Terellian;
    const FName Warehouse(TEXT("Janus/Yard/Output"));auto* Inv=FEconomyLedger::CreateInventory(Pilot.Economy,Warehouse,FEconomyRuntime::IndustryOwner(),TEXT("Janus"),100);
    Inv->bHullStorage=true;FEconomyLedger::Deposit(Pilot.Economy,Warehouse,DataTableUtils::MakeValidName(TEXT("falcon assembled hull")),1,14.5);
    FText Why;const FGuid Receipt=FGuid::NewGuid();if(!TestTrue(TEXT("Eligible pilot buys actual stocked hull"),ShipyardAI::Purchase(Pilot,Hull,Warehouse,TEXT("Janek"),Receipt,Why))){AddError(Why.ToString());return false;}
    TestEqual(TEXT("Stock consumed"),ShipyardAI::Stock(Pilot.Economy,Warehouse,TEXT("Falcon")),0.0);
    TestEqual(TEXT("Exact hull price debited"),Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),426000LL);
    if(!TestEqual(TEXT("Owned hull delivered to hangar"),Pilot.HangarShips.Num(),1))return false;
    TestEqual(TEXT("Hangar location retained"),Pilot.HangarShips[0].Location,FString(TEXT("Janek")));
    TestFalse(TEXT("Credit purchase does not receive cash lifetime policy"),Pilot.HangarShips[0].bCashPurchased);
    TestTrue(TEXT("Retry uses original receipt"),ShipyardAI::Purchase(Pilot,Hull,Warehouse,TEXT("Janek"),Receipt,Why));TestEqual(TEXT("Retry cannot grant a second hull"),Pilot.HangarShips.Num(),1);
    TestFalse(TEXT("Empty stock cannot invent another hull"),ShipyardAI::Purchase(Pilot,Hull,Warehouse,TEXT("Janek"),FGuid::NewGuid(),Why));
    Hull.ShipName=TEXT("Berents");Hull.ShipType=0x80;TestFalse(TEXT("Cadet cannot buy a capital ship"),ShipyardAI::Eligible(Pilot,Hull,Why));
    return true;
}
#endif
