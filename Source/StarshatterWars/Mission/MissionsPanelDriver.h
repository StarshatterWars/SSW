#pragma once
#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "MissionsPanelDriver.generated.h"
class UMenuScreen;
class UCanvasPanel;
class UTextBlock;
class UButton;
class UUserWidget;
UCLASS()
class STARSHATTERWARS_API UMissionsPanelDriver : public UBaseScreen
{
    GENERATED_BODY()
public:
    void SetMenuManager(UMenuScreen* InManager) { Manager = InManager; }
    virtual void Show() override;
    virtual void HandleCancel() override;
    void ShowCampaignMode();
    void ShowSingleMode();
    void ShowCustomMode();
    void UpdateBriefingEnabled();
    void RefreshTabAccess();
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    void BuildDriverLayout();
    void RefreshTabStyles();
private:
    enum class EMissionsNavigationMode : uint8 { None, Campaign, Training, Single, Custom, Story };
    EMissionsNavigationMode ActiveMode = EMissionsNavigationMode::None;
    UPROPERTY(Transient) TObjectPtr<UButton> TrainingTab = nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> CampaignTab = nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> SingleTab = nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> CustomTab = nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> StoryTab = nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> MenuTab = nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> BriefingTab = nullptr;
    UPROPERTY(Transient) TObjectPtr<UMenuScreen> Manager = nullptr;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> ContentHost = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ModeTitle = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CurrentTimeText = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CampaignTimeText = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PlayerNameText = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PlayerGroupText = nullptr;
    UPROPERTY(Transient) TObjectPtr<UUserWidget> ActiveChild = nullptr;
    UPROPERTY(Transient) TObjectPtr<UUserWidget> TrainingChild = nullptr;
    UPROPERTY(Transient) TObjectPtr<UUserWidget> CampaignChild = nullptr;
    UPROPERTY(Transient) TObjectPtr<UUserWidget> SingleChild = nullptr;
    UPROPERTY(Transient) TObjectPtr<UUserWidget> CustomChild = nullptr;
    UPROPERTY(Transient) TObjectPtr<UUserWidget> StoryChild = nullptr;
    void SelectChild(UUserWidget* Child, const TCHAR* Title);
    void HideChildHeader(UUserWidget* Child);
    void RefreshHeaderStatus();
    bool IsCampaignTabUnlocked() const;
    bool AreStandaloneTabsUnlocked() const;
    void BindHeaderClock();
    void HandleHeaderClock(uint64 UniverseSecondsNow);
    void HandleCampaignClock(uint64 UniverseSecondsNow, uint64 TPlusSeconds);
    bool bHeaderClockBound = false;
    UFUNCTION() void OpenCampaign();
    UFUNCTION() void OpenTraining();
    UFUNCTION() void OpenSingle();
    UFUNCTION() void OpenCustom();
    UFUNCTION() void OpenStory();
    UFUNCTION() void Back();
    UFUNCTION() void OpenBriefing();
};
