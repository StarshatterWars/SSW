#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MissionArcRules.h"
#include "AgentAI.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShortMissionArcTest,"Starshatter.Agents.ShortArcProgressionAndPersistence",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FShortMissionArcTest::RunTest(const FString&)
{
    FString Json; FMissionArcCatalog Catalog; FText Error;
    if(!TestTrue(TEXT("Authored arc JSON is staged"),FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Agents/mission_arcs.json"))))) return false;
    if(!TestTrue(TEXT("Authored arcs validate"),Catalog.LoadJson(Json,Error)))return false;
    TestEqual(TEXT("Six regional short arcs"),Catalog.GetArcs().Num(),6);
    constexpr int32 First=910017, Second=910015, Third=910016;
    const FName Pilot(TEXT("Arc Test Pilot")); AgentAI Agent;
    TestTrue(TEXT("T1 retains its own progression"),Agent.ArcRequirement(Pilot,101).IsEmpty());
    const int32 Future[]={Second,Third};
    TestNull(TEXT("Future steps cannot be offered"),Agent.Request(Pilot,Future,TEXT("JanekTraffic")));
    TestFalse(TEXT("Locked step explains its prerequisite"),Agent.ArcRequirement(Pilot,Second).IsEmpty());
    const int32 Start[]={First};
    TestNotNull(TEXT("First step can be offered"),Agent.Request(Pilot,Start,TEXT("JanekFleet")));
    TestTrue(TEXT("First step can be accepted"),Agent.Accept(Pilot,First));
    TestFalse(TEXT("Accept is idempotent against duplicate queues"),Agent.Accept(Pilot,First));
    Agent.Start(Pilot,First); Agent.Resolve(Pilot,First,false);
    TestFalse(TEXT("Failure does not unlock the next step"),Agent.ArcRequirement(Pilot,Second).IsEmpty());
    Agent.Request(Pilot,Start,TEXT("JanekFleet"),true); Agent.Accept(Pilot,First); Agent.Start(Pilot,First);
    TestTrue(TEXT("Success records the first step"),Agent.Resolve(Pilot,First,true));
    TestFalse(TEXT("Duplicate results cannot settle twice"),Agent.Resolve(Pilot,First,true));
    TestTrue(TEXT("Second step unlocks after successful result"),Agent.ArcRequirement(Pilot,Second).IsEmpty());
    TestFalse(TEXT("Third step still requires second"),Agent.ArcRequirement(Pilot,Third).IsEmpty());
    AgentAI Restored;
    TestTrue(TEXT("Arc progress uses the existing saved journal"),Restored.RestoreJournal(Agent.SaveJournal()));
    TestTrue(TEXT("Previous success survives restart"),Restored.IsCompleted(Pilot,First));
    TestTrue(TEXT("Next step stays unlocked after restart"),Restored.ArcRequirement(Pilot,Second).IsEmpty());
    TestNotNull(TEXT("Next agent can offer the unlocked step"),Restored.Request(Pilot,Future,TEXT("JanekTraffic")));
    Restored.Accept(Pilot,Second); Restored.Start(Pilot,Second); Restored.Interrupt(Pilot,Second);
    TestTrue(TEXT("Interruption requeues the same step"),Restored.GetQueue(Pilot)[0].State==AgentAI::EState::Accepted);
    Restored.Start(Pilot,Second); Restored.Resolve(Pilot,Second,true);
    TestTrue(TEXT("Final step unlocks in order despite nonascending IDs"),Restored.ArcRequirement(Pilot,Third).IsEmpty());
    const int32 Finish[]={Third};
    Restored.Request(Pilot,Finish,TEXT("JanekMaterials")); Restored.Accept(Pilot,Third); Restored.Start(Pilot,Third); Restored.Resolve(Pilot,Third,true);
    const auto Done=Catalog.Evaluate(Third,[&](int32 Id){return Restored.IsCompleted(Pilot,Id);});
    TestEqual(TEXT("All three steps complete"),Done.Completed,3);
    TestEqual(TEXT("Complete arc has no next step"),Done.Current,INDEX_NONE);
    TestNull(TEXT("Completed arcs do not offer repeat rewards"),Restored.Request(Pilot,Start,TEXT("JanekFleet"),true));
    AgentAI Migrated;
    const auto History=[](int32 Id){return Id==First;};
    TestNotNull(TEXT("Preexisting saved mission history unlocks the next offer"),Migrated.Request(Pilot,Future,TEXT("JanekTraffic"),true,History));
    TestTrue(TEXT("History-only prerequisite remains valid on acceptance"),Migrated.Accept(Pilot,Second,History));
    TestTrue(TEXT("Description includes agent handoff and current step"),Migrated.ArcStatus(Pilot,Second,History).ToString().Contains(TEXT("CURRENT:")));
    const int32 Before=Catalog.GetArcs().Num();
    FString Duplicate=Json;
    Duplicate.ReplaceInline(TEXT("910015"),TEXT("910017"));
    TestFalse(TEXT("Duplicate mission in arc data rejected atomically"),Catalog.LoadJson(Duplicate,Error));
    TestEqual(TEXT("Bad catalog cannot erase valid arcs"),Catalog.GetArcs().Num(),Before);
    return true;
}
#endif
