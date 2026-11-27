#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EconomySnapshot.h"
#include "EconomyOwnership.h"
#include "EconomyRuntime.h"
#include "ProductionCatalog.h"
#include "CargoCatalogSubsystem.h"
#include "GameStructs_traffic.h"
#include "Engine/GameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomySnapshotTest,"Starshatter.Economy.Service.SnapshotWireOwnershipAndFixture",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FEconomySnapshotTest::RunTest(const FString&)
{
    FString Error;if(!TestTrue(TEXT("Facility catalog"),SSWProductionCatalog::Reload(Error)))return false;
    auto* Cargo=NewObject<UCargoCatalogSubsystem>(NewObject<UGameInstance>());Cargo->LoadCatalog();
    FEconomyState State;State.EconomyClockSeconds=100;State.Wallets.Add(TEXT("Pilot"),5000);State.bLegacyWalletMigrated=true;
    FEconomyLedger::CreateInventory(State,TEXT("Pilot/Cargo"),TEXT("Pilot"),TEXT("Janus"),100);FEconomyLedger::Deposit(State,TEXT("Pilot/Cargo"),TEXT("iron"),5,1);
    FEconomyRuntime::InitializeMarkets(State,*Cargo);
    const auto& Site=SSWProductionCatalog::Facilities().FindChecked(TEXT("Janek_Orbital_Materials_Works"));
    const auto& Recipe=SSWProductionCatalog::Recipes().FindChecked(Site.Recipe);const FName Input=FEconomyRuntime::Warehouse(Site.System,Site.Facility,TEXT("Input")),Output=FEconomyRuntime::Warehouse(Site.System,Site.Facility,TEXT("Output"));
    FEconomyRuntime::EnsureWarehouse(State,Input,TEXT("Janus"),Site.CapacityTonnes);FEconomyRuntime::EnsureWarehouse(State,Output,TEXT("Janus"),Site.CapacityTonnes,true);
    const FName Business(*(TEXT("NPC/")+Site.System+TEXT("/")+Site.Facility));State.Inventories.FindChecked(Input).TradingOwner=Business;State.Inventories.FindChecked(Output).TradingOwner=Business;
    State.Wallets.Add(Business,9007199254740993LL);
    FIndustryJob Job;Job.Id=FGuid(10,20,30,40);Job.Owner=FEconomyRuntime::IndustryOwner();Job.Facility=FName(*Site.Facility);Job.Recipe=Site.Recipe;Job.Input=Input;Job.Output=Output;Job.State=EIndustryJobState::Running;Job.Duration=120;Job.Remaining=30;Job.CompletionAt=130;Job.Lane=0;Job.Products=Recipe.Outputs;
    for(const auto& Product:Job.Products)State.Inventories.FindChecked(Output).ReservedOutputTonnes+=Product.Quantity*Product.UnitTonnes;
    State.Jobs.Add(Job);FIndustryJob PlayerJob=Job;PlayerJob.Id=FGuid(11,21,31,41);PlayerJob.Owner=TEXT("Pilot");State.Jobs.Add(PlayerJob);
    FGalaxyTrafficState Traffic;Traffic.Clock=100;FGalaxyTrafficJourney Trip;Trip.Id=FGuid(100,200,300,400);Trip.Hold=TEXT("Traffic/Fixture");Trip.SourceInventory=Output;Trip.DestinationInventory=Input;
    Trip.Phase=EGalaxyTrafficPhase::Traveling;Trip.Item=Recipe.Outputs[0].Item;Trip.Quantity=1;Trip.UnitTonnes=Recipe.Outputs[0].UnitTonnes;Trip.Remaining=15;Trip.Route={TEXT("Janus/Janek"),TEXT("Janus/Janek")};Trip.Buyer=Business;Trip.Seller=Business;Trip.TradeCredits=10;Trip.bDestinationReserved=true;
    FEconomyLedger::CreateInventory(State,Trip.Hold,FEconomyRuntime::IndustryOwner(),TEXT("Janus"),1000);FEconomyLedger::Deposit(State,Trip.Hold,Trip.Item,1,Trip.UnitTonnes);State.Inventories.FindChecked(Input).ReservedOutputTonnes+=Trip.UnitTonnes;
    State.Wallets.Add(FName(*(TEXT("NPC freight escrow/")+Trip.Id.ToString(EGuidFormats::Digits))),10);Traffic.Journeys.Add(Trip);
    auto Wire=SSWEconomySnapshot::Capture(State,Traffic,Error);if(!TestTrue(TEXT("Capture actual UE wire"),Wire.IsValid()))return false;
    const auto E=Wire->GetObjectField(TEXT("economy"));const auto Wallets=E->GetObjectField(TEXT("wallets"));
    TestFalse(TEXT("No pilot wallet crosses boundary"),Wallets->HasField(TEXT("Pilot")));TestFalse(TEXT("No pilot hold crosses boundary"),E->GetObjectField(TEXT("inventories"))->HasField(TEXT("Pilot/Cargo")));
    TestEqual(TEXT("Large budget stays exact decimal text"),Wallets->GetStringField(Business.ToString()),FString(TEXT("9007199254740993")));
    TestEqual(TEXT("Only NPC job exported"),E->GetArrayField(TEXT("jobs")).Num(),1);
    FString Json;FJsonSerializer::Serialize(Wire.ToSharedRef(),TJsonWriterFactory<>::Create(&Json));const auto Path=FPaths::ProjectSavedDir()/TEXT("Automation/EconomyWireFixture.json");IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
    TestTrue(TEXT("Real UE fixture for native process integration"),FFileHelper::SaveStringToFile(Json,*Path));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyProjectionTest,"Starshatter.Economy.Service.ProjectionPreservesPlayer",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FEconomyProjectionTest::RunTest(const FString&)
{
    FString File,Json,Reason;TSharedPtr<FJsonObject> Reply;
    if(FParse::Value(FCommandLine::Get(),TEXT("SSWEconomyProjectionFixture="),File))
    {
        if(!TestTrue(TEXT("Actual native projection file"),FFileHelper::LoadFileToString(Json,*File)&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Reply)))return false;
    }
    else
    {
        auto Economy=MakeShared<FJsonObject>(),Wallets=MakeShared<FJsonObject>(),Inventories=MakeShared<FJsonObject>(),Inv=MakeShared<FJsonObject>(),Trip=MakeShared<FJsonObject>();
        Wallets->SetStringField(TEXT("NPC/Fixture"),TEXT("9007199254740993"));Inv->SetStringField(TEXT("id"),FGuid(1,2,3,4).ToString());Inv->SetStringField(TEXT("owner"),TEXT("Civilian industry"));Inv->SetNumberField(TEXT("capacityTonnes"),100);Inv->SetStringField(TEXT("tradingOwner"),TEXT("NPC/Fixture"));Inventories->SetObjectField(TEXT("Janus/Fixture/Stock"),Inv);
        Economy->SetObjectField(TEXT("wallets"),Wallets);Economy->SetObjectField(TEXT("inventories"),Inventories);Economy->SetNumberField(TEXT("economyClockSeconds"),100);Trip->SetArrayField(TEXT("journeys"),{});Reply=MakeShared<FJsonObject>();Reply->SetObjectField(TEXT("economy"),Economy);Reply->SetObjectField(TEXT("traffic"),Trip);
    }
    FEconomyState Remote;FGalaxyTrafficState Trips;TMap<FGuid,FString> Ids;
    if(!TestTrue(TEXT("Decode server view"),SSWEconomySnapshot::Decode(Reply,Remote,Trips,Ids,Reason))){AddError(Reason);return false;}
    FEconomyState Local;Local.Wallets.Add(TEXT("Pilot"),5000);FEconomyLedger::CreateInventory(Local,TEXT("Pilot/Hold"),TEXT("Pilot"),TEXT("Janus"),100);FEconomyLedger::Deposit(Local,TEXT("Pilot/Hold"),TEXT("Iron"),7,1);const auto Hold=Local.Inventories.FindChecked(TEXT("Pilot/Hold"));
    SSWEconomyOwnership::ApplyNpcProjection(Local,Remote);
    TestEqual(TEXT("Projection does not alter pilot credits"),Local.Wallets.FindRef(TEXT("Pilot")),5000LL);
    TestTrue(TEXT("Pilot cargo and item IDs retained"),FEconomyInventory::StaticStruct()->CompareScriptStruct(&Hold,&Local.Inventories.FindChecked(TEXT("Pilot/Hold")),0));
    TestTrue(TEXT("Server clock is copied, never locally advanced"),Local.EconomyClockSeconds==Remote.EconomyClockSeconds);
    TestTrue(TEXT("NPC inventories displayed"),Local.Inventories.Num()>1);
    return true;
}
#endif
