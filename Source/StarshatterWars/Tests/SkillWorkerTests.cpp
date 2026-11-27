#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "NativeEconomyWorker.h"
#include "SkillsSubsystem.h"
#include "HAL/PlatformProcess.h"
#include "HAL/Event.h"
namespace
{
struct FSkillWorkerFixture
{
    FS_PlayerGameInfo Pilot;FNativeSkillCommand Command;FText Why;
    TUniquePtr<FNativeEconomyWorker> Worker=MakeUnique<FNativeEconomyWorker>();
    FEvent* Done=FPlatformProcess::GetSynchEventFromPool(false);
    FSkillWorkerFixture()
    {
        FSkillTrainingRules::Duration(1);Pilot.Rank=2;Pilot.Name=TEXT("Skill pilot");Pilot.Economy.Wallets.Add(FEconomyLedger::Pilot(),1000000);
        Command.Skill=TEXT("worker_navigation");Command.Receipt=FGuid::NewGuid();Command.NowUtc=100000;Command.Rank=2;Command.Empire=TEXT("Terellian");Command.MaximumCost=1000;
        FSkillDefinition D;D.Name=Command.Skill;D.DisplayName=TEXT("Worker navigation");D.BookPrice=1000;D.AdvancedBookPrice=5000;D.MinimumRank=2;D.Empire=Command.Empire;Command.Definitions.Add(D.Name,D);
    }
    ~FSkillWorkerFixture(){Worker.Reset();FPlatformProcess::ReturnSynchEventToPool(Done);}
    TUniquePtr<FNativeEconomyResult> Run(FNativeSkillCommand C)
    {
        C.Source=Pilot.Skills;C.SourceQueue=Pilot.SkillQueue;FNativeEconomyInput I;I.State=Pilot.Economy;I.Revision=7;I.Profile=TEXT("Skills/session1");I.Skill=MakeUnique<FNativeSkillCommand>(MoveTemp(C));
        if(!Worker->Submit(MoveTemp(I),[this](){Done->Trigger();}) || !Done->Wait(10000))return nullptr;return Worker->TakeResult();
    }
    bool Apply(FNativeSkillCommand C)
    {auto R=Run(C);return R && FNativeEconomyWorker::CommitSkill(MoveTemp(*R),Pilot,7,TEXT("Skills/session1"),[]{return true;},Why);}
    void Own(int32 Level=0)
    {FSkillProgress P;P.Id=FGuid::NewGuid();P.LearnedUtc=1;P.TrainedLevel=Level;Pilot.Skills.Add(Command.Skill,P);}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSkillBookWorkerTest,"Starshatter.Domains.Skills.BookIsolationRollbackReplay",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FSkillBookWorkerTest::RunTest(const FString&)
{
    FSkillWorkerFixture F;F.Command.Action=ESkillCommand::Book;auto R=F.Run(F.Command);if(!TestNotNull(TEXT("Skill reply"),R.Get()))return false;
    TestTrue(TEXT("Skill command executes on worker"),R->bOffGameThread);TestTrue(TEXT("No early book"),F.Pilot.Skills.IsEmpty());TestEqual(TEXT("No early charge"),F.Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),1000000LL);
    TestFalse(TEXT("Generic commit refuses partial skill transaction"),FNativeEconomyWorker::Commit(MoveTemp(*R),F.Pilot.Economy,7,TEXT("Skills/session1")));
    TestFalse(TEXT("Save failure rejects purchase"),FNativeEconomyWorker::CommitSkill(MoveTemp(*R),F.Pilot,7,TEXT("Skills/session1"),[]{return false;},F.Why));
    TestTrue(TEXT("Save failure restores book"),F.Pilot.Skills.IsEmpty());TestEqual(TEXT("Save failure restores money"),F.Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),1000000LL);
    TestTrue(TEXT("Retry saves book"),F.Apply(F.Command));TestEqual(TEXT("Book is not a trained level"),FSkillTrainingRules::Level(F.Pilot.Skills.Find(F.Command.Skill)),0);
    TestEqual(TEXT("One book charge"),F.Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),999000LL);TestFalse(TEXT("Receipt replay refused"),F.Apply(F.Command));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSkillReadSetWorkerTest,"Starshatter.Domains.Skills.ProfileRevisionAndReadSet",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FSkillReadSetWorkerTest::RunTest(const FString&)
{
    FSkillWorkerFixture F;F.Command.Action=ESkillCommand::Book;auto R=F.Run(F.Command);if(!R)return false;
    TestFalse(TEXT("Wrong account rejected"),FNativeEconomyWorker::CommitSkill(MoveTemp(*R),F.Pilot,7,TEXT("Skills/session2"),[]{return true;},F.Why));
    TestFalse(TEXT("Stale revision rejected"),FNativeEconomyWorker::CommitSkill(MoveTemp(*R),F.Pilot,8,TEXT("Skills/session1"),[]{return true;},F.Why));
    F.Own();TestFalse(TEXT("Unversioned skill change protected"),FNativeEconomyWorker::CommitSkill(MoveTemp(*R),F.Pilot,7,TEXT("Skills/session1"),[]{return true;},F.Why));F.Pilot.Skills.Reset();
    F.Pilot.Rank=4;F.Pilot.Name=TEXT("Career advanced");TestTrue(TEXT("Valid result commits only its domain"),FNativeEconomyWorker::CommitSkill(MoveTemp(*R),F.Pilot,7,TEXT("Skills/session1"),[]{return true;},F.Why));
    TestEqual(TEXT("Career rank preserved"),F.Pilot.Rank,4);TestEqual(TEXT("Pilot name preserved"),F.Pilot.Name,FString(TEXT("Career advanced")));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSkillQueueWorkerTest,"Starshatter.Domains.Skills.LevelQueueAndOfflineAdvance",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FSkillQueueWorkerTest::RunTest(const FString&)
{
    FSkillWorkerFixture F;F.Own();F.Command.Action=ESkillCommand::Queue;F.Command.Target=5;TestTrue(TEXT("Queue all five standard levels"),F.Apply(F.Command));TestEqual(TEXT("Each level is a queue entry"),F.Pilot.SkillQueue.Num(),5);
    auto Advance=F.Command;Advance.Action=ESkillCommand::Advance;Advance.Receipt=FGuid::NewGuid();Advance.NowUtc=F.Pilot.SkillQueue.Last().FinishesUtc;
    TestTrue(TEXT("Offline end time completes levels"),F.Apply(Advance));TestEqual(TEXT("Level V earned"),F.Pilot.Skills.FindChecked(F.Command.Skill).TrainedLevel,5);TestTrue(TEXT("Completed queue emptied"),F.Pilot.SkillQueue.IsEmpty());
    Advance.Receipt=FGuid::NewGuid();auto R=F.Run(Advance);if(!R)return false;int32 Saves=0;TestTrue(TEXT("Idle advancement succeeds"),FNativeEconomyWorker::CommitSkill(MoveTemp(*R),F.Pilot,7,TEXT("Skills/session1"),[&](){++Saves;return true;},F.Why));TestEqual(TEXT("Idle advancement does not save"),Saves,0);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSkillAdvancedWorkerTest,"Starshatter.Domains.Skills.AdvancedRankEmpireAndMonthLimit",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FSkillAdvancedWorkerTest::RunTest(const FString&)
{
    FSkillWorkerFixture F;F.Own();F.Command.Action=ESkillCommand::Queue;F.Command.Target=6;TestFalse(TEXT("Standard book cannot queue advanced level"),F.Apply(F.Command));TestTrue(TEXT("Failed multi-level request does not partially queue"),F.Pilot.SkillQueue.IsEmpty());
    F.Pilot.Skills.FindChecked(F.Command.Skill).TrainedLevel=5;auto Buy=F.Command;Buy.Action=ESkillCommand::Advanced;Buy.MaximumCost=5000;Buy.Empire=TEXT("Marakan");TestFalse(TEXT("Empire gate enforced"),F.Apply(Buy));Buy.Empire=F.Command.Empire;Buy.Rank=1;TestFalse(TEXT("Rank gate enforced"),F.Apply(Buy));Buy.Rank=2;
    TestTrue(TEXT("Advanced book unlocks training"),F.Apply(Buy));TestEqual(TEXT("Advanced book has SSW price"),F.Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),995000LL);
    F.Command.Receipt=FGuid::NewGuid();F.Command.Target=10;TestTrue(TEXT("Advanced levels within month queued"),F.Apply(F.Command));TestTrue(TEXT("Queue is within thirty days"),F.Pilot.SkillQueue.Last().FinishesUtc<=F.Command.NowUtc+30LL*86400);
    FSkillWorkerFixture Limited;Limited.Own(5);Limited.Pilot.Skills.FindChecked(Limited.Command.Skill).bAdvancedBookOwned=true;
    FSkillTrainingEntry Occupied;Occupied.Id=FGuid::NewGuid();Occupied.Skill=TEXT("Other");Occupied.StartedUtc=Limited.Command.NowUtc;Occupied.FinishesUtc=Limited.Command.NowUtc+30LL*86400-1;Limited.Pilot.SkillQueue.Add(Occupied);
    Limited.Command.Action=ESkillCommand::Queue;Limited.Command.Target=6;TestFalse(TEXT("Queue beyond month rejected atomically"),Limited.Apply(Limited.Command));TestEqual(TEXT("Existing queue retained"),Limited.Pilot.SkillQueue.Num(),1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSkillInstantWorkerTest,"Starshatter.Domains.Skills.InstantQuoteAndCancel",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FSkillInstantWorkerTest::RunTest(const FString&)
{
    FSkillWorkerFixture F;F.Own();F.Command.Action=ESkillCommand::Queue;F.Command.Target=2;TestTrue(TEXT("Queue prepared"),F.Apply(F.Command));auto Instant=F.Command;Instant.Action=ESkillCommand::Instant;Instant.Receipt=FGuid::NewGuid();Instant.Entry=F.Pilot.SkillQueue[0].Id;Instant.MaximumCost=1;
    TestFalse(TEXT("No charge above accepted quote"),F.Apply(Instant));Instant.MaximumCost=500;TestTrue(TEXT("Instant level saves"),F.Apply(Instant));TestEqual(TEXT("Level I earned"),F.Pilot.Skills.FindChecked(F.Command.Skill).TrainedLevel,1);TestEqual(TEXT("Exact instant price"),F.Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),999500LL);
    TestFalse(TEXT("Instant request cannot replay"),F.Apply(Instant));auto Cancel=F.Command;Cancel.Action=ESkillCommand::Cancel;Cancel.Receipt=FGuid::NewGuid();Cancel.Entry=F.Pilot.SkillQueue[0].Id;Cancel.NowUtc+=100;
    TestTrue(TEXT("Cancellation saves"),F.Apply(Cancel));TestTrue(TEXT("Earned partial progress retained"),F.Pilot.Skills.FindChecked(F.Command.Skill).SkillPoints>=100);TestTrue(TEXT("Queue empty after cancellation"),F.Pilot.SkillQueue.IsEmpty());return true;
}
#endif
