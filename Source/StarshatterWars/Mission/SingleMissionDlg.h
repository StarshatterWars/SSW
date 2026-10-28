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
class Mission;
class Campaign;
struct FS_CampaignMission;

UCLASS()
class STARSHATTERWARS_API USingleMissionRowProxy : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<USingleMissionDlg> Owner;
    int32 Index = INDEX_NONE;
    UFUNCTION() void Activate();
};

UCLASS()
class STARSHATTERWARS_API USingleMissionDlg : public UMissionSelectDlg
{
    GENERATED_BODY()
public:
    USingleMissionDlg(const FObjectInitializer& ObjectInitializer);
    virtual ~USingleMissionDlg() override;
    virtual void Show() override;
    virtual void HandleAccept() override;
    virtual void HandleCancel() override;
    virtual bool IsCustomMissionView() const { return false; }
    virtual bool IsCampaignMissionView() const { return false; }
    bool HasSelectedMission() const { return MissionIds.IsValidIndex(ChosenIndex) && MissionId > 0; }
    void SelectMissionAt(int32 Index);
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    void BuildSingleMissionScreen();
    void RefreshMissionRoster();
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
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MissionSitrep = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MissionCountLabel = nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> LaunchButton = nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> NewMissionButton = nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> EditMissionButton = nullptr;
    UPROPERTY(Transient) TObjectPtr<UButton> DeleteMissionButton = nullptr;
    UPROPERTY(Transient) TArray<TObjectPtr<UButton>> MissionButtons;
    UPROPERTY(Transient) TArray<TObjectPtr<USingleMissionRowProxy>> RowProxies;
    TArray<int32> MissionIds;
    TArray<const FS_CampaignMission*> ScenarioRows;
    TArray<MissionInfo*> CustomMissionInfos;
    TArray<Mission*> StandaloneBriefingModels; // Owned, released with selector widget.
    Mission* StandaloneBriefingMission = nullptr;
    int32 StandaloneBriefingMissionId = INDEX_NONE;
    Campaign* CustomCampaign = nullptr; // Owned by legacy Campaign registry.
    int32 PendingCustomDeleteId = INDEX_NONE;
    int32 ChosenIndex = INDEX_NONE;
};
