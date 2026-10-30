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
    void SetShipPreviewActive(bool bActive);

protected:
    virtual void NativeConstruct() override;
    virtual void NativePreConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;
    void BuildSituationLayout();

    UPROPERTY(Transient) TObjectPtr<class UBorder> PresentationRoot;
    UPROPERTY(Transient) TObjectPtr<class UScrollBox> SituationScroll;
    UPROPERTY(Transient) TObjectPtr<class UScrollBox> ObjectivesScroll;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PreviewStatusText;
    UPROPERTY(Transient) TObjectPtr<class UShipBriefingPreview> ShipPreview;
    bool bShipPreviewActive = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MissionObjective|Presentation")
    bool bUse3DShipPreview = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MissionObjective|Presentation")
    bool bModernBriefingText = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MissionObjective|Presentation", meta = (ClampMin = "12", ClampMax = "24"))
    int32 BriefingBodyFontSize = 18;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MissionObjective|Presentation")
    FLinearColor BriefingBodyColor = FLinearColor(0.88f, 0.93f, 0.98f, 1.f);

    void ApplyBriefingTextPresentation();

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