/* Starshatter Wars - standalone single mission selection.
   New screen; legacy MissionSelectDlg is unchanged. */
#pragma once
#include "CoreMinimal.h"
#include "MissionSelectDlg.h"
#include "SingleMissionDlg.generated.h"

class UButton;
class UBorder;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UCanvasPanel;
class MissionInfo;
class Campaign;
class USingleMissionDlg;

/** Per-row selection callback: never infer selection from hover or focus. */
UCLASS()
class STARSHATTERWARS_API USSWCampaignMissionRowAction : public UObject
{
    GENERATED_BODY()
public:
    TWeakObjectPtr<USingleMissionDlg> Owner;
    int32 RowIndex = INDEX_NONE;
    UFUNCTION() void Activate();
};
struct FS_CampaignMission;

UCLASS()
class STARSHATTERWARS_API USingleMissionDlg : public UMissionSelectDlg
{
    GENERATED_BODY()
public:
    USingleMissionDlg(const FObjectInitializer& ObjectInitializer);
    virtual void Show() override;
    virtual void HandleAccept() override;
    virtual void HandleCancel() override;
    void SelectCampaignMissionRow(int32 RowIndex);
    virtual bool IsCustomMissionView() const { return false; }
    virtual bool IsCampaignMissionView() const { return false; }
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    void BuildSingleMissionScreen();
    void RefreshMissionRoster();
    void SelectMissionAt(int32 Index);
    void RefreshMissionDetails();
    void LaunchSelected();
    UTextBlock* MakeLabel(const FString& Value, int32 Size, const FLinearColor& Tint, bool bWrap = false);
    UBorder* MakePanel(const FLinearColor& Tint);
    UButton* MakeActionButton(const FString& Label, bool bEnabled = true, bool bRosterButton = false);
    UFUNCTION() void OnLaunchPressed();
    UFUNCTION() void OnCreateMission();
    UFUNCTION() void OnEditMission();
    UFUNCTION() void OnDeleteMission();
    UFUNCTION() void OnCustomMissionChosen();
    UFUNCTION() void OnCustomDeleteConfirmed();
    UFUNCTION() void OnCustomDeleteCanceled();
    UFUNCTION() void OnBackPressed();
    UFUNCTION() void OnCampaignTabPressed();
    UFUNCTION() void OnSingleTabPressed();
    UFUNCTION() void OnCustomTabPressed();
    UFUNCTION() void OnMultiplayerTabPressed();
    UFUNCTION() void OnMission01Pressed();
    UFUNCTION() void OnMission02Pressed();
    UFUNCTION() void OnMission03Pressed();
    UFUNCTION() void OnMission04Pressed();
    UFUNCTION() void OnMission05Pressed();
    UFUNCTION() void OnMission06Pressed();
    UFUNCTION() void OnMission07Pressed();
    UFUNCTION() void OnMission08Pressed();
    UFUNCTION() void OnMission09Pressed();
    UFUNCTION() void OnMission10Pressed();
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> MissionRoster = nullptr;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> MissionDetailsScroll = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MissionTitle = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MissionRole = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MissionLocation = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MissionTime = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MissionSummary = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MissionObjectives = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MissionCountLabel = nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> LaunchButton = nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> NewMissionButton = nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> EditMissionButton = nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> DeleteMissionButton = nullptr;
    UPROPERTY(Transient) TArray<TObjectPtr<UButton>> MissionButtons;
    UPROPERTY(Transient) TArray<TObjectPtr<USSWCampaignMissionRowAction>> CampaignRowActions;
    TArray<int32> MissionIds;
    TArray<const FS_CampaignMission*> ScenarioRows;
    TArray<MissionInfo*> CustomMissionInfos;
    Campaign* CustomCampaign = nullptr; // Owned by legacy Campaign registry.
    int32 PendingCustomDeleteId = INDEX_NONE;
    int32 ChosenIndex = INDEX_NONE;
};
