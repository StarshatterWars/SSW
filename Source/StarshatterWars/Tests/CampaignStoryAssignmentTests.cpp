#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "CampaignStoryRules.h"
#include "Campaign.h"
#include "MissionInfo.h"

namespace
{
    FS_CampaignMission MakeStory()
    {
        FS_CampaignMission Row;
        Row.Id=2201; Row.StoryCampaignId=2; Row.StoryUnlockScene=TEXT("07-Research-Lab");
        Row.StartTime=TEXT("01/00:00:00"); Row.MissionName=TEXT("Research Lab: Relief Corridor"); Row.Objective=TEXT("Protect the observatory");
        Row.DisplayType=EMissionDisplayType::PlayerMission; Row.Source=EMissionSource::Scripted; Row.MissionType=EMissionType::SWEEP;
        FS_MissionElement Pilot; Pilot.Design=TEXT("Falcon"); Pilot.Player=1; Pilot.Count=1;
        Pilot.Instruction.Add(FS_MissionInstruction()); Row.Element.Add(Pilot);
        return Row;
    }
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryAssignmentSeparationTest,"Starshatter.Campaign.StoryAssignmentSeparation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStoryAssignmentSeparationTest::RunTest(const FString&)
{
    auto Row=MakeStory();
    TestTrue(TEXT("Explicit playable companion accepted"),CampaignStoryRules::IsPlayable(Row,2));
    TestFalse(TEXT("Unseen scene does not unlock assignment"),CampaignStoryRules::IsUnlocked(Row,2,false));
    TestTrue(TEXT("Presented scene unlocks its companion"),CampaignStoryRules::IsUnlocked(Row,2,true));
    TestFalse(TEXT("Foreign campaign cannot claim companion"),CampaignStoryRules::IsUnlocked(Row,3,true));
    Row.Scene=TEXT("07-Research-Lab");
    TestFalse(TEXT("Original cinematic remains cinematic"),CampaignStoryRules::IsPlayable(Row,2));
    TestEqual(TEXT("Scene key is preserved"),Row.Scene,FString(TEXT("07-Research-Lab")));
    Row.Scene.Empty(); Row.Element[0].Design=TEXT("CameraPod");
    TestFalse(TEXT("Camera-player flag does not turn scene into playable combat"),CampaignStoryRules::IsPlayable(Row,2));
    Row=MakeStory(); Row.Element[0].Instruction.Empty();
    TestFalse(TEXT("No objective instructions means no invented combat"),CampaignStoryRules::IsPlayable(Row,2));
    Row=MakeStory(); Row.StoryUnlockScene.Empty();
    TestFalse(TEXT("Unbound companion cannot bypass trigger"),CampaignStoryRules::IsPlayable(Row,2));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryAssignmentRegistryTest,"Starshatter.Campaign.StoryAssignmentRegistry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStoryAssignmentRegistryTest::RunTest(const FString&)
{
    FS_Campaign Data; Data.Index=1; Data.Name=TEXT("Story fixture"); Data.Start=TEXT("01/00:00:00");
    Data.StoryMissions.Add(MakeStory()); Data.StoryMissions.Add(MakeStory());
    auto Scene=MakeStory(); Scene.Id=207; Scene.Scene=TEXT("07-Research-Lab");
    Data.Missions.Add(Scene);
    FS_CampaignMissionList SceneList; SceneList.Start=TEXT("01/00:00:00"); SceneList.Id=207; SceneList.Name=TEXT("Research Lab cinematic");
    Data.MissionList.Add(SceneList);
    struct FStoryCampaignFixture : Campaign { using Campaign::Campaign; using Campaign::SetCampaignData; };
    FStoryCampaignFixture Runtime(2,"Story fixture",true); Runtime.SetCampaignData(&Data); Runtime.LoadFromData(Data);
    TestTrue(TEXT("Original scene remains excluded from player offerings"),Runtime.IsCutsceneMission(207));
    TestEqual(TEXT("Companion ID deduplicated"),Runtime.GetStoryMissionList().size(),1);
    TestTrue(TEXT("Companion data lookup keeps original ID"),Runtime.FindStoryMissionData(2201)!=nullptr);
    Runtime.GetMissionList().destroy();
    TestEqual(TEXT("Generated list rebuild cannot delete companion assignment"),Runtime.GetStoryMissionList().size(),1);
    TestEqual(TEXT("Companion ID unchanged"),Runtime.GetStoryMissionList()[0]->id,2201);
    Runtime.Clear();
    TestEqual(TEXT("Full campaign clear releases companion metadata"),Runtime.GetStoryMissionList().size(),0);
    return true;
}
#endif
