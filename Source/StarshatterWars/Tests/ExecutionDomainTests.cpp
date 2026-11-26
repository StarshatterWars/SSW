#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "NativeEconomyWorker.h"
#include "NativeSimulationWorker.h"
#include "SimDataEvents.h"
#include "HAL/PlatformProcess.h"
#include "HAL/Event.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyPurchaseWorkerTest,"Starshatter.Domains.EconomyPurchaseIsolationAndPersistence",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FEconomyPurchaseWorkerTest::RunTest(const FString&)
{
    FS_PlayerGameInfo Live;Live.Economy.Wallets.Add(FEconomyLedger::Pilot(),5000);Live.Economy.bLegacyWalletMigrated=true;Live.Name=TEXT("Untouched pilot");Live.Rank=3;
    const FGuid Receipt=FGuid::NewGuid();
    const auto Request=[&](uint64 Revision)
    {
        FNativeEconomyInput In;In.State=Live.Economy;In.Revision=Revision;In.Profile=TEXT("Pilot/epoch1");In.Purchase=MakeUnique<FNativeStorePurchase>();
        auto& C=*In.Purchase;C.Offer.Id=TEXT("Missile/Test");C.Offer.Category=EShipStoreCategory::Missiles;C.Offer.Price=100;C.Quantity=3;C.ExpectedTotal=300;C.Dock=TEXT("Carrier / Orion");C.Receipt=Receipt;
        C.Stored=Live.StoredShipEquipment;C.Skins=Live.OwnedShipSkins;return In;
    };
    FNativeEconomyWorker Worker;FEvent* Done=FPlatformProcess::GetSynchEventFromPool(false);ON_SCOPE_EXIT{FPlatformProcess::ReturnSynchEventToPool(Done);};
    int32 Notifications=0;bool GameThreadOnly=true;
    const auto H=FSimDataEvents::Changed().AddLambda([&](const FEconomyState& E){if(&E==&Live.Economy){++Notifications;GameThreadOnly&=IsInGameThread();}});ON_SCOPE_EXIT{FSimDataEvents::Changed().Remove(H);};
    FText Why;
    TestTrue(TEXT("Economy accepts a copied purchase command"),Worker.Submit(Request(4),[Done](){Done->Trigger();}));if(!Done->Wait(10000))return false;
    auto Out=Worker.TakeResult();if(!TestNotNull(TEXT("Purchase result exists"),Out.Get()))return false;
    TestTrue(TEXT("Purchase executes on economy worker"),Out->bOffGameThread);TestTrue(TEXT("Native purchase succeeds"),Out->Purchase && Out->Purchase->bSucceeded);
    TestEqual(TEXT("Worker cannot spend live wallet"),Live.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),5000LL);TestEqual(TEXT("Worker cannot notify UI"),Notifications,0);
    TestFalse(TEXT("Revision race cannot overwrite a new transaction"),FNativeEconomyWorker::CommitPurchase(MoveTemp(*Out),Live,5,TEXT("Pilot/epoch1"),[]{return true;},Why));
    TestFalse(TEXT("Profile/load fence rejects old career"),FNativeEconomyWorker::CommitPurchase(MoveTemp(*Out),Live,4,TEXT("Other pilot/epoch1"),[]{return true;},Why));
    auto& Concurrent=Live.StoredShipEquipment.AddDefaulted_GetRef();Concurrent.CatalogId=TEXT("Missile/Concurrent");Concurrent.Quantity=2;
    TestFalse(TEXT("Loadout store changes cannot be overwritten even without a ledger revision"),FNativeEconomyWorker::CommitPurchase(MoveTemp(*Out),Live,4,TEXT("Pilot/epoch1"),[]{return true;},Why));
    TestEqual(TEXT("Concurrent stores retained"),Live.StoredShipEquipment[0].Quantity,2);Live.StoredShipEquipment.Reset();
    Live.OwnedShipSkins.Add(TEXT("Concurrent license"));
    TestFalse(TEXT("Concurrent licenses are retained"),FNativeEconomyWorker::CommitPurchase(MoveTemp(*Out),Live,4,TEXT("Pilot/epoch1"),[]{return true;},Why));Live.OwnedShipSkins.Reset();
    TestFalse(TEXT("Failed save rolls purchase back"),FNativeEconomyWorker::CommitPurchase(MoveTemp(*Out),Live,4,TEXT("Pilot/epoch1"),[]{return false;},Why));
    TestEqual(TEXT("Failed persistence retains wallet"),Live.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),5000LL);TestTrue(TEXT("Failed persistence retains stores"),Live.StoredShipEquipment.IsEmpty());
    TestTrue(TEXT("Retry same receipt after rollback"),Worker.Submit(Request(6),[Done](){Done->Trigger();}));if(!Done->Wait(10000))return false;Out=Worker.TakeResult();if(!Out)return false;
    TestTrue(TEXT("Validated purchase commits atomically"),FNativeEconomyWorker::CommitPurchase(MoveTemp(*Out),Live,6,TEXT("Pilot/epoch1"),[]{return true;},Why));
    TestEqual(TEXT("Exact cost, no duplicate charge"),Live.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),4700LL);TestEqual(TEXT("Three missiles delivered to issuing dock"),ShipStoreAI::Stored(Live,TEXT("Missile/Test"),TEXT("Carrier / Orion")),3);
    TestEqual(TEXT("Unrelated player state is untouched"),Live.Rank,3);TestEqual(TEXT("Pilot identity is untouched"),Live.Name,FString(TEXT("Untouched pilot")));TestTrue(TEXT("All notifications occur on game thread"),GameThreadOnly);
    TestFalse(TEXT("Result cannot be applied twice"),FNativeEconomyWorker::CommitPurchase(MoveTemp(*Out),Live,6,TEXT("Pilot/epoch1"),[]{return true;},Why));
    TestTrue(TEXT("Repeated receipt calculates a refusal"),Worker.Submit(Request(7),[Done](){Done->Trigger();}));if(!Done->Wait(10000))return false;Out=Worker.TakeResult();TestTrue(TEXT("Worker does not duplicate persistent receipt"),Out && Out->Purchase && !Out->Purchase->bSucceeded);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimulationDomainWorkerTest,"Starshatter.Domains.SimulationBroadPhaseParityAndEpoch",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FSimulationDomainWorkerTest::RunTest(const FString&)
{
    FRandomStream Random(82731);FNativeCollisionInput Input;Input.Epoch=7;Input.Sequence=11;
    for(int32 I=0;I<512;++I)Input.Hulls.Add({FVector(Random.FRandRange(-5000.f,5000.f),Random.FRandRange(-5000.f,5000.f),Random.FRandRange(-5000.f,5000.f)),Random.FRandRange(1.f,500.f)});
    for(int32 I=0;I<128;++I)Input.Queries.Add({uint32(I+1),FVector(-7000,Random.FRandRange(-5000.f,5000.f),Random.FRandRange(-5000.f,5000.f)),FVector(7000,Random.FRandRange(-5000.f,5000.f),Random.FRandRange(-5000.f,5000.f))});
    Input.Queries.Add({129,FVector::ZeroVector,FVector::ZeroVector});
    const auto Hulls=Input.Hulls;const auto Queries=Input.Queries;
    FNativeSimulationWorker Worker;TestTrue(TEXT("Large ballistic workload qualifies for dedicated worker"),FNativeSimulationWorker::WorthDispatching(Hulls.Num(),Queries.Num()));
    TestFalse(TEXT("Tiny scenes avoid worker overhead"),FNativeSimulationWorker::WorthDispatching(10,2));
    TestTrue(TEXT("Simulation accepts owned geometry snapshot"),Worker.Submit(MoveTemp(Input)));
    FNativeCollisionInput Competing;Competing.Epoch=7;Competing.Sequence=12;TestFalse(TEXT("One bounded in-flight batch prevents overlap"),Worker.Submit(MoveTemp(Competing)));
    auto Out=Worker.Finish();if(!TestNotNull(TEXT("Frame barrier returns snapshot"),Out.Get()))return false;
    TestTrue(TEXT("Simulation arithmetic ran off game thread"),Out->bOffGameThread);TestTrue(TEXT("Matching epoch and sequence accepted"),FNativeSimulationWorker::Matches(*Out,7,11));
    TestFalse(TEXT("Old mission epoch cannot apply"),FNativeSimulationWorker::Matches(*Out,8,11));TestFalse(TEXT("Different frame cannot apply"),FNativeSimulationWorker::Matches(*Out,7,12));
    TestEqual(TEXT("One candidate mask per query"),Out->Candidates.Num(),Queries.Num());
    for(int32 I=0;I<Queries.Num();++I){TArray<uint8> Serial;SSWBoltCollision::FindCandidates(Hulls,Queries[I].Start,Queries[I].End,Serial,false);TestTrue(TEXT("Every broad-phase candidate matches legacy serial calculation"),Serial==Out->Candidates[I]);}
    TestNull(TEXT("Mailbox is consumed once"),Worker.Finish().Get());
    // Shutdown joins its own native work without waiting for a UI callback.
    {FNativeSimulationWorker Closing;FNativeCollisionInput Last;Last.Epoch=8;Last.Sequence=1;Last.Hulls=Hulls;Last.Queries=Queries;TestTrue(TEXT("Teardown test submits native work"),Closing.Submit(MoveTemp(Last)));}
    return true;
}
#endif
