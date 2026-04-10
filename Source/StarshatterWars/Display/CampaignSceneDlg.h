/*  Project Starshatter Wars
    Fractal Dev Studios
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "GameStructs.h"
#include "CampaignSceneDlg.generated.h"

class UBorder;
class UOverlay;
class UTextBlock;
class UFont;
class UCmpnScreen;

UCLASS()
class STARSHATTERWARS_API UCampaignSceneDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCampaignSceneDlg(const FObjectInitializer& ObjectInitializer);

    void SetManager(UCmpnScreen* InManager) { Manager = InManager; }

    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    void Show();
    void Hide();

    void LoadSceneFromMissionData(const FS_CampaignMission& MissionData);

    // Backward-compatible wrapper so old call sites still compile:
    void LoadCaptionsFromMissionData(const FS_CampaignMission& MissionData)
    {
        LoadSceneFromMissionData(MissionData);
    }

    void BeginSceneByName(const FString& InSceneName, float InDurationSeconds);
    void AdvanceSceneFromTimer(float NowSeconds);

    bool IsSceneRunning() const { return bSceneRunning; }

protected:
    void BuildRuntimeWidgets();
    void ResetSceneState();
    void BuildSortedEventQueue();
    void ProcessPendingEvents(float ElapsedSeconds);
    void ExecuteEventBlockAtTime(double BlockTime);

    FString BuildBodyTextFromDisplayBlock(const TArray<FString>& Lines) const;
    float ResolveSceneDurationSeconds() const;

    UFont* GetRegularLimerickFont() const;
    UFont* GetBoldLimerickFont() const;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    UBorder* RuntimeHost = nullptr;

    UPROPERTY()
    UOverlay* RuntimeOverlay = nullptr;

    // Scene header from mission objective:
    UPROPERTY()
    UTextBlock* HeaderText = nullptr;

    // Current display title line:
    UPROPERTY()
    UTextBlock* MessageTitleText = nullptr;

    // Current display subtitle/body lines:
    UPROPERTY()
    UTextBlock* MessageSubtitleText = nullptr;

protected:
    UPROPERTY()
    TObjectPtr<UCmpnScreen> Manager = nullptr;

    UPROPERTY()
    FS_CampaignMission ActiveMissionData;

    UPROPERTY()
    TArray<FS_MissionEvent> SortedEvents;

    TMap<double, TArray<FString>> DisplayBlocks;
    TArray<double> SortedDisplayTimes;
    int32 NextDisplayBlockIndex = 0;

    FString ActiveSceneName;
    float SceneStartRealSeconds = 0.0f;
    float SceneDurationSeconds = 0.0f;

    bool bSceneRunning = false;
};