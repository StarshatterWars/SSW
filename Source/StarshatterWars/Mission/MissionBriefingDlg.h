// MissionBriefingDlg.h

/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         MissionBriefingDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    MissionBriefingDlg (Unreal)
    - Common mission header + SIT/PKG/NAV/WEP + ACCEPT/CANCEL handling
    - Ported from legacy MsnDlg (Starshatter 4.5)
    - Widget components use UPROPERTY BindWidgetOptional for UMG wiring
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "MissionBriefingDlg.generated.h"

class UMissionPlanner;

class UButton;
class UTextBlock;

// Legacy sim/campaign forward declarations (your ported types):
class Campaign;
class Mission;
class MissionInfo;

UCLASS()
class STARSHATTERWARS_API UMissionBriefingDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UMissionBriefingDlg(const FObjectInitializer& ObjectInitializer);

    // External wiring:
    void SetManager(UMissionPlanner* InManager) { Manager = InManager; }

    // Legacy-like API:
    virtual void ShowMsnDlg();                 // refresh header + tabs + buttons
    virtual void OnCommit();                   // Accept
    virtual void OnCancel();                   // Cancel
    virtual void OnTabButton(UButton* Pressed);// SIT/PKG/NAV/WEP (Pressed is one of the tab buttons)

protected:
    // UUserWidget lifecycle
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    // Legacy helper:
    virtual int32 CalcTimeOnTarget() const;

protected:
    // ------------------------------------------------------------
    // UMG Bindings (OPTIONAL so you can wire gradually in UMG)
    // ------------------------------------------------------------

    // Header labels (legacy ids: 200/202/204/206/208/207)
    UPROPERTY(BlueprintReadOnly, Category = "MissionBriefing|Widgets", meta = (BindWidgetOptional))
    UTextBlock* MissionNameText = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "MissionBriefing|Widgets", meta = (BindWidgetOptional))
    UTextBlock* MissionSystemText = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "MissionBriefing|Widgets", meta = (BindWidgetOptional))
    UTextBlock* MissionSectorText = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "MissionBriefing|Widgets", meta = (BindWidgetOptional))
    UTextBlock* MissionTimeStartText = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "MissionBriefing|Widgets", meta = (BindWidgetOptional))
    UTextBlock* MissionTimeTargetText = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "MissionBriefing|Widgets", meta = (BindWidgetOptional))
    UTextBlock* MissionTimeTargetLabelText = nullptr;

    // Tabs (legacy ids: 900/901/902/903)
    UPROPERTY(BlueprintReadOnly, Category = "MissionBriefing|Widgets", meta = (BindWidgetOptional))
    UButton* SitButton = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "MissionBriefing|Widgets", meta = (BindWidgetOptional))
    UButton* PkgButton = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "MissionBriefing|Widgets", meta = (BindWidgetOptional))
    UButton* NavButton = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "MissionBriefing|Widgets", meta = (BindWidgetOptional))
    UButton* WepButton = nullptr;

    // Accept/Cancel (legacy ids: 1/2)
    UPROPERTY(BlueprintReadOnly, Category = "MissionBriefing|Widgets", meta = (BindWidgetOptional))
    UButton* AcceptButton = nullptr;

    // ------------------------------------------------------------
    // Options (MUST be exposed to avoid "Category but not exposed" warnings)
    // ------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MissionBriefing|Options")
    bool bDisableWeaponTabInNetLobby = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MissionBriefing|Options")
    bool bDisableTabsWhenMissionNotOK = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MissionBriefing|Options")
    bool bShowTimeOnTarget = true;

protected:
    // ------------------------------------------------------------
    // Raw pointers by request (NO UPROPERTY)
    // ------------------------------------------------------------
    UMissionPlanner* Manager = nullptr;

    Campaign* CampaignPtr = nullptr;
    Mission* MissionPtr = nullptr;
    MissionInfo* InfoPtr = nullptr;

    int32 PackageIndex = -1;

private:
    // Internal click handlers
    UFUNCTION()
    void HandleAcceptClicked();

    UFUNCTION()
    void HandleCancelClicked();

    UFUNCTION()
    void HandleSitClicked();

    UFUNCTION()
    void HandlePkgClicked();

    UFUNCTION()
    void HandleNavClicked();

    UFUNCTION()
    void HandleWepClicked();

private:
    void BindButtons();
    void UnbindButtons();
    static FText ToTextFromUtf8(const char* Utf8);
};
