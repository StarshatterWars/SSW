#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "MissionObjectiveDlg.generated.h"

class UTextBlock;
class UImage;
class UMissionPlanner;
class UMissionBriefingDlg;
class Mission;

UCLASS()
class STARSHATTERWARS_API UMissionObjectiveDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UMissionObjectiveDlg(const FObjectInitializer& ObjectInitializer);

    void SetManager(UMissionPlanner* InManager) { Manager = InManager; }
    void SetParentDlg(UMissionBriefingDlg* InParentDlg);

    UFUNCTION(BlueprintCallable, Category = "MissionObjective")
    void RefreshFromMission();

protected:
    virtual void NativeConstruct() override;

private:
    Mission* ResolveMission() const;
    void RefreshSituationAndObjectives(Mission* MissionPtr);
    void RefreshPlayerCaption(Mission* MissionPtr);
    void RefreshPreviewPlaceholder(Mission* MissionPtr);

private:
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* SituationText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* ObjectivesText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* PlayerCaptionText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UImage* PreviewImage = nullptr;

private:
    UPROPERTY()
    UMissionBriefingDlg* ParentDlg = nullptr;

    UPROPERTY(Transient)
    UMissionPlanner* Manager = nullptr;
};