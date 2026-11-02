#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "MissionDebriefResult.h"
#include "CampaignSave.h"
#include "Kismet/GameplayStatics.h"
#include "MissionDebriefDlg.h"
#include "Mission.h"
#include "SimEvent.h"
#include "SimElement.h"
#include "Instruction.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/CanvasPanel.h"
#include "Blueprint/WidgetTree.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#include "ContentStreaming.h"
#include "Components/Button.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "RenderingThread.h"
#include "DynamicRHI.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionDebriefResultTest,"Starshatter.Mission.DebriefOutcome",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionDebriefResultTest::RunTest(const FString&)
{
    FMissionDebriefEvidence E;
    E.bBasicFlightTraining=true; E.bDocked=true; E.RoutePoints=5; E.CompletedRoutePoints=5;
    TestTrue(TEXT("T1 route and safe docking succeed"),EvaluateMissionDebrief(E).bSuccess);
    E.CompletedRoutePoints=4;
    TestFalse(TEXT("Dock bonus cannot pass a skipped waypoint"),EvaluateMissionDebrief(E).bSuccess);
    E.CompletedRoutePoints=5; E.bDocked=false;
    TestFalse(TEXT("Route without docking remains incomplete"),EvaluateMissionDebrief(E).bSuccess);
    E.bDocked=true; E.bLost=true;
    TestFalse(TEXT("Loss cannot succeed"),EvaluateMissionDebrief(E).bSuccess);
    E={}; E.RequiredObjectives=2; E.CompletedObjectives=2;
    TestTrue(TEXT("All primary objectives succeed"),EvaluateMissionDebrief(E).bSuccess);
    E.bFailedObjective=true;
    TestFalse(TEXT("Failed primary objective overrides completion count"),EvaluateMissionDebrief(E).bSuccess);
    E={}; E.bDocked=true;
    TestEqual(TEXT("No authored criteria is not falsely labeled success"),EvaluateMissionDebrief(E).Outcome,FString(TEXT("NOT ASSESSED")));
    Mission Training; Training.SetIdentity(101); Training.SetMissionType(int(EMissionType::TRAINING));
    SimElement Route("Delta",1); ShipStats SafeRecovery("Delta 2",1);
    SafeRecovery.AddEvent(ESimEvent::DOCK,"Archon");
    for (int I=0; I<5; ++I)
    {
        auto* N=new Instruction("Janek",FVector::ZeroVector,I==4 ? EInstruction::RTB : EInstruction::Vector);
        N->SetStatus(I==4 ? INSTRUCTION_STATUS::ACTIVE : INSTRUCTION_STATUS::COMPLETE);
        Route.AddNavPoint(N,nullptr,false);
    }
    auto Collected=CollectMissionDebriefEvidence(&Training,&Route,&SafeRecovery);
    TestEqual(TEXT("Runtime route collects all five checkpoints including recovered RTB"),Collected.CompletedRoutePoints,5);
    TestTrue(TEXT("Real instructions and DOCK event produce T1 success"),EvaluateMissionDebrief(Collected).bSuccess);
    Route.GetFlightPlan().at(1)->SetStatus(INSTRUCTION_STATUS::SKIPPED);
    TestFalse(TEXT("Runtime skipped instruction is not treated as completed"),EvaluateMissionDebrief(CollectMissionDebriefEvidence(&Training,&Route,&SafeRecovery)).bSuccess);
    auto* Save=NewObject<UCampaignSave>();
    FMissionDebriefRecord Record; Record.MissionId=101; Record.Outcome=TEXT("SUCCESS");
    Record.Evidence=TEXT("5/5 waypoints; safely docked"); Record.Points=100; Record.Pilot=TEXT("Fixture");
    Save->MissionDebriefs.Add(Record); Save->RecordCompletedMission(101);
    TArray<uint8> Data;
    TestTrue(TEXT("Debrief evidence serializes through Unreal SaveGame"),UGameplayStatics::SaveGameToMemory(Save,Data));
    auto* Loaded=Cast<UCampaignSave>(UGameplayStatics::LoadGameFromMemory(Data));
    if (TestNotNull(TEXT("Debrief save reloads"),Loaded) && TestEqual(TEXT("One result persists"),Loaded->MissionDebriefs.Num(),1))
    {
        TestEqual(TEXT("Success persists separately from points"),Loaded->MissionDebriefs[0].Outcome,Record.Outcome);
        TestEqual(TEXT("Evidence persists"),Loaded->MissionDebriefs[0].Evidence,Record.Evidence);
        TestEqual(TEXT("Docking score preserved"),Loaded->MissionDebriefs[0].Points,100);
        TestTrue(TEXT("Successful assignment persists"),Loaded->CompletedMissionIds.Contains(101));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionDebriefUiTest,"Starshatter.Mission.DebriefLayoutAndSnapshot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMissionDebriefUiTest::RunTest(const FString&)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* GI=NewObject<UGameInstance>(); World->SetGameInstance(GI);
    ON_SCOPE_EXIT {ShipStats::Initialize(); World->DestroyWorld(false);};
    UClass* Class=LoadClass<UMissionDebriefDlg>(nullptr,TEXT("/Game/Screens/Mission/WBP_MissionDebrief.WBP_MissionDebrief_C"));
    if (!TestNotNull(TEXT("Authored debrief Blueprint loads"),Class)) return false;
    auto* Screen=NewObject<UMissionDebriefDlg>(World,Class); Screen->Initialize();
    auto Widget=Screen->TakeWidget(); Widget->SlatePrepass();
    TestTrue(TEXT("Modern debrief built"),Screen->bModernUiBuilt);
    bool Statistics=false;
    Screen->WidgetTree->ForEachWidget([&](UWidget* W){if(auto* T=Cast<UTextBlock>(W))Statistics|=T->GetText().ToString()==TEXT("STATISTICS");});
    TestTrue(TEXT("Statistics heading uses correct spelling"),Statistics);
    TestTrue(TEXT("Unit rows use the required compiled UMG asset"),
        Screen->UnitListWidget->GetEntryWidgetClass() && Screen->UnitListWidget->GetEntryWidgetClass()->ClassGeneratedBy!=nullptr);
    if (!TestNotNull(TEXT("Result label exists"),Screen->OutcomeWidget.Get())) return false;
    Screen->EnsureUi(); Screen->EnsureUi();
    TestEqual(TEXT("One modern accept button after repeated setup"),Screen->CloseButtonWidget->GetToolTipText().ToString(),FString(TEXT("Accept debrief and return")));
    Mission Fixture; Fixture.SetName("Basic Space Flight"); Fixture.SetIdentity(101); Fixture.SetTeam(1); Fixture.SetRegion("Janek");
    Screen->MissionPtr=&Fixture;
    Screen->DebriefResult={true,TEXT("SUCCESS"),TEXT("Training route: 5/5 waypoints complete. Recovery: docked safely.")};
    auto* Player=ShipStats::Find("Delta 2"); Player->SetIFF(1); Player->SetRegion("Janek"); Player->SetRole("Training"); Player->SetType("Falcon");
    Player->AddGunShot(); Player->AddEvent(ESimEvent::DOCK,"Archon","Deck 3");
    auto* Instructor=ShipStats::Find("Delta 1"); Instructor->SetIFF(1); Instructor->SetRegion("Janek"); Instructor->SetRole("Instructor"); Instructor->SetType("Falcon"); Instructor->AddGunShot(); Instructor->AddGunShot();
    Screen->DrawUnits();
    TestEqual(TEXT("Both unit records displayed"),Screen->UnitListWidget->GetNumItems(),2);
    ShipStats::Initialize(); // CommitMission does this while debrief remains visible.
    Screen->DrawSelectedUnit(1);
    TestTrue(TEXT("Instructor summary survives stats cleanup"),Screen->SummaryListWidget->GetNumItems()>5);
    auto* First=Cast<UDebriefListItem>(Screen->SummaryListWidget->GetItemAt(0));
    if (TestNotNull(TEXT("Snapshot first row"),First)) TestEqual(TEXT("Unit switch uses its own snapshot"),First->Col1,FString(TEXT("2")));
    Screen->DrawSelectedUnit(0);
    TestTrue(TEXT("Player event log survives cleanup"),Screen->EventListWidget->GetNumItems()>=2);
    Screen->ObjectivesWidget->SetText(FText::FromString(TEXT("Practice launching, flying, docking, and basic spaceborne maneuvering.")));
    Screen->MissionNameWidget->SetText(FText::FromString(TEXT("Basic Space Flight")));
    Screen->MissionSystemWidget->SetText(FText::FromString(TEXT("Janus")));
    Screen->MissionSectorWidget->SetText(FText::FromString(TEXT("Janek")));
    Screen->MissionTimeStartWidget->SetText(FText::FromString(TEXT("Day 01 09:23:00")));
    Screen->OutcomeWidget->SetText(FText::FromString(Screen->DebriefResult.Outcome));
    Screen->OutcomeWidget->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f,1.f,0.4f)));
    Screen->EvidenceWidget->SetText(FText::FromString(Screen->DebriefResult.Reason));
    Screen->MissionScoreWidget->SetText(FText::FromString(TEXT("100 points")));
    FString Capture;
    if (!GUsingNullRHI && FParse::Value(FCommandLine::Get(),TEXT("DebriefCapture="),Capture))
    {
        FlushAsyncLoading(); FAssetCompilingManager::Get().FinishAllCompilation();
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
        IStreamingManager::Get().StreamAllResources(0.f); FlushRenderingCommands();
        FWidgetRenderer Renderer(true,false);
        Renderer.DrawWidget(Widget,FVector2D(1920,1080)); FlushRenderingCommands();
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
        Widget->SlatePrepass(); auto* Target=Renderer.DrawWidget(Widget,FVector2D(1920,1080)); FlushRenderingCommands();
        TArray<FColor> Pixels; FReadSurfaceDataFlags Flags(RCM_UNorm); Flags.SetLinearToGamma(false);
        Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels,Flags);
        TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(1920,1080,Pixels,PNG);
        TestTrue(TEXT("Debrief preview written"),FFileHelper::SaveArrayToFile(PNG,*Capture));
    }
    Screen->MissionPtr=nullptr; Screen->RemoveFromParent();
    return true;
}
#endif
