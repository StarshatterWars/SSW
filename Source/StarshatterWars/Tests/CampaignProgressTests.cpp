#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "CampaignSave.h"
#include "Campaign.h"
#include "Mission.h"
#include "MissionInfo.h"
#include "CombatEvent.h"
#include "CampaignPresentationRules.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignProgressTest, "Starshatter.Campaign.ProgressRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCampaignProgressTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UCampaignSave> Save(NewObject<UCampaignSave>());
    Save->CampaignIndex=2; Save->CampaignRowName=TEXT("HighlandFixture");
    Save->InitializeCampaignClock(123456);
    TestFalse(TEXT("Empty scene cannot consume progress"),Save->RecordPlayedScene(TEXT("  ")));
    TestTrue(TEXT("Viewed scene recorded"),Save->RecordPlayedScene(TEXT(" 01-News-Start ")));
    TestFalse(TEXT("Repeated skip/completion is idempotent"),Save->RecordPlayedScene(TEXT("01-news-start")));
    TestTrue(TEXT("Second scene remains independent"),Save->RecordPlayedScene(TEXT("Victory")));
    TestFalse(TEXT("Invalid assignment rejected"),Save->RecordCompletedMission(0));
    TestTrue(TEXT("Completed assignment recorded"),Save->RecordCompletedMission(11));
    TestFalse(TEXT("Repeated debrief does not duplicate completion"),Save->RecordCompletedMission(11));
    TArray<uint8> Bytes;
    TestTrue(TEXT("Serialize full Unreal SaveGame"),UGameplayStatics::SaveGameToMemory(Save.Get(),Bytes));
    TStrongObjectPtr<UCampaignSave> Loaded(Cast<UCampaignSave>(UGameplayStatics::LoadGameFromMemory(Bytes)));
    if(!TestNotNull(TEXT("Load progress SaveGame"),Loaded.Get())) return false;
    TestTrue(TEXT("Mission history survives reload"),Loaded->CompletedMissionIds==Save->CompletedMissionIds);
    TestTrue(TEXT("Viewed scenes survive reload"),Loaded->HasPlayedScene(TEXT("01-News-Start")));
    TestEqual(TEXT("Existing campaign clock preserved"),Loaded->CampaignStartUniverseSeconds,uint64(123456));

    Campaign Story(2,"Progress fixture",true);
    auto* Viewed=new CombatEvent(&Story,ECombatEventType::NONE,0,0,ECombatEventSource::NONE,"");
    Viewed->SetSceneFile("01-News-Start");Story.GetEvents().append(Viewed);
    auto* Pending=new CombatEvent(&Story,ECombatEventType::NONE,0,0,ECombatEventSource::NONE,"");
    Pending->SetSceneFile("02-Coup-Failure");Story.GetEvents().append(Pending);
    auto* OneShot=new MissionInfo();OneShot->id=11;OneShot->exec_once=1;Story.GetMissionList().append(OneShot);
    auto* Repeatable=new MissionInfo();Repeatable->id=12;Repeatable->exec_once=0;Story.GetMissionList().append(Repeatable);
    Loaded->RecordCompletedMission(12);Loaded->RestoreProgress(&Story);
    TestTrue(TEXT("Reload restores viewed scene event"),Viewed->Visited());
    TestFalse(TEXT("Unplayed story scene remains pending"),Pending->Visited());
    TestEqual(TEXT("Completed one-shot stays consumed"),OneShot->exec_once,-1);
    TestEqual(TEXT("Repeatable assignment retains legacy replay behavior"),Repeatable->exec_once,0);
    Viewed->SetVisited(false);Loaded->CampaignIndex=3;Loaded->RestoreProgress(&Story);
    TestFalse(TEXT("Other campaign history cannot consume this scene"),Viewed->Visited());
    TStrongObjectPtr<UCampaignSave> Fresh(NewObject<UCampaignSave>());
    TestTrue(TEXT("Restart/new-save defaults clear viewed and mission history"),Fresh->PlayedSceneNames.IsEmpty()&&Fresh->CompletedMissionIds.IsEmpty());

    Save->CampaignIndex=2;
    Mission Assignment(21);Story.SetMission(&Assignment);
    TestFalse(TEXT("Abort/incomplete commit never records completion"),Save->RecordCompletedMission(&Story,&Assignment));
    Assignment.SetComplete(true);
    TestTrue(TEXT("Completed owned assignment records completion"),Save->RecordCompletedMission(&Story,&Assignment));
    Mission Standalone(22);Standalone.SetComplete(true);
    TestFalse(TEXT("Standalone mission cannot write active campaign progress"),Save->RecordCompletedMission(&Story,&Standalone));
    Story.SetMission(nullptr);
    FString LiveSaveFile;
    if(FParse::Value(FCommandLine::Get(),TEXT("LiveCampaignSave="),LiveSaveFile))
    {
        TArray<uint8> LiveBytes;
        TestTrue(TEXT("Read actual isolated live campaign save"),FFileHelper::LoadFileToArray(LiveBytes,*LiveSaveFile));
        TStrongObjectPtr<UCampaignSave> Live(Cast<UCampaignSave>(UGameplayStatics::LoadGameFromMemory(LiveBytes)));
        if(TestNotNull(TEXT("Actual live campaign save reloads through Unreal"),Live.Get()))
        {
            TestEqual(TEXT("Live save retains campaign identity"),Live->CampaignIndex,2);
            TestTrue(TEXT("Naturally completed cutscene survives live save reload"),Live->HasPlayedScene(TEXT("01-News-Start")));
            AddInfo(TEXT("Verified persisted playback history from the real isolated Campaign 2 run."));
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignUnlockTest, "Starshatter.Campaign.SequentialUnlocks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCampaignUnlockTest::RunTest(const FString& Parameters)
{
    FS_PlayerGameInfo Player;
    using CampaignPresentationRules::IsCampaignUnlocked;
    TestTrue(TEXT("Training available"),IsCampaignUnlocked(1,Player));
    TestTrue(TEXT("Campaign 2 available"),IsCampaignUnlocked(2,Player));
    for(int32 Id=3;Id<=5;++Id) TestFalse(TEXT("Later campaigns initially locked"),IsCampaignUnlocked(Id,Player));
    Player.SetCampaignComplete(2,true);
    TestTrue(TEXT("Finishing 2 unlocks 3"),IsCampaignUnlocked(3,Player));
    TestFalse(TEXT("Finishing 2 does not unlock 4"),IsCampaignUnlocked(4,Player));
    TestFalse(TEXT("Finishing 2 does not unlock 5"),IsCampaignUnlocked(5,Player));
    Player.SetCampaignComplete(3,true);
    TestTrue(TEXT("Finishing 3 unlocks 4"),IsCampaignUnlocked(4,Player));
    TestFalse(TEXT("5 still requires 4"),IsCampaignUnlocked(5,Player));
    Player.SetCampaignComplete(4,true);
    TestTrue(TEXT("Finishing 4 unlocks 5"),IsCampaignUnlocked(5,Player));
    Player.SetCampaignComplete(2,false);
    TestFalse(TEXT("Non-contiguous completion cannot bypass campaign order"),IsCampaignUnlocked(5,Player));
    TestFalse(TEXT("Invalid campaign rejected"),IsCampaignUnlocked(0,Player));
    return true;
}
#endif
