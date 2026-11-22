#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "ScopedTestWorldContext.h"
#include "CutsceneReplayDlg.h"
#include "Components/TextBlock.h"
#include "Components/OverlaySlot.h"
#include "Components/Viewport.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"
#include "Sim.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArchiveReplayTest,"Starshatter.Intel.ArchiveReplay",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FArchiveReplayTest::RunTest(const FString& Parameters)
{
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    FScopedSSWTestWorldContext TestWorldContext1(World);
    if(!TestNotNull(TEXT("Replay fixture world"),World))return false;
    World->SetGameInstance(NewObject<UGameInstance>());
    const Sim* Before=Sim::GetSim();
    TStrongObjectPtr<UCutsceneReplayDlg> Panel(NewObject<UCutsceneReplayDlg>(World));
    Panel->UUserWidget::Initialize();auto Slate=Panel->TakeWidget();
    FS_CampaignMission Scene;Scene.Scene=TEXT("Replay fixture");Scene.Objective=TEXT("Training introduction");
    FS_MissionEvent Title;Title.EventType=MISSIONEVENT_TYPE::DISPLAY;Title.EventTime=0;Title.EventMessage=TEXT("Recorded title");Scene.Event.Add(Title);
    FS_MissionEvent End;End.EventType=MISSIONEVENT_TYPE::END_SCENE;End.EventTime=10;Scene.Event.Add(End);
    Panel->LoadReplay(Scene,1);
    TestNotNull(TEXT("Replay owns an isolated viewport"),Panel->ReplayViewport.Get());
    auto* ViewSlot=Cast<UOverlaySlot>(Panel->ReplayViewport->Slot);
    if(TestNotNull(TEXT("Replay viewport has overlay layout"),ViewSlot))
    {
        TestEqual(TEXT("Replay fills panel width"),ViewSlot->GetHorizontalAlignment(),HAlign_Fill);
        TestEqual(TEXT("Replay fills panel height"),ViewSlot->GetVerticalAlignment(),VAlign_Fill);
    }
    TestEqual(TEXT("Replay captions do not impose full-screen width"),Panel->CaptionTextBottom->GetMinDesiredWidth(),0.f);
    TestTrue(TEXT("Replay starts playback"),Panel->bReplayPlaying);
    TestEqual(TEXT("Original events copied"),Panel->SortedEvents.Num(),2);
    Panel->ReplayPause();TestFalse(TEXT("Pause stops replay clock"),Panel->bReplayPlaying);
    Panel->ReplaySeconds=4;Panel->ReplayRewind();TestEqual(TEXT("Rewind resets only replay time"),Panel->ReplaySeconds,0.f);
    Panel->ReplayPlay();Panel->ReplayStop();TestFalse(TEXT("Stop leaves replay paused"),Panel->bReplayPlaying);
    TestTrue(TEXT("Replay does not replace the active simulation"),Sim::GetSim()==Before);
    World->DestroyWorld(false);return true;
}
#endif
