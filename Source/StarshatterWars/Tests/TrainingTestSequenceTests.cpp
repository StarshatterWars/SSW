#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "TrainingTestRules.h"
#include "AgentMissionRules.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrainingTestSequence, "Starshatter.Mission.TestTrainingSequence",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FTrainingTestSequence::RunTest(const FString&)
{
    const FName Pilot(TEXT("Test cadet")), Instructor(TEXT("JanekFlightTraining"));
    AgentAI Session;
    for (int32 Id : {101,102,103,104,105,106,107})
    {
        const auto Available = FTrainingTestRules::Eligible(Session, Pilot);
        if (!TestEqual(TEXT("Exactly one next lesson offered"), Available.Num(), 1)) return false;
        TestEqual(TEXT("Lessons progress in authored T1-T7 order"), Available[0], Id);
        const auto* Offer = Session.Request(Pilot, Available, Instructor, true);
        if (!TestNotNull(TEXT("Instructor offers next lesson"), Offer)) return false;
        TestTrue(TEXT("A declined test lesson can be requested again"), Session.Decline(Pilot, Id));
        TestNotNull(TEXT("Declining never skips a lesson"), Session.Request(Pilot, Available, Instructor, true));
        TestTrue(TEXT("Accept lesson"), Session.Accept(Pilot, Id));
        TestTrue(TEXT("No later lesson can be queued early"), FTrainingTestRules::Eligible(Session, Pilot).IsEmpty());
        TestFalse(TEXT("Cannot launch out of sequence"), Session.Start(Pilot, Id+1));
        TestTrue(TEXT("Launch accepted lesson"), Session.Start(Pilot, Id));
        TestTrue(TEXT("Failed lesson resolves"), Session.Resolve(Pilot, Id, false));
        TestEqual(TEXT("Failure does not advance curriculum"), FTrainingTestRules::Eligible(Session, Pilot)[0], Id);
        TestNotNull(TEXT("Failed lesson can be retried"), Session.Request(Pilot, Available, Instructor, true));
        TestTrue(TEXT("Accept retry"), Session.Accept(Pilot, Id));
        TestTrue(TEXT("Start retry"), Session.Start(Pilot, Id));
        TestTrue(TEXT("Success advances curriculum"), Session.Resolve(Pilot, Id, true));
    }
    TestTrue(TEXT("Seven successes finish Operation Live Fire"), FTrainingTestRules::Eligible(Session, Pilot).IsEmpty());
    for (int32 Id : {104,105,106,107})
    {
        const int32 Rank = FTrainingTestRules::RequiredRank(Id);
        TestFalse(TEXT("Test mode cannot bypass rank"), FTrainingTestRules::Unlocked(Id,Rank-1,true,true));
        TestTrue(TEXT("Correct rank and skill unlock lesson"), FTrainingTestRules::Unlocked(Id,Rank,true,false));
        if (Id >= 105) TestFalse(TEXT("Rank without command skill remains locked"), FTrainingTestRules::Unlocked(Id,Rank,false,true));
    }
    int32 Campaign, Mission;
    const int32 ArcId = AgentMissionRules::Encode(2,101);
    TestTrue(TEXT("Campaign assignment identity round trips"),AgentMissionRules::Decode(ArcId,Campaign,Mission));
    TestEqual(TEXT("Campaign retained"),Campaign,2);
    TestEqual(TEXT("Authored mission identity retained"),Mission,101);
    TestNotEqual(TEXT("Other campaign same mission is distinct"),ArcId,AgentMissionRules::Encode(3,101));
    TestFalse(TEXT("Regional IDs cannot decode as campaign"),AgentMissionRules::Decode(910001,Campaign,Mission));
    TestTrue(TEXT("Local agent can offer another sector in same system"),AgentMissionRules::LocalSystem(TEXT("Janus"),TEXT("JANUS")));
    TestFalse(TEXT("Local agent cannot offer another system"),AgentMissionRules::LocalSystem(TEXT("Janus"),TEXT("Solus")));
    AgentAI FreshSession;
    TestEqual(TEXT("New testing session starts at T1"), FTrainingTestRules::Eligible(FreshSession, Pilot)[0], 101);
    return true;
}
#endif
