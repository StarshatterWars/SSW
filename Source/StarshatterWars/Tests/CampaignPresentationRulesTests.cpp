#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "CampaignPresentationRules.h"
#include "FighterActor.h"
#include "Components/StaticMeshComponent.h"
#include "LandingGear.h"
#include "MissionSceneIsolation.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignPresentationRulesTest, "Starshatter.Campaign.PresentationRules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCampaignPresentationRulesTest::RunTest(const FString& Parameters)
{
    using namespace CampaignPresentationRules;
    FS_PlayerGameInfo PilotFixture;
    FS_PlayerGameInfo Graduate;Graduate.Campaign=1;
    for(int32 Id=101;Id<=103;++Id)Graduate.MarkMissionCompleted(1,Id);
    TestFalse(TEXT("T3 alone does not show the T4 graduation handoff"),NeedsTrainingHandoff(Graduate,[](int32){return false;}));
    Graduate.MarkMissionCompleted(1,104);
    TestTrue(TEXT("All four initial lessons offer the Campaign 2 handoff"),NeedsTrainingHandoff(Graduate,[](int32){return false;}));
    Graduate.Campaign=2;
    TestTrue(TEXT("Graduate restored to Janek can recover even after the pending flag was cleared"),NeedsFleetReturnRecovery(Graduate,true,true));
    TestFalse(TEXT("Recovery does not interrupt an active mission"),NeedsFleetReturnRecovery(Graduate,false,true));
    TestFalse(TEXT("Recovery does not gate normal campaign travel away from Janek"),NeedsFleetReturnRecovery(Graduate,true,false));
    Graduate.PendingFleetDeploymentCampaign=2;
    TestTrue(TEXT("Accepted deployment remains pending until arrival"),HasPendingFleetDeployment(Graduate,2));
    Graduate.PendingFleetDeploymentCampaign=0;
    TestFalse(TEXT("Leaving Meridian after deployment does not block later missions"),HasPendingFleetDeployment(Graduate,2));
    Graduate.PendingFleetDeploymentCampaign=2;Graduate.Campaign=3;
    TestFalse(TEXT("Old deployment orders cannot gate a different campaign"),HasPendingFleetDeployment(Graduate,3));
    Graduate.Campaign=2;
    TestFalse(TEXT("Accepted Campaign 2 dismisses the training referral"),NeedsTrainingHandoff(Graduate,[](int32){return true;}));
    TestTrue(TEXT("Background WorldScape planets are isolated from the live mission"),MissionSceneIsolation::IsWorldSurface(GetDefault<AWorldScapeRoot>()));
    TestTrue(TEXT("Background native terrain is isolated from the live mission"),MissionSceneIsolation::IsWorldSurface(GetDefault<ANativeTerrainActor>()));
    TestFalse(TEXT("Ordinary actors are not hidden by the planet isolation policy"),MissionSceneIsolation::IsWorldSurface(GetDefault<AActor>()));
    TestFalse(TEXT("New Cadet cannot select a campaign or standalone missions"), CanSelectCampaign(PilotFixture));
    PilotFixture.bStartedAsEnsign = true;
    TestTrue(TEXT("First-run Ensign can select a campaign and standalone missions"), CanSelectCampaign(PilotFixture));
    TestFalse(TEXT("First-run Ensign still needs Flight School for the campaign mission tab"), HasPassedFlightSchool(PilotFixture));
    PilotFixture.bStartedAsEnsign = false;
    PilotFixture.MarkMissionCompleted(1, 102);
    TestFalse(TEXT("Target Practice does not unlock campaigns"), CanSelectCampaign(PilotFixture));
    PilotFixture.MarkMissionCompleted(1, 103);
    TestTrue(TEXT("Saved Flight School completion unlocks the campaign mission tab"), HasPassedFlightSchool(PilotFixture));
    TestTrue(TEXT("Saved Flight School completion unlocks campaign selection"), CanSelectCampaign(PilotFixture));
    TestTrue(TEXT("Unvisited intro due now can play"), IsSceneDue(false,true,0,0));
    TestFalse(TEXT("Future story scene cannot start on campaign entry"), IsSceneDue(false,true,500,100));
    TestFalse(TEXT("Visited intro cannot replay"), IsSceneDue(true,true,0,100));
    TestFalse(TEXT("Non-scene event cannot start a cutscene"), IsSceneDue(false,false,0,100));
    FS_CampaignMissionList Row; Row.Id=201; Row.DisplayType=EMissionDisplayType::PlayerMission;
    FS_CampaignMission Scene; Scene.Id=201; Scene.Scene=TEXT("01-News-Start");
    TArray<FS_CampaignMission> Authored={Scene};
    TestTrue(TEXT("Scene metadata overrides misleading playable list tag"),
        MissionDisplayType(Row,Authored)==EMissionDisplayType::CutsceneOnly);
    Row.Id=1;
    TestTrue(TEXT("Ordinary player assignment retained"),
        MissionDisplayType(Row,Authored)==EMissionDisplayType::PlayerMission);
    AFighterActor* Fighter=GetMutableDefault<AFighterActor>();
    for(UStaticMeshComponent* Mesh:{Fighter->LeftGear.Get(),Fighter->RightGear.Get(),Fighter->NoseGear.Get()})
    {
        if(TestNotNull(TEXT("Static attachment exists before gear art is assigned"),Mesh))
        {
            TestTrue(TEXT("Attachment uses its appropriate root"), Mesh->GetAttachParent() ==
                Fighter->FighterLandingGearRoot.Get());
            TestTrue(TEXT("No attachment collision"),Mesh->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
        }
    }
    LandingGear Gear;
    Gear.SetState(LandingGear::GEAR_DOWN); Gear.SetState(LandingGear::GEAR_RAISE);
    Gear.ExecFrame(0.5); TestTrue(TEXT("Retracting fraction drives visuals"),Gear.GetDeployment()>0 && Gear.GetDeployment()<1);
    Gear.ExecFrame(0.6); Gear.ExecFrame(0);
    TestTrue(TEXT("Retracted gear no longer signals down"),Gear.GetState()==LandingGear::GEAR_UP);
    Gear.SetState(LandingGear::GEAR_LOWER); Gear.ExecFrame(1.1); Gear.ExecFrame(0);
    TestTrue(TEXT("Full extension signals gear-down HUD state"),Gear.GetState()==LandingGear::GEAR_DOWN);
    return true;
}
#endif
