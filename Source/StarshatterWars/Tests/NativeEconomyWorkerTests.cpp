#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "NativeEconomyWorker.h"
#include "EconomyRuntime.h"
#include "ProductionCatalog.h"
#include "ManufacturingAI.h"
#include "SimDataEvents.h"
#include "HAL/PlatformProcess.h"
#include "HAL/Event.h"
#include "Misc/ScopeExit.h"
#include <atomic>
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNativeEconomyWorkerTest,"Starshatter.Economy.Events.NativeWorkerIsolationAndCommit",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FNativeEconomyWorkerTest::RunTest(const FString&)
{
    FEconomyState Live;Live.Wallets.Add(FEconomyLedger::Pilot(),5000);Live.EconomyClockSeconds=61;
    const FName Deposit=TEXT("Test mine"),Ore=TEXT("Magnetite"),Hold=FEconomyRuntime::Warehouse(TEXT("Janus"),TEXT("Belt"),TEXT("Ore"));
    Live.BackgroundMineDeadlines.Add(Deposit,60);Live.BackgroundMineLastCycle.Add(Deposit,0);Live.NextBackgroundMineAt=60;
    const auto Request=[&](uint64 Revision)
    {FNativeEconomyInput In;In.State=Live;In.Revision=Revision;In.Profile=TEXT("Pilot A");In.NowUtc=1000;In.bInventoryChanged=true;In.Mines.Add({Deposit,TEXT("Janus"),TEXT("Belt"),Ore,20,1,100});return In;};
    FEvent* Done=FPlatformProcess::GetSynchEventFromPool(false);ON_SCOPE_EXIT{FPlatformProcess::ReturnSynchEventToPool(Done);};
    FNativeEconomyWorker Worker;
    std::atomic<int32> DataNotifications{0};const auto Handle=FSimDataEvents::Changed().AddLambda([&](const FEconomyState&){++DataNotifications;});ON_SCOPE_EXIT{FSimDataEvents::Changed().Remove(Handle);};
    TestTrue(TEXT("Dedicated worker accepts native snapshot"),Worker.Submit(Request(3),[Done](){Done->Trigger();}));
    if(!TestTrue(TEXT("Native calculation completes"),Done->Wait(10000)))return false;
    auto Output=Worker.TakeResult();if(!TestNotNull(TEXT("Result mailbox owns state"),Output.Get()))return false;
    TestTrue(TEXT("Economy arithmetic ran off the game thread"),Output->bOffGameThread);
    TestEqual(TEXT("Worker never broadcasts UI delegates"),DataNotifications.load(),0);
    TestEqual(TEXT("Worker cannot mutate live cargo"),FEconomyLedger::Quantity(Live,Hold,Ore),0.0);
    TestFalse(TEXT("Changed live revision rejects stale output"),FNativeEconomyWorker::Commit(MoveTemp(*Output),Live,4,TEXT("Pilot A")));
    TestEqual(TEXT("Rejection preserves pilot credits"),Live.Wallets.FindRef(FEconomyLedger::Pilot()),int64(5000));
    TestTrue(TEXT("Retry accepts fresh snapshot"),Worker.Submit(Request(4),[Done](){Done->Trigger();}));if(!Done->Wait(10000))return false;
    Output=Worker.TakeResult();if(!Output)return false;
    TestFalse(TEXT("Different pilot cannot receive another economy"),FNativeEconomyWorker::Commit(MoveTemp(*Output),Live,4,TEXT("Pilot B")));
    FMiningLease LiveLease;LiveLease.Claimant=TEXT("Live shuttle");LiveLease.ExpiresAt=100;Live.MiningLeases.Add(TEXT("Live scan claim"),LiveLease);
    TestTrue(TEXT("Validated result commits on game thread"),FNativeEconomyWorker::Commit(MoveTemp(*Output),Live,4,TEXT("Pilot A")));
    TestTrue(TEXT("Worker cannot overwrite a live scanning claim"),Live.MiningLeases.Contains(TEXT("Live scan claim")));
    TestEqual(TEXT("Completed mining credits precisely twenty tonnes"),FEconomyLedger::Quantity(Live,Hold,Ore),20.0);
    TestEqual(TEXT("Committed result publishes one UI invalidation"),DataNotifications.load(),1);
    TestFalse(TEXT("The same result cannot commit twice"),FNativeEconomyWorker::Commit(MoveTemp(*Output),Live,4,TEXT("Pilot A")));
    TestEqual(TEXT("Duplicate result cannot duplicate ore"),FEconomyLedger::Quantity(Live,Hold,Ore),20.0);
    FString CatalogError;if(!TestTrue(TEXT("Actual production catalog loads"),SSWProductionCatalog::Reload(CatalogError)))return false;
    const FName FactoryId=TEXT("Janek_Orbital_Materials_Works");const auto* Site=SSWProductionCatalog::Facilities().Find(FactoryId);
    if(!TestNotNull(TEXT("Janek refinery exists"),Site))return false;
    const auto* Recipe=SSWProductionCatalog::Recipes().Find(Site->Recipe);if(!TestNotNull(TEXT("Janek recipe exists"),Recipe))return false;
    const FName RefineryInput=FEconomyRuntime::Warehouse(Site->System,Site->Facility,TEXT("Input")),RefineryOutput=FEconomyRuntime::Warehouse(Site->System,Site->Facility,TEXT("Output"));
    FEconomyRuntime::EnsureWarehouse(Live,RefineryInput,TEXT("Janus"),Site->CapacityTonnes);
    TestTrue(TEXT("Real mined stock supplies refinery input"),FEconomyLedger::Transfer(Live,FEconomyRuntime::IndustryOwner(),Hold,RefineryInput,Ore,Recipe->InputTonnes));
    const auto RefineryRequest=[&](uint64 Revision)
    {
        FNativeEconomyInput In;In.State=Live;In.Revision=Revision;In.Profile=TEXT("Pilot A");In.NowUtc=1000;In.bInventoryChanged=true;
        In.Facilities.Add(FactoryId,*Site);auto Batch=*Recipe;Batch.NativeMineralKeys=ManufacturingAI::MineralKeys(Batch);In.Recipes.Add(Site->Recipe,MoveTemp(Batch));return In;
    };
    int32 PhaseNotifications=0;bool PhasesOnGameThread=true;
    const auto PhaseHandle=ProductionEvents::Changed().AddLambda([&](FName,FGuid,EProductionEvent,const FText&){++PhaseNotifications;PhasesOnGameThread&=IsInGameThread();});ON_SCOPE_EXIT{ProductionEvents::Changed().Remove(PhaseHandle);};
    const int32 BeforeWork=DataNotifications.load();
    TestTrue(TEXT("Actual refinery dispatches to worker"),Worker.Submit(RefineryRequest(6),[Done](){Done->Trigger();}));if(!Done->Wait(10000))return false;
    Output=Worker.TakeResult();if(!Output)return false;
    TestEqual(TEXT("Worker refinery cannot notify UI before commit"),DataNotifications.load(),BeforeWork);
    TestEqual(TEXT("Worker lifecycle events are buffered"),PhaseNotifications,0);
    TestTrue(TEXT("Start result commits"),FNativeEconomyWorker::Commit(MoveTemp(*Output),Live,6,TEXT("Pilot A")));
    if(!TestTrue(TEXT("Input created a scheduled job"),!Live.Jobs.IsEmpty()))return false;
    TestEqual(TEXT("Start event delivered once"),PhaseNotifications,1);
    TestEqual(TEXT("No concentrate before completion"),FEconomyLedger::Quantity(Live,RefineryOutput,DataTableUtils::MakeValidName(Recipe->CargoItem)),0.0);
    Live.EconomyClockSeconds=Live.Jobs[0].CompletionAt+.01;
    TestTrue(TEXT("Completion dispatches"),Worker.Submit(RefineryRequest(7),[Done](){Done->Trigger();}));if(!Done->Wait(10000))return false;
    Output=Worker.TakeResult();if(!Output)return false;
    TestTrue(TEXT("Completed batch commits"),FNativeEconomyWorker::Commit(MoveTemp(*Output),Live,7,TEXT("Pilot A")));
    const double Expected=Recipe->OutputQuantity>0?Recipe->OutputQuantity:Recipe->OutputTonnes;
    TestTrue(TEXT("Worker produces exact authored concentrate quantity"),FMath::IsNearlyEqual(FEconomyLedger::Quantity(Live,RefineryOutput,DataTableUtils::MakeValidName(Recipe->CargoItem)),Expected));
    TestEqual(TEXT("Start and completion events arrive exactly once"),PhaseNotifications,2);
    TestTrue(TEXT("All lifecycle callbacks execute on game thread"),PhasesOnGameThread);
    // Destruction waits for its owned native work; no actor or UI callback is required to finish.
    {FNativeEconomyWorker Closing;TestTrue(TEXT("Shutdown test queues work"),Closing.Submit(Request(5),[](){}));}
    return true;
}
#endif
