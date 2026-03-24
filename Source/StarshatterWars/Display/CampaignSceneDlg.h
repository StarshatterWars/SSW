/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignSceneDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    CampaignSceneDlg (Unreal)
    - Campaign title card and load/cutscene dialog.
    - Ported from legacy CmpSceneDlg (Starshatter 4.5).
    - Hosts a "scene" view area plus optional subtitles.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "CampaignSceneDlg.generated.h"

class UPanelWidget;
class URichTextBlock;

class UCampaignScreen;

class UTexture2D;

UCLASS()
class STARSHATTERWARS_API UCampaignSceneDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCampaignSceneDlg(const FObjectInitializer& ObjectInitializer);

    void SetManager(UCampaignScreen* InManager) { Manager = InManager; }

    virtual void Show();
    virtual void Hide();

    virtual void ExecFrame(float DeltaSeconds);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    void RegisterControls();
    void BuildSubtitlesCache();
    void AdvanceSubtitlesIfNeeded(float NowSeconds);

protected:
    // UMG bind points (wire these in the widget blueprint)
    UPROPERTY(VisibleAnywhere, Category = "CampaignScene|Widgets", meta = (BindWidgetOptional))
    UPanelWidget* SceneHost = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "CampaignScene|Widgets", meta = (BindWidgetOptional))
    URichTextBlock* SubtitlesText = nullptr;

    // Options
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CampaignScene|Options")
    bool bEnableLensFlare = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CampaignScene|Options")
    bool bEnableCoronaOnly = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CampaignScene|Options")
    bool bEnableSubtitles = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CampaignScene|Options")
    int32 MaxSubtitleLinesVisible = 6;

    // Assets (assign in defaults/BP)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CampaignScene|Assets")
    TSoftObjectPtr<UTexture2D> Flare1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CampaignScene|Assets")
    TSoftObjectPtr<UTexture2D> Flare2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CampaignScene|Assets")
    TSoftObjectPtr<UTexture2D> Flare3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CampaignScene|Assets")
    TSoftObjectPtr<UTexture2D> Flare4;

protected:
    // Raw pointers by request (no UPROPERTY)
    UCampaignScreen* Manager = nullptr;

    // Subtitles state
    TArray<FString> SubtitleLines;
    int32 SubtitleTopLine = 0;
    float SubtitlesDelaySeconds = 0.0f;
    float NextSubtitleTimeSeconds = 0.0f;

    // One-shot init each Show()
    bool bCutsceneInitialized = false;
};
