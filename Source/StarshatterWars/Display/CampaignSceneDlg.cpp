/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignSceneDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    CampaignSceneDlg (Unreal)
    - Ported from legacy CmpSceneDlg (Starshatter 4.5).
*/

#include "CampaignSceneDlg.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/RichTextBlock.h"
#include "Kismet/GameplayStatics.h"

UCampaignSceneDlg::UCampaignSceneDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCampaignSceneDlg::NativeConstruct()
{
    Super::NativeConstruct();

    RegisterControls();

    // Dialog starts hidden by default in most flows; if you want auto-show, call Show() externally.
    bCutsceneInitialized = false;
}

void UCampaignSceneDlg::NativeDestruct()
{
    Super::NativeDestruct();
}

void UCampaignSceneDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ExecFrame(InDeltaTime);
}

void UCampaignSceneDlg::RegisterControls()
{
    // BindWidgetOptional members are already assigned by UMG if names match.
    // Nothing else to do here unless you want runtime widget discovery.
}

void UCampaignSceneDlg::Show()
{
    SetVisibility(ESlateVisibility::Visible);

    bCutsceneInitialized = false;

    if (bEnableSubtitles)
    {
        BuildSubtitlesCache();
        SubtitleTopLine = 0;
        SubtitlesDelaySeconds = 0.0f;
        NextSubtitleTimeSeconds = 0.0f;

        if (SubtitlesText)
        {
            SubtitlesText->SetText(FText::FromString(TEXT("")));
        }
    }

    // NOTE:
    // The legacy code wires CameraView/DisplayView to a window and scene.
    // In Unreal, SceneHost is typically a panel that contains:
    // - a SceneCapture2D render target image, OR
    // - a viewport/UMG wrapper you already built.
    //
    // Hook your existing cutscene/camera pipeline here.
}

void UCampaignSceneDlg::Hide()
{
    SetVisibility(ESlateVisibility::Collapsed);

    // Undo any "DisplayView set window" behavior here if your port has an equivalent.
}

void UCampaignSceneDlg::ExecFrame(float DeltaSeconds)
{
    // If you have a manager-driven flow:
    // - If cutscene is missing/over, bounce back to command dialog.
    //
    // Legacy behavior:
    // if (!cutscene_mission) manager->ShowCmdDlg();

    const float NowSeconds = UGameplayStatics::GetRealTimeSeconds(GetWorld());

    if (!bCutsceneInitialized)
    {
        bCutsceneInitialized = true;

        // Initialize lens flare/corona elements if your camera view supports it.
        // In Unreal, usually handled by post process/materials; keep flags for parity.
        //
        // bEnableLensFlare: use flare1..flare4
        // bEnableCoronaOnly: flare1 only
    }

    if (bEnableSubtitles)
    {
        AdvanceSubtitlesIfNeeded(NowSeconds);
    }
}

void UCampaignSceneDlg::BuildSubtitlesCache()
{
    SubtitleLines.Reset();

    // Legacy: stars->GetSubtitles()
    // In Unreal: you can push subtitles text into the widget before Show(), or fetch from a subsystem.
    //
    // For now, read whatever is currently in SubtitlesText as the source (safe default).
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

    // Reset displayed text
    if (SubtitlesText)
    {
        SubtitlesText->SetText(FText::FromString(TEXT("")));
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

    // Legacy computes delay from (END_SCENE - BEGIN_SCENE) / nlines.
    // If you have that mission event timing, set SubtitlesDelaySeconds before calling Show().
    //
    // Fallback: 2.5 seconds per line.
    if (SubtitlesDelaySeconds == 0.0f)
    {
        SubtitlesDelaySeconds = 2.5f;
        NextSubtitleTimeSeconds = NowSeconds + SubtitlesDelaySeconds;

        // Render initial block
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

    // Render a window of lines like a scrolling box
    const int32 Start = FMath::Clamp(SubtitleTopLine, 0, SubtitleLines.Num() - 1);
    const int32 EndExclusive = FMath::Clamp(Start + FMath::Max(1, MaxSubtitleLinesVisible), 0, SubtitleLines.Num());

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
