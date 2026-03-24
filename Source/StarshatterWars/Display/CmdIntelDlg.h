/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         CmdIntelDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmdIntelDlg (Unreal port of CmdIntelDlg)
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "Blueprint/UserWidget.h"
#include "CmdDlg.h"
#include "GameStructs.h"
#include "CmdIntelDlg.generated.h"

// Forward declarations (UMG)
class UButton;
class UTextBlock;
class UImage;
class UListView;
class URichTextBlock;

// Starshatter core forward declarations
class Starshatter;
class Campaign;
class CombatEvent;
class UCmpnScreen;

UENUM(BlueprintType)
enum class ECmdIntelRowType : uint8
{
    Event,
    Blank
};

UCLASS()
class STARSHATTERWARS_API UCmdIntelNewsItem : public UObject
{
    GENERATED_BODY()

public:
    // Display columns (matches legacy list columns)
    UPROPERTY() FString UnreadMark;  // "*" or " "
    UPROPERTY() FString Date;        // formatted day/time
    UPROPERTY() FString Title;
    UPROPERTY() FString Loc;
    UPROPERTY() FString Source;

    // Backing pointer to legacy event (non-UObject). Do not UPROPERTY this.
    CombatEvent* EventPtr = nullptr;

    UPROPERTY() ECmdIntelRowType RowType = ECmdIntelRowType::Event;
};

UCLASS()
class STARSHATTERWARS_API UCmdIntelDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCmdIntelDlg(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
    void SetManager(UCmpnScreen* InManager);
    void SetParentCmdDlg(UCmdDlg* InParentCmdDlg);
    void ShowIntelDlg();

private:
    void BindFormWidgets();

    // Core tick logic (mirrors legacy CmdIntelDlg::ExecFrame)
    void ExecFrame();

    // Header refresh (mirrors CmdDlg::ExecFrame header portion)
    void ExecHeaderFrame();

    // List maintenance
    void RebuildNewsListIfCampaignChanged();
    void AppendNewEventsIfAny();
    void AutoScrollToFirstUnreadIfNeeded();

    // Selection actions
    void ShowSelectedEvent(CombatEvent* EventPtr, int32 SelectedIndex);
    CombatEvent* GetSelectedEvent(int32& OutSelectedIndex) const;

    // Movie overlay
    void ShowMovie();
    void HideMovie();

    // Helpers
    void ClearNewsDetails();

private:
    // Routed mode change
    void SetModeAndRoute(ECOMMAND_MODE InMode);

private:
    // UMG bindings (name them exactly in your widget blueprint, or bind manually)
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* txt_group = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* txt_score = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* txt_name = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* txt_time = nullptr;

    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_orders = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_theater = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_forces = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_intel = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_missions = nullptr;

    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_save = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_exit = nullptr;

    // Intel tab widgets
    UPROPERTY(meta = (BindWidgetOptional)) UListView* lst_news = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) URichTextBlock* txt_news = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UImage* img_news = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_play = nullptr;

    // Movie surface container:
    // In UMG, this should be a widget you can show/hide that hosts your cutscene view (e.g., a custom viewport widget).
    UPROPERTY(meta = (BindWidgetOptional)) UWidget* mov_news = nullptr;

private:

    UCmpnScreen* Manager = nullptr;

    Starshatter* Stars = nullptr;
    Campaign* CampaignPtr = nullptr;

    // Legacy tracking
    double UpdateTime = 0.0;
    int32 StartSceneCountdown = 0;
    FString EventScene;

    ECOMMAND_MODE Mode = ECOMMAND_MODE::MODE_INTEL;

    // Default image (optional): if you have a texture asset, set it in BP and apply to img_news.
    UPROPERTY() UTexture2D* DefaultNewsTexture = nullptr;

private:
    // UFUNCTION handlers (must be UFUNCTION for AddDynamic)
    UFUNCTION() void OnSaveClicked();
    UFUNCTION() void OnExitClicked();

    UFUNCTION() void OnModeOrdersClicked();
    UFUNCTION() void OnModeTheaterClicked();
    UFUNCTION() void OnModeForcesClicked();
    UFUNCTION() void OnModeIntelClicked();
    UFUNCTION() void OnModeMissionsClicked();

    UFUNCTION() void OnPlayClicked();

    // ListView delegates
    UFUNCTION() void OnNewsItemClicked(UObject* Item);

protected:
    UPROPERTY()
    UCmdDlg* ParentCmdDlg = nullptr;
};
