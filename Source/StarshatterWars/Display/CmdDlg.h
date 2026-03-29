/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         CmdDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmdDlg
    - Unreal port of Starshatter CmdDlg helper/controller.
    - In classic code, CmdDlg is NOT a FormWindow; it binds to an existing FormWindow owned by CmpnScreen.
      In Unreal, we treat this as a UBaseScreen-derived widget that owns its own UMG widgets and uses FORM IDs.
    - Responsibilities:
        * Shows current campaign title, group description, team score, and time.
        * Provides mode buttons (Orders/Theater/Forces/Intel/Missions) and Save/Exit.
        * Updates Intel button label with unread count.
        * Enables/disables Save/Forces/Intel when campaign is training.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "Campaign.h"
#include "Starshatter.h"
#include "CmpnScreen.h"
#include "GameStructs.h"

#include "Engine/Texture2D.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/Border.h"
#include "Components/ComboBoxString.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "Components/WidgetSwitcher.h"
#include "TimerSubsystem.h"
#include "CmdDlg.generated.h"

class UTextBlock;
class UButton;
class Starshatter;
class Campaign;
class CombatGroup;
class UCmpFileDlg;

class UPanelWidget;
class USelectableButtonGroup;
class UMenuButton;
class UMenuScreen;
class UVerticalBox;
class UScrollBox;

class UUMenuButton;
class UTextBlock;
class UListView;
class URichTextBlock;

class UCmdOrdersDlg;
class UCmdForceDlg;
class UCmdMissionsDlg;
class UCmdIntelDlg;
class UCmdTheaterDlg;
class UCmdMsgDlg;

enum class ECmdScreen : uint8
{
    None,
    Orders,
    Missions,
    Intel,
    Theater,
    Forces
};

UCLASS()
class STARSHATTERWARS_API UCmdDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCmdDlg(const FObjectInitializer& ObjectInitializer);

public:
    virtual void SetMenuManager(UMenuScreen* InManager);
    virtual void InitializeDlg(UMenuScreen* InManager);

    UPROPERTY(meta = (BindWidgetOptional)) class UWidgetSwitcher* OperationalSwitcher;
    UPROPERTY(meta = (BindWidgetOptional)) class UCmdOrdersDlg* CmdOrdersPanel = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) class UCmdForceDlg* CmdForcesPanel = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) class UCmdIntelDlg* CmdIntelPanel = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) class UCmdMissionsDlg* CmdMissionsPanel = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) class UCmdTheaterDlg* CmdTheaterPanel = nullptr;

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Campaign")
    FS_Campaign CurrentCampaignData;

    UPROPERTY(BlueprintReadOnly, Category = "Campaign")
    bool bHasCurrentCampaign = false;

public:
    const FS_Campaign& GetCurrentCampaignData() const { return CurrentCampaignData; }
    bool HasCurrentCampaign() const { return bHasCurrentCampaign; }

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativePreConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
    virtual void BindFormWidgets() override;

public:
    void SetManager(UCmpnScreen* InManager);
    void ShowCmdDlg();
    void ExecFrame();

    int32 CurrentMissionId = -1;
    void UpdateMissionButton();

protected:
    UPROPERTY(Transient)
    TObjectPtr<UMenuScreen> manager = nullptr;

private:
    UCmpnScreen* CmpnScreen = nullptr;
    Campaign* CampaignPtr = nullptr;
    Starshatter* Stars = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* TitleText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* PlayerNameText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* GameTimeText;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* CampaignTPlusText;

    UPROPERTY(meta = (BindWidgetOptional)) class UTextBlock* LocationSystemText;
    UPROPERTY(meta = (BindWidgetOptional)) class UTextBlock* CampaignNameText;
    UPROPERTY(meta = (BindWidgetOptional)) class UTextBlock* OperationsModeText;
    UPROPERTY(meta = (BindWidgetOptional)) class UTextBlock* CurrentUnitText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) class UTextBlock* PlayerScoreText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) class UTextBlock* CurrentLocationText;

    UPROPERTY(meta = (BindWidgetOptional)) class UTextBlock* MissionButtonText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) class UButton* MissionButton = nullptr;

    UPROPERTY(meta = (BindWidgetOptional)) class UTextBlock* ReturnButtonText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) class UButton* ReturnButton = nullptr;

    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UMenuButton> MenuButtonClass;

    UPROPERTY(meta = (BindWidgetOptional))
    USelectableButtonGroup* MenuToggleGroup;

    UPROPERTY(meta = (BindWidgetOptional))
    UPanelWidget* MenuButtonContainer;

    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UTextBlock* txt_name = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), Transient) UButton* btn_save = nullptr;
    UPROPERTY(meta = (BindWidgetOptional), Transient) UButton* btn_exit = nullptr;

    ECOMMAND_MODE Mode = ECOMMAND_MODE::MODE_ORDERS;

    TArray<FString> MenuItems = {
        TEXT("ORDERS"),
        TEXT("THEATER"),
        TEXT("FORCES"),
        TEXT("INTEL"),
        TEXT("MISSIONS")
    };

    UPROPERTY()
    TArray<UMenuButton*> AllMenuButtons;

    void ShowDlg();
    void HideDlg();
    void RefreshUIFromSubsystem();

private:
    UFUNCTION() void OnSaveClicked();
    UFUNCTION() void OnExitClicked();

    UFUNCTION() void OnMenuToggleSelected(UMenuButton* SelectedButton);
    UFUNCTION() void OnMenuButtonSelected(UMenuButton* SelectedButton);
    UFUNCTION() void OnMenuToggleHovered(UMenuButton* SelectedButton);

    UFUNCTION() void OnCancelButtonClicked();
    UFUNCTION() void OnCancelButtonHovered();
    UFUNCTION() void OnCancelButtonUnHovered();

    UFUNCTION() void OnMissionButtonClicked();
    UFUNCTION() void OnMissionButtonHovered();
    UFUNCTION() void OnMissionButtonUnHovered();

    void LoadForcesInfo();
    void LoadOrdersInfo();
    void LoadMissionsInfo();
    void LoadIntelInfo();
    void LoadTheaterInfo();

    UFUNCTION() void HandleGameTimers();
    UFUNCTION() void HandleUniverseSecondTick(uint64 UniverseSecondsNow);
    UFUNCTION() void HandleUniverseMinuteTick(uint64 UniverseSecondsNow);
    UFUNCTION() void HandleCampaignTPlusChanged(uint64 UniverseSecondsNow, uint64 TPlusSeconds);

    ECmdScreen CurrentScreen = ECmdScreen::None;

    bool ShouldDisableCommandPanels() const;

    TArray<UMenuButton*> MenuButtonArray;
    void RefreshCommandButtons();

    bool bLastDisableState = false;
};