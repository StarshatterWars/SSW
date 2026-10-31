#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "CmpnScreen.h"
#include "CampaignSceneDlg.h"
#include "CmpLoadDlg.h"
#include "Blueprint/WidgetTree.h"
#include "CampaignSceneActor.h"
#include "Campaign.h"
#include "CombatEvent.h"
#include "CampaignSave.h"
#include "SSWGameInstance.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/LevelStreamingDynamic.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignSceneLoadingGateTest,"Starshatter.Campaign.SceneLoadingGate",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampaignSceneLoadingGateTest::RunTest(const FString& Parameters)
{
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    if(!TestNotNull(TEXT("Fixture world"),World))return false;
    World->SetGameInstance(NewObject<UGameInstance>());
    TStrongObjectPtr<UCmpLoadDlg> Cover(NewObject<UCmpLoadDlg>(World));
    Cover->UUserWidget::Initialize();
    auto CoverSlate=Cover->TakeWidget();
    if(TestNotNull(TEXT("Loading screen has a native root on first construction"),Cover->WidgetTree->RootWidget.Get()))
        TestTrue(TEXT("First Slate display uses the actual loading artwork/status tree"),Cover->WidgetTree->RootWidget->GetCachedWidget().IsValid());
    Cover->SetLoadingStage(TEXT("LOADING SCENE MODELS..."),0.5f);
    TestTrue(TEXT("Scene readiness stage survives loading-screen construction"),Cover->IsUsingManualStage());
    TStrongObjectPtr<UCmpnScreen> Screen(NewObject<UCmpnScreen>(World));
    TStrongObjectPtr<UCampaignSceneDlg> Scene(NewObject<UCampaignSceneDlg>(World));
    Screen->CmpSceneDlg=Scene.Get();
    Screen->SceneLoadScreenStartTime=-100;
    Screen->SceneWarmupReadyTime=-100;
    Screen->bSceneWarmupStarted=true;
    Screen->SceneReadyFrameCount=999;
    TestFalse(TEXT("Elapsed fixed delay alone cannot reveal missing scene"),Screen->CanRevealSceneNow());
    Screen->ActiveSceneStreamingLevel=NewObject<ULevelStreamingDynamic>(World);
    Screen->bScenePresentationPrepared=true;
    TestFalse(TEXT("Requested map without loaded environment remains covered"),Screen->CanRevealSceneNow());
    FString Status;
    TestFalse(TEXT("Missing models/environment are not presentation-ready"),Scene->IsPreparedSceneReady(Status));
    TestFalse(TEXT("Loading does not start the scene timeline"),Scene->IsSceneRunning());
    TestFalse(TEXT("Scene loading reports the unmet readiness stage"),Status.IsEmpty());
    ACampaignSceneActor* Actor=World->SpawnActor<ACampaignSceneActor>();
    if(TestNotNull(TEXT("Scene actor"),Actor))
    {
        TestFalse(TEXT("Unbuilt mission cannot pass model gate"),Actor->AreSceneShipsReady(Status));
        Actor->SetScenePlaybackEnabled(false);
    }
    Screen->SceneReadyFrameCount=17;Screen->bScenePresentationPrepared=true;
    Screen->BeginSceneTransition();
    TestEqual(TEXT("Next scene starts with no ready frames"),Screen->SceneReadyFrameCount,0);
    TestFalse(TEXT("Next scene must rebuild presentation"),Screen->bScenePresentationPrepared);
    TestFalse(TEXT("Failed/loading scene is never marked played"),Screen->bScenePlaybackStarted);
    Screen->ClearPendingSceneTransition();
    auto* Instance=NewObject<USSWGameInstance>();World->SetGameInstance(Instance);
    Instance->CampaignSave=NewObject<UCampaignSave>();Instance->CampaignSave->CampaignIndex=2;
    Campaign Story(2,"Scene read/view fixture",true);Story.SetCampaignStatus(ECampaignStatus::ACTIVE);Story.SetTime(0);
    auto* Event=new CombatEvent(&Story,ECombatEventType::NONE,0,0,ECombatEventSource::NONE,"");
    Event->SetSceneFile("01-News-Start");Event->SetVisited(true);Story.GetEvents().append(Event);
    Screen->CampaignPtr=&Story;
    TestTrue(TEXT("Reading intel cannot consume an unplayed cutscene"),Screen->FindFirstPendingSceneEvent()==Event);
    Instance->CampaignSave->RecordPlayedScene(TEXT("01-News-Start"));
    TestTrue(TEXT("Saved playback history prevents repeat presentation"),Screen->FindFirstPendingSceneEvent()==nullptr);
    Screen->CampaignPtr=nullptr;
    World->DestroyWorld(false);
    return true;
}
#endif
