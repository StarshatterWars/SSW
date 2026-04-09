/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignSceneDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    CampaignSceneDlg (Unreal)
    - Timer-driven V1 cutscene viewer.
*/

#include "CampaignSceneDlg.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/RichTextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "CampaignScreen.h"

UCampaignSceneDlg::UCampaignSceneDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCampaignSceneDlg::NativeConstruct()
{
    Super::NativeConstruct();

    RegisterControls();

    bCutsceneInitialized = false;
    bSceneRunning = false;
    SceneStartRealSeconds = 0.0f;
    SceneDurationSeconds = 0.0f;
    ActiveSceneName.Empty();

    SetVisibility(ESlateVisibility::Collapsed);
}

void UCampaignSceneDlg::NativeDestruct()
{
    Super::NativeDestruct();
}

void UCampaignSceneDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Intentionally inert in V1.
    // Scene advancement is driven externally by the shared timer/event callback.
}

void UCampaignSceneDlg::RegisterControls()
{
    // BindWidgetOptional members are assigned automatically if names match in WBP.
    // No runtime discovery required for V1.
}

void UCampaignSceneDlg::Show()
{
    SetVisibility(ESlateVisibility::Visible);

    if (bEnableSubtitles)
    {
        if (SubtitlesText)
        {
            SubtitlesText->SetText(FText::GetEmpty());
        }
    }

    // V1 hook point:
    // SceneHost can later contain a render target image or your existing scene view widget.
}

void UCampaignSceneDlg::Hide()
{
    SetVisibility(ESlateVisibility::Collapsed);
}

void UCampaignSceneDlg::ExecFrame(float DeltaSeconds)
{
    // Retained for interface compatibility.
    // V1 scene progression is driven by AdvanceSceneFromTimer().
    (void)DeltaSeconds;
}

void UCampaignSceneDlg::BeginSceneByName(const FString& InSceneName, float InDurationSeconds)
{
    ActiveSceneName = InSceneName;
    SceneDurationSeconds = FMath::Max(0.0f, InDurationSeconds);
    SceneStartRealSeconds = UGameplayStatics::GetRealTimeSeconds(GetWorld());
    bSceneRunning = true;
    bCutsceneInitialized = false;

    SubtitleLines.Reset();
    SubtitleTopLine = 0;
    SubtitlesDelaySeconds = 0.0f;
    NextSubtitleTimeSeconds = 0.0f;

    Show();

    if (bEnableSubtitles)
    {
        BuildSubtitlesCache();

        if (SubtitlesText)
        {
            SubtitlesText->SetText(FText::GetEmpty());
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("[CampaignSceneDlg] BeginSceneByName: Scene=%s Duration=%.2f"),
        *ActiveSceneName,
        SceneDurationSeconds);
}

void UCampaignSceneDlg::AdvanceSceneFromTimer(float NowSeconds)
{
    if (!bSceneRunning)
    {
        return;
    }

    if (!bCutsceneInitialized)
    {
        bCutsceneInitialized = true;

        UE_LOG(LogTemp, Log,
            TEXT("[CampaignSceneDlg] Initializing scene: %s"),
            *ActiveSceneName);

        // Future hook:
        // initialize scene render / camera / display pipeline here.
    }

    if (bEnableSubtitles)
    {
        AdvanceSubtitlesIfNeeded(NowSeconds);
    }

    const float Elapsed = NowSeconds - SceneStartRealSeconds;
    if (SceneDurationSeconds > 0.0f && Elapsed >= SceneDurationSeconds)
    {
        UE_LOG(LogTemp, Log,
            TEXT("[CampaignSceneDlg] Scene complete: %s"),
            *ActiveSceneName);

        bSceneRunning = false;
        Hide();

        if (Manager)
        {
            Manager->ShowCmdDlg();
        }
    }
}

void UCampaignSceneDlg::BuildSubtitlesCache()
{
    SubtitleLines.Reset();

    // V1 safe default:
    // use current widget text as subtitle source if pre-seeded externally.
    FString Raw;
    if (SubtitlesText)
    {
        Raw = SubtitlesText->GetText().ToString();
    }

    if (Raw.IsEmpty())
    {
        return;
    }

    Raw.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
    Raw.ReplaceInline(TEXT("\r"), TEXT("\n"));
    Raw.ParseIntoArrayLines(SubtitleLines, false);

    if (SubtitlesText)
    {
        SubtitlesText->SetText(FText::GetEmpty());
    }
}

void UCampaignSceneDlg::AdvanceSubtitlesIfNeeded(float NowSeconds)
{
    if (!SubtitlesText)
    {
        return;
    }

    if (SubtitleLines.Num() <= 0)
    {
        return;
    }

    // Fallback delay per line for V1.
    // Later this can be derived from BeginScene/EndScene timing from mission data.
    if (SubtitlesDelaySeconds == 0.0f)
    {
        SubtitlesDelaySeconds = 2.5f;
        NextSubtitleTimeSeconds = NowSeconds + SubtitlesDelaySeconds;
        SubtitleTopLine = 0;
    }

    if (SubtitlesDelaySeconds > 0.0f && NowSeconds >= NextSubtitleTimeSeconds)
    {
        NextSubtitleTimeSeconds = NowSeconds + SubtitlesDelaySeconds;
        SubtitleTopLine++;
    }

    if (SubtitleTopLine < 0)
    {
        SubtitleTopLine = 0;
    }

    if (SubtitleTopLine >= SubtitleLines.Num())
    {
        SubtitleTopLine = SubtitleLines.Num() - 1;
    }

    const int32 Start = FMath::Clamp(SubtitleTopLine, 0, SubtitleLines.Num() - 1);
    const int32 EndExclusive = FMath::Clamp(
        Start + FMath::Max(1, MaxSubtitleLinesVisible),
        0,
        SubtitleLines.Num());

    FString Out;
    for (int32 i = Start; i < EndExclusive; ++i)
    {
        Out += SubtitleLines[i];

        if (i + 1 < EndExclusive)
        {
            Out += TEXT("\n");
        }
    }

    SubtitlesText->SetText(FText::FromString(Out));
}