#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "MissionBriefingDlg.generated.h"

class UMissionPlanner;
class UMenuButton;
class USelectableButtonGroup;
class UPanelWidget;
class UWidgetSwitcher;
class UButton;
class UTextBlock;
class Campaign;
class Mission;
class MissionInfo;

class UMissionObjectiveDlg;
class UMissionPackageDlg;
class UMissionNavDlg;
class UMissionWeaponDlg;

UENUM()
enum class EMissionBriefingMode : uint8
{
    SIT = 0,
    PKG,
    NAV,
    WEP
};

UCLASS()
class STARSHATTERWARS_API UMissionBriefingDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UMissionBriefingDlg(const FObjectInitializer& ObjectInitializer);

    void SetManager(UMissionPlanner* InManager) { Manager = InManager; }

    virtual void ShowMsnDlg();
    virtual void OnCommit();
    virtual void OnCancel();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual int32 CalcTimeOnTarget() const;

    void SetMode(EMissionBriefingMode NewMode);
    void RefreshHeader();
    void BuildMenuButtons();
    void RefreshMenuSelection();
    void InitializeSubPanels();

    UFUNCTION()
    void OnMenuToggleSelected(UMenuButton* SelectedButton);

    UFUNCTION()
    void OnMenuToggleHovered(UMenuButton* HoveredButton);

    UFUNCTION()
    void HandleAcceptClicked();

    UFUNCTION()
    void HandleCancelClicked();

protected:
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

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* PlayerNameText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* GameTimeText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* MissionTPlusText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* CurrentLocationText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* CurrentUnitText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* PlayerScoreText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UWidgetSwitcher* MissionSwitcher = nullptr;

    UPROPERTY(EditAnywhere, Category = "MissionBriefing|UI")
    TSubclassOf<UMenuButton> MenuButtonClass;

    UPROPERTY(meta = (BindWidgetOptional))
    USelectableButtonGroup* MenuToggleGroup = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UPanelWidget* MenuButtonContainer = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* MissionButton = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* ReturnButton = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* MissionButtonText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* ReturnButtonText = nullptr;

    // Switcher child panels
    UPROPERTY(meta = (BindWidgetOptional))
    UMissionObjectiveDlg* SitPanel = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UMissionPackageDlg* PkgPanel = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UMissionNavDlg* NavPanel = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UMissionWeaponDlg* WepPanel = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MissionBriefing|Options")
    bool bDisableWeaponTabInNetLobby = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MissionBriefing|Options")
    bool bDisableTabsWhenMissionNotOK = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MissionBriefing|Options")
    bool bShowTimeOnTarget = true;

    UPROPERTY()
    TArray<UMenuButton*> AllMenuButtons;

    UPROPERTY()
    TArray<FString> MenuItems = { TEXT("SIT"), TEXT("PKG"), TEXT("NAV"), TEXT("WEP") };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MissionBriefing|State")
    EMissionBriefingMode CurrentMode = EMissionBriefingMode::SIT;

    UMissionPlanner* Manager = nullptr;
    Campaign* CampaignPtr = nullptr;
    Mission* MissionPtr = nullptr;
    MissionInfo* InfoPtr = nullptr;
    int32 PackageIndex = -1;

private:
    static FText ToTextFromUtf8(const char* Utf8);
};