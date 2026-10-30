#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "CampaignPresentationRules.h"
#include "FighterActor.h"
#include "Components/StaticMeshComponent.h"
#include "LandingGear.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignPresentationRulesTest, "Starshatter.Campaign.PresentationRules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCampaignPresentationRulesTest::RunTest(const FString& Parameters)
{
    using namespace CampaignPresentationRules;
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
    for(UStaticMeshComponent* Mesh:{Fighter->LeftGear.Get(),Fighter->RightGear.Get(),Fighter->NoseGear.Get(),Fighter->Cockpit.Get()})
    {
        if(TestNotNull(TEXT("Static attachment exists before gear art is assigned"),Mesh))
        {
            TestTrue(TEXT("Attachment is directly under hull"),Mesh->GetAttachParent()==Fighter->HullMesh);
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
