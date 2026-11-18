#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "StarshatterGameDataSubsystem.h"
#include "Engine/DataTable.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryMissionMergeTest,"Starshatter.Agents.StoryMissionRuntimeMerge",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStoryMissionMergeTest::RunTest(const FString&)
{
    auto* Source=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_StoryMissions.DT_StoryMissions"));
    auto* Campaigns=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_Campaign.DT_Campaign"));
    if (!TestNotNull(TEXT("Story catalog"),Source) || !TestNotNull(TEXT("Campaign catalog"),Campaigns)) return false;
    int32 Total=0;
    for (const auto& Pair:Campaigns->GetRowMap())
    {
        FS_Campaign Row=*reinterpret_cast<const FS_Campaign*>(Pair.Value);
        const auto Scenes=Row.Missions;
        const int32 Added=UStarshatterGameDataSubsystem::MergeStoryMissionRows(Row,*Source); Total+=Added;
        const int32 Count=Row.StoryMissions.Num();
        UStarshatterGameDataSubsystem::MergeStoryMissionRows(Row,*Source);
        TestEqual(TEXT("Repeated binding cannot duplicate stories"),Row.StoryMissions.Num(),Count);
        TestEqual(TEXT("Video/cinematic records retained"),Row.Missions.Num(),Scenes.Num());
        for (const auto& Story:Row.StoryMissions)
        {
            TestEqual(TEXT("Story remains with its own operation"),Story.StoryCampaignId,Row.Index+1);
            TestTrue(TEXT("Story links to authored narrative event"),Scenes.ContainsByPredicate([&Story](const auto& Scene){return Scene.Scene==Story.StoryUnlockScene;}));
        }
    }
    TestEqual(TEXT("All28 authored playable story assignments bind"),Total,28);
    return true;
}
#endif
