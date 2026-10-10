#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "GameStructs.h"
#include "MissionBriefingDlg.generated.h"

class UEnhancedInputComponent;
class UInputMappingContext;
class UInputAction;
class SFighterHUDDetails;
class SEngineeringPopup;
class UEngineeringDlg;
class UWeaponsDlg;
class SWeaponsPopup;
class SObjectivesPopup;
class SNavigationPopup;
class UNavigationDlg;
struct FMissionCameraRig;
struct FInputActionValue;
class QuitView;
class UQuitMissionMenu;
class ACameraActor;
class ULevelStreamingDynamic;
class SWidget;
class AActor;
class UMissionPlanner;
class UMenuButton;
class UMenuScreen;
class USizeBox;
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
    virtual void ExecFrame();

    void SetCampaign(Campaign* InCampaign) { CampaignPtr = InCampaign; }
    void SetMission(Mission* InMission) { MissionPtr = InMission; }

    // Menu manager hookup (matches your existing pattern)
    virtual void SetMenuManager(UMenuScreen* InManager);
    virtual void InitializeDlg(UMenuScreen* InManager);

    Mission* GetMissionPtr() const { return MissionPtr; }

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;


    virtual int32 CalcTimeOnTarget() const;

    void SetMode(EMissionBriefingMode NewMode);
    void RefreshHeader();
    void CloseEmptySectorPreview(bool bPreserveSimulation = false);
    bool StartLiveMission();
    bool bLiveMissionStarted = false;
    bool EnableMissionMenuInput();
    void DisableMissionMenuInput();
    void ToggleMissionMenu();
    bool bPreviousTargetHeld = false;
    bool bNextTargetHeld = false;
    void ReleasePreviousMissionTarget();
    void ReleaseNextMissionTarget();
    UPROPERTY(Transient) TObjectPtr<UInputAction> EngineeringAction;
    TSharedPtr<SWidget> EngineeringPopup;
    UPROPERTY(Transient) TObjectPtr<UEngineeringDlg> EngineeringPanelWidget;
    bool bEngineeringPreviousCursor = false;
    void ToggleEngineering();
    void CloseEngineering();
    TSharedPtr<SWidget> WeaponsPopup;
    UPROPERTY(Transient) TObjectPtr<UWeaponsDlg> WeaponsPanelWidget;
    bool bWeaponsPreviousCursor = false;
    void CloseWeaponsPopup();
    UPROPERTY(Transient) TObjectPtr<UInputAction> ObjectivesPanelAction;
    TSharedPtr<SObjectivesPopup> ObjectivesPopup;
    bool bObjectivesPreviousCursor = false;
    void ToggleObjectivesPopup();
    void CloseObjectivesPopup();
    UPROPERTY(Transient) TObjectPtr<UInputAction> NavMapAction;
    UPROPERTY(Transient) TObjectPtr<UNavigationDlg> NavigationPanelWidget;
    TSharedPtr<SWidget> NavigationPopup;
    bool bNavigationPreviousCursor=false;
    void ToggleNavigationPopup();
    void CloseNavigationPopup();
    UPROPERTY(Transient) TObjectPtr<UInputAction> RadioMenuAction;
    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> RadioChoiceContext;
    UPROPERTY(Transient) TArray<TObjectPtr<UInputAction>> RadioChoiceActions;
    void ToggleFighterRadio();
    void UpdateFighterRadioInput();
    void SelectFighterRadio(int32 Number);
    void SelectFighterRadio0();
    void SelectFighterRadio1();
    void SelectFighterRadio2();
    void SelectFighterRadio3();
    void SelectFighterRadio4();
    void SelectFighterRadio5();
    UPROPERTY(Transient) TObjectPtr<UInputAction> GearToggleAction;
    void ToggleMissionGear();
    UPROPERTY(Transient) TObjectPtr<UInputAction> MissionThrottleAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> MissionThrottleZeroAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> MissionThrottleFullAction;
    void OnMissionThrottleStep(const FInputActionValue& Value);
    void OnMissionThrottleZero();
    void ApplyMissionRotation(int32 Axis, float Value);
    void ApplyMissionTranslation(int32 Axis, float Value);
    void ApplyMissionAugmenter(bool Enabled);
    void ClearMissionRotationInput();
    UPROPERTY(Transient) TObjectPtr<UInputAction> MissionStrafeAction;
    void OnMissionStrafe(const FInputActionValue& Value);
    void ReleaseMissionStrafe();
    UPROPERTY(Transient) TObjectPtr<UInputAction> MissionForwardThrustAction;
    void OnMissionForwardThrust(const FInputActionValue& Value);
    void ReleaseMissionForwardThrust();
    UPROPERTY(Transient) TObjectPtr<UInputAction> MissionVerticalThrustAction;
    void OnMissionVerticalThrust(const FInputActionValue& Value);
    void ReleaseMissionVerticalThrust();
    UPROPERTY(Transient) TObjectPtr<UInputAction> MissionAugmenterAction;
    void OnMissionAugmenter();
    void ReleaseMissionAugmenter();

    void ApplyMissionShields(int32 Command);
    UPROPERTY(Transient) TObjectPtr<UInputAction> MissionShieldsUpAction;
    void OnMissionShieldsUp();
    UPROPERTY(Transient) TObjectPtr<UInputAction> MissionShieldsDownAction;
    void OnMissionShieldsDown();
    UPROPERTY(Transient) TObjectPtr<UInputAction> MissionShieldsFullAction;
    void OnMissionShieldsFull();
    UPROPERTY(Transient) TObjectPtr<UInputAction> MissionShieldsZeroAction;
    void OnMissionShieldsZero();

    UPROPERTY(Transient) TObjectPtr<UInputAction> MissionPitchAction;
    void OnMissionPitch(const FInputActionValue& Value);
    void ReleaseMissionPitch();
    UPROPERTY(Transient) TObjectPtr<UInputAction> MissionYawAction;
    void OnMissionYaw(const FInputActionValue& Value);
    void ReleaseMissionYaw();
    UPROPERTY(Transient) TObjectPtr<UInputAction> MissionRollAction;
    void OnMissionRoll(const FInputActionValue& Value);
    void ReleaseMissionRoll();

    void OnMissionThrottleFull();
    void ApplyMissionThrottle(double Amount, bool bRelative);

    void PreviousMissionTarget();
    void NextMissionTarget();
    void CycleMissionTarget(int32 Direction);
    void UpdateMissionTargetCamera();
    void BindMissionCameraInput();
    void OnMissionCameraCommand(const FInputActionValue& Value, int32 Command);
    void ReleaseMissionCameraCommand(const FInputActionValue& Value, int32 Command);
    void CycleMissionViewObject();
    TSharedPtr<FMissionCameraRig> MissionCameraRig;
    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> MissionCameraContext;
    UPROPERTY(Transient) TArray<TObjectPtr<UInputAction>> MissionCameraActions;
    void ToggleMissionCameraView();
    bool bMissionTargetInspection = false;
    TWeakObjectPtr<AActor> MissionPlayerCameraActor;
    FBox MissionPlayerLocalBounds = FBox(ForceInit);
    double MissionPlayerCameraSearchTime = 0;
    double MissionPlayerCameraDistance = 0;
    UPROPERTY(Transient) TObjectPtr<UInputAction> MissionCameraViewAction;
    void OnTargetZoom(const FInputActionValue& Value);
    void ToggleFighterHUD();
    void ToggleFighterCautionPanel();
    UPROPERTY(Transient) TObjectPtr<UInputAction> HUDWarningsAction;
    void ToggleFighterWeaponsPanel();
    UPROPERTY(Transient) TObjectPtr<UInputAction> WeaponsPanelAction;
    void CycleFighterMFDLeft();
    void CycleFighterMFDRight();
    void CycleFighterWeapon();
    bool CanUseFighterHUD() const;
    TSharedPtr<SFighterHUDDetails> FighterHUDDetails;
    UPROPERTY(Transient) TObjectPtr<UInputAction> FighterMFDLeftAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> FighterMFDRightAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> FighterWeaponCycleAction;
    bool bFighterHUDVisible = true;
    TSharedPtr<SWidget> FighterHUDPanels;
    UPROPERTY(Transient) TObjectPtr<UInputAction> FighterHUDToggleAction;
    double MissionTargetZoomDistance = 1.0;
    UPROPERTY(Transient) TObjectPtr<UInputAction> MissionTargetZoomAction;
    TWeakObjectPtr<AActor> SelectedMissionTarget;
    FText SelectedMissionTargetName;
    FBox SelectedMissionTargetLocalBounds = FBox(ForceInit);
    TSharedPtr<SWidget> MissionTargetOverlay;
    FTimerHandle MissionTargetCameraTimer;
    double MissionTargetLastUpdate = 0.0;
    void HandleMissionMenuAction(uintptr_t Action);
    UPROPERTY(Transient) TObjectPtr<UEnhancedInputComponent> MissionMenuInput;
    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> MissionMenuContext;
    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> MissionTargetContext;
    UPROPERTY(Transient) TObjectPtr<UInputAction> PreviousMissionTargetAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> NextMissionTargetAction;
    bool bAddedMissionMenuContext = false;
    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> FighterInputContext;
    bool bAddedFighterInputContext = false;
    UPROPERTY(EditDefaultsOnly, Category="Mission|Pause Menu")
    TSubclassOf<UQuitMissionMenu> QuitMissionMenuClass;
    QuitView* MissionQuitMenu = nullptr;
    bool bMissionControlsOpen = false;
    bool bControlsPreviousWorldPaused = false;
    bool bControlsPreviousRuntimePaused = false;

    UFUNCTION()
    void HandleMissionSystemLevelShown();
    void RevealMissionSunWhenCameraReady();
    FTimerHandle MissionSunRevealTimer;
    TArray<TWeakObjectPtr<AActor>> MissionSunHiddenActors;
    int32 MissionCameraReadyChecks = 0;
    double MissionCameraStableSince = -1.0;

    UPROPERTY(Transient)
    TObjectPtr<ULevelStreamingDynamic> MissionSystemLevel = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<ACameraActor> MissionPreviewCamera = nullptr;
    TWeakObjectPtr<AActor> PreviousMissionViewTarget;
    FString MissionSystemPackage;
    double MissionTitleRevealTime = -1.0;
    TSharedPtr<SWidget> EmptySectorOverlay;
    TSharedPtr<SWidget> MissionSceneCover;
    TArray<TWeakObjectPtr<AActor>> PreviewHiddenActors;
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
    UPROPERTY(Transient)
    TObjectPtr<UMenuScreen> manager = nullptr;

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Campaign")
    FS_Campaign CurrentCampaignData;

    UPROPERTY(BlueprintReadOnly, Category = "Campaign")
    bool bHasCurrentCampaign = false;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    USizeBox* RootSizeBox = nullptr;
    
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* TitleText;
    
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
    UVerticalBox* MenuButtonContainer = nullptr;

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
    UMissionObjectiveDlg* MissionSituationPanel = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UMissionPackageDlg* MissionPackagePanel = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UMissionNavDlg* MissionNavPanel = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UMissionWeaponDlg* MissionWepPanel = nullptr;

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
    UMissionPlanner* MissionScreen = nullptr;
    EMissionBriefingMode CurrentScreen = EMissionBriefingMode::SIT;

    UFUNCTION() void HandleGameTimers();
    UFUNCTION() void HandleUniverseSecondTick(uint64 UniverseSecondsNow);
    UFUNCTION() void HandleUniverseMinuteTick(uint64 UniverseSecondsNow);
    UFUNCTION() void HandleCampaignTPlusChanged(uint64 UniverseSecondsNow, uint64 TPlusSeconds);
};