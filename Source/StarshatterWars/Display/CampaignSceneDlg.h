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

    // Backward-compatible wrapper:
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

    void ExecuteDisplayBlockAtTime(double BlockTime);
    void ExecuteMessageEvent(const FS_MissionEvent& Event);

    FString BuildBodyTextFromDisplayBlock(const TArray<FString>& Lines) const;
    float ResolveSceneDurationSeconds() const;

    static FString FixEscapedNewlines(const FString& InText);

    UFont* GetRegularLimerickFont() const;
    UFont* GetBoldLimerickFont() const;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    UBorder* RuntimeHost = nullptr;

    UPROPERTY()
    UOverlay* RuntimeOverlay = nullptr;

    // Mission objective / scene heading:
    UPROPERTY()
    UTextBlock* HeaderText = nullptr;

    // Current display title card line:
    UPROPERTY()
    UTextBlock* MessageTitleText = nullptr;

    // Current display subtitle/body line(s):
    UPROPERTY()
    UTextBlock* MessageSubtitleText = nullptr;

    // Bottom-of-screen narration captions:
    UPROPERTY()
    UTextBlock* CaptionTextBottom = nullptr;

protected:
    UPROPERTY()
    TObjectPtr<UCmpnScreen> Manager = nullptr;

    UPROPERTY()
    FS_CampaignMission ActiveMissionData;

    UPROPERTY()
    TArray<FS_MissionEvent> SortedEvents;

    // Grouped DISPLAY lines by timestamp:
    TMap<double, TArray<FString>> DisplayBlocks;
    TArray<double> SortedDisplayTimes;
    int32 NextDisplayBlockIndex = 0;

    // Raw timed MESSAGE events with captions:
    int32 NextMessageEventIndex = 0;

    FString ActiveSceneName;
    float SceneStartRealSeconds = 0.0f;
    float SceneDurationSeconds = 0.0f;

    bool bSceneRunning = false;
};