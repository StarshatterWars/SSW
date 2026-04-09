/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignSceneDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    CampaignSceneDlg (Unreal)
    - Campaign title card / cutscene viewer.
    - Timer-driven V1 implementation.
    - Scene progression is advanced externally by the existing
      campaign/event timer callback, not by widget tick.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "CampaignSceneDlg.generated.h"

class UPanelWidget;
class URichTextBlock;
class UTexture2D;

class UCampaignScreen;

UCLASS()
class STARSHATTERWARS_API UCampaignSceneDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCampaignSceneDlg(const FObjectInitializer& ObjectInitializer);

    void SetManager(UCampaignScreen* InManager) { Manager = InManager; }

    virtual void Show();
    virtual void Hide();

    // Legacy-style entry point retained for compatibility,
    // but V1 scene progression is driven externally.
    virtual void ExecFrame(float DeltaSeconds);

    // ------------------------------------------------------------
    // Timer-driven V1 API
    // ------------------------------------------------------------
    void BeginSceneByName(const FString& InSceneName, float InDurationSeconds);
    void AdvanceSceneFromTimer(float NowSeconds);

    bool IsSceneRunning() const { return bSceneRunning; }
    const FString& GetActiveSceneName() const { return ActiveSceneName; }

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    void RegisterControls();
    void BuildSubtitlesCache();
    void AdvanceSubtitlesIfNeeded(float NowSeconds);

protected:
    // UMG bind points
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

    // Assets
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CampaignScene|Assets")
    TSoftObjectPtr<UTexture2D> Flare1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CampaignScene|Assets")
    TSoftObjectPtr<UTexture2D> Flare2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CampaignScene|Assets")
    TSoftObjectPtr<UTexture2D> Flare3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CampaignScene|Assets")
    TSoftObjectPtr<UTexture2D> Flare4;

    // ------------------------------------------------------------
    // Timer-driven scene state
    // ------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CampaignScene|State")
    FString ActiveSceneName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CampaignScene|State")
    float SceneStartRealSeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CampaignScene|State")
    float SceneDurationSeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CampaignScene|State")
    bool bSceneRunning = false;

protected:
    // Raw pointer by request
    UCampaignScreen* Manager = nullptr;

    // Subtitles state
    TArray<FString> SubtitleLines;
    int32 SubtitleTopLine = 0;
    float SubtitlesDelaySeconds = 0.0f;
    float NextSubtitleTimeSeconds = 0.0f;

    // One-shot init per scene start
    bool bCutsceneInitialized = false;
};