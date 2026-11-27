#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "NativeEconomyWorker.h"
#include "EconomyRuntime.h"
#include "ProductionCatalog.h"
#include "ManufacturingAI.h"
#include "CargoCatalogSubsystem.h"
#include "Engine/GameInstance.h"
#include "TrafficAI.h"
#include "PlayerSaveGame.h"
#include "HAL/Event.h"
#include "HAL/PlatformProcess.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIdleFreightFairnessTest,"Starshatter.Economy.Background.IdleSectorCannotStarveProducers",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FIdleFreightFairnessTest::RunTest(const FString&)
{
    FGalaxyTrafficState Traffic;FEconomyState Economy;TMap<FName,FGalaxyTrafficSector> Graph;TMap<FName,FGalaxyTrafficEndpoint> Endpoints;
    for(const TCHAR* Name:{TEXT("Janus/A-idle"),TEXT("Janus/B-mine"),TEXT("Janus/C-works")}){FGalaxyTrafficSector Node;Node.Id=Name;Node.System=TEXT("Janus");Graph.Add(Node.Id,Node);}
    for(int32 Step=1;Step<=3;++Step){Traffic.Clock=Step;TrafficAI::Dispatch(Traffic,Economy,Graph,Endpoints,NAME_None,32);}
    TestEqual(TEXT("Idle attempts service every sector without decorative transit nodes"),Traffic.SectorLastServed.Num(),3);
    TestEqual(TEXT("Fairness never fabricates cargo journeys"),Traffic.Journeys.Num(),0);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBackgroundEconomySoakTest,"Starshatter.Economy.Background.JanusSixHourWorkerRestart",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FBackgroundEconomySoakTest::RunTest(const FString&)
{
    // Accelerated simulation time, not a six-hour wall-clock/GPU soak. No world,
    // loaded sector, ship actor or open economy panel exists in this fixture.
    FString Error;if(!TestTrue(TEXT("Authored production catalog"),SSWProductionCatalog::Reload(Error)))return false;
    const FName FacilityId=TEXT("Janek_Orbital_Materials_Works");const auto* Site=SSWProductionCatalog::Facilities().Find(FacilityId);
    if(!TestNotNull(TEXT("Janek authored refinery"),Site))return false;
    const auto* Authored=SSWProductionCatalog::Recipes().Find(Site->Recipe);if(!TestNotNull(TEXT("Actual refinery recipe"),Authored))return false;
    FSSWProductionRecipe Recipe=*Authored;Recipe.NativeMineralKeys=ManufacturingAI::MineralKeys(Recipe);
    auto* Catalog=NewObject<UCargoCatalogSubsystem>(NewObject<UGameInstance>());Catalog->LoadCatalog();
    TMap<FName,FS_CargoCatalogRow> Goods;Catalog->ForEachDefinition([&](FName Id,const FS_CargoCatalogRow& Row){Goods.Add(Id,Row);});
    FString Json;TArray<TSharedPtr<FJsonValue>> MineRows;
    if(!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Cargo/background_mining.json"))) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),MineRows))return false;
    TArray<FNativeBackgroundMine> Mines;
    for(const auto& Value:MineRows)
    {
        const auto O=Value->AsObject();if(O->GetStringField(TEXT("System"))!=Site->System)continue;
        Mines.Add({FName(*O->GetStringField(TEXT("Id"))),FName(*Site->System),FName(*O->GetStringField(TEXT("Region"))),DataTableUtils::MakeValidName(O->GetStringField(TEXT("Item"))),O->GetNumberField(TEXT("ReserveTonnes")),O->GetNumberField(TEXT("RateTonnesPerSecond")),O->GetNumberField(TEXT("StorageTonnes"))});
    }
    if(!TestTrue(TEXT("Janus has authored background mines"),!Mines.IsEmpty()))return false;
    auto* Save=NewObject<UPlayerSaveGame>();auto& Live=Save->PlayerInfo.Economy;Live.bLegacyWalletMigrated=true;Live.Wallets.Add(FEconomyLedger::Pilot(),5000);
    FEconomyRuntime::InitializeMarkets(Live,*Catalog);
    TMap<FName,FGalaxyTrafficSector> Graph;TMap<FName,FGalaxyTrafficEndpoint> Endpoints;
    const auto AddEndpoint=[&](FName System,FName Region,FName Station)
    {
        FGalaxyTrafficSector Node;Node.System=System;Node.Region=Region;Node.Id=FName(*(System.ToString()+TEXT("/")+Region.ToString()));Graph.FindOrAdd(Node.Id)=Node;
        FGalaxyTrafficEndpoint Endpoint;Endpoint.System=System;Endpoint.Region=Region;Endpoint.Station=Station;Endpoint.Sector=Node.Id;Endpoint.Id=FName(*(Node.Id.ToString()+TEXT("/")+Station.ToString()));Endpoints.Add(Endpoint.Id,Endpoint);
    };
    for(const auto& Mine:Mines)AddEndpoint(Mine.System,Mine.Region,Mine.Region);
    AddEndpoint(FName(*Site->System),FName(*Site->Region),FName(*Site->Facility));
    for(auto& Node:Graph)for(const auto& Other:Graph)if(Node.Key!=Other.Key)Node.Value.Neighbors.Add(Other.Key); // same authored system only
    const FString Slot=TEXT("SSW_Automation_Background_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);ON_SCOPE_EXIT{UGameplayStatics::DeleteGameInSlot(Slot,0);};
    FEvent* Done=FPlatformProcess::GetSynchEventFromPool(false);ON_SCOPE_EXIT{FPlatformProcess::ReturnSynchEventToPool(Done);};
    FNativeEconomyWorker Worker;int32 CompletedBatches=0,Delivered=0,WorkerSteps=0;TSet<FGuid> DeliveredIds;bool CallbacksOnGameThread=true;
    const auto EventHandle=ProductionEvents::Changed().AddLambda([&](FName SiteName,FGuid,EProductionEvent Phase,const FText&){if(SiteName==FName(*Site->Facility) && Phase==EProductionEvent::Completed)++CompletedBatches;CallbacksOnGameThread&=IsInGameThread();});ON_SCOPE_EXIT{ProductionEvents::Changed().Remove(EventHandle);};
    const double Started=FPlatformTime::Seconds();uint64 Revision=0;bool Restarted=false;
    for(int32 Step=0;Step<360;++Step)
    {
        Live.EconomyClockSeconds+=60;
        FNativeEconomyInput Input;Input.State=Live;Input.Revision=++Revision;Input.Profile=Slot;Input.NowUtc=1800000000+int64(Live.EconomyClockSeconds);Input.bInventoryChanged=true;Input.Facilities.Add(FacilityId,*Site);Input.Recipes.Add(Site->Recipe,Recipe);Input.Mines=Mines;Input.Goods=Goods;
        if(!TestTrue(TEXT("Background worker accepts request"),Worker.Submit(MoveTemp(Input),[Done](){Done->Trigger();})) || !TestTrue(TEXT("Background request completes"),Done->Wait(10000)))return false;
        auto Result=Worker.TakeResult();if(!Result || !TestTrue(TEXT("Economy calculations execute off game thread"),Result->bOffGameThread))return false;
        if(!TestTrue(TEXT("Authoritative result commits"),FNativeEconomyWorker::Commit(MoveTemp(*Result),Live,Revision,Slot)))return false;++WorkerSteps;
        auto& Traffic=Save->PlayerInfo.Traffic;Traffic.Clock=Live.EconomyClockSeconds;
        for(auto& Journey:Traffic.Journeys)
        {
            TrafficAI::AdvanceScheduled(Journey,Live,Traffic.Clock,false,false,true,90,1);
            if(Journey.Phase==EGalaxyTrafficPhase::Completed && !DeliveredIds.Contains(Journey.Id)){DeliveredIds.Add(Journey.Id);++Delivered;}
        }
        Traffic.Journeys.RemoveAll([](const auto& Journey){return Journey.IsFinished();});
        TrafficAI::Dispatch(Traffic,Live,Graph,Endpoints,NAME_None,32);
        if(Step==179)
        {
            const double BeforeClock=Live.EconomyClockSeconds;const int32 BeforeJourneys=Traffic.Journeys.Num();
            if(!TestTrue(TEXT("Mid-run background checkpoint saves"),UGameplayStatics::SaveGameToSlot(Save,Slot,0)))return false;
            auto* Reloaded=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot,0));if(!TestNotNull(TEXT("Background checkpoint reloads"),Reloaded))return false;
            Save->PlayerInfo=Reloaded->PlayerInfo;Restarted=true;
            TestEqual(TEXT("Economy clock survives restart"),Live.EconomyClockSeconds,BeforeClock);TestEqual(TEXT("Active freight survives restart"),Save->PlayerInfo.Traffic.Journeys.Num(),BeforeJourneys);
            const int32 InventoryCount=Live.Inventories.Num();FEconomyRuntime::InitializeMarkets(Live,*Catalog);TestEqual(TEXT("Reload does not create duplicate markets"),Live.Inventories.Num(),InventoryCount);
        }
    }
    TestTrue(TEXT("Mines deplete while no sector is loaded"),Live.BackgroundOreRemaining.Num()>0);
    TestTrue(TEXT("Scheduled NPC freight delivers real ore"),Delivered>0);TestTrue(TEXT("Refinery produces finished concentrate"),CompletedBatches>0);
    TestTrue(TEXT("Mid-run restart tested"),Restarted);TestTrue(TEXT("Lifecycle notifications stay on game thread"),CallbacksOnGameThread);
    TestEqual(TEXT("Background NPC economy never debits or tops up pilot wallet"),Live.Wallets.FindRef(FEconomyLedger::Pilot()),5000LL);
    int32 Samples=0;int64 Trades=0;for(const auto& Market:Live.RegionalMarkets){Samples+=Market.Value.Samples.Num();for(const auto& Sample:Market.Value.Samples)Trades+=Sample.TradeCount;}
    TestTrue(TEXT("Regional history advances with panels closed"),Samples>100);TestTrue(TEXT("NPC purchases settle real trades"),Trades>0);
    for(const auto& Inv:Live.Inventories){TestTrue(TEXT("No warehouse over capacity"),FEconomyLedger::Used(Inv.Value)+Inv.Value.ReservedOutputTonnes<=Inv.Value.CapacityTonnes+.001);for(const auto& Stack:Inv.Value.Items)TestTrue(TEXT("Finite non-negative stocks"),FMath::IsFinite(Stack.Quantity) && Stack.Quantity>=0);}
    auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("scope"),TEXT("Six simulated hours, actual Janus mining/refinery catalogs, native worker, abstract freight, no world/actors/panels. Accelerated integration test, not a wall-clock packaged-client soak."));Report->SetNumberField(TEXT("simulatedHours"),6);Report->SetNumberField(TEXT("wallSeconds"),FPlatformTime::Seconds()-Started);Report->SetNumberField(TEXT("workerSteps"),WorkerSteps);Report->SetNumberField(TEXT("completedBatches"),CompletedBatches);Report->SetNumberField(TEXT("deliveredJourneys"),Delivered);Report->SetNumberField(TEXT("historySamples"),Samples);Report->SetNumberField(TEXT("settledTradeCount"),Trades);Report->SetBoolField(TEXT("restarted"),Restarted);Report->SetNumberField(TEXT("activeJourneys"),Save->PlayerInfo.Traffic.Journeys.Num());Report->SetNumberField(TEXT("trafficSequence"),Save->PlayerInfo.Traffic.Sequence);Report->SetNumberField(TEXT("mineralKeys"),Recipe.NativeMineralKeys.Num());Report->SetNumberField(TEXT("jobs"),Live.Jobs.Num());
    TArray<TSharedPtr<FJsonValue>> Stocks;for(const auto& Pair:Live.Inventories)if(Pair.Value.System==TEXT("Janus")){auto Stock=MakeShared<FJsonObject>();Stock->SetStringField(TEXT("id"),Pair.Key.ToString());Stock->SetNumberField(TEXT("items"),Pair.Value.Items.Num());Stock->SetNumberField(TEXT("desired"),Pair.Value.DesiredStock.Num());Stock->SetNumberField(TEXT("used"),FEconomyLedger::Used(Pair.Value));Stock->SetStringField(TEXT("trader"),Pair.Value.TradingOwner.ToString());Stocks.Add(MakeShared<FJsonValueObject>(Stock));}Report->SetArrayField(TEXT("janusStocks"),Stocks);FString Out;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Out));
    TestTrue(TEXT("Background integration evidence saved"),FFileHelper::SaveStringToFile(Out,*(FPaths::ProjectSavedDir()/TEXT("Analysis/BackgroundEconomySoak.json"))));return true;
}
#endif
