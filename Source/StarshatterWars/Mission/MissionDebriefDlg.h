#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Components/ListView.h"
#include "MissionDebriefDlg.generated.h"

class UTextBlock;
class URichTextBlock;
class UButton;
class Campaign;
class Mission;
class Sim;
class Ship;
class UMissionPlanner;

UCLASS()
class STARSHATTERWARS_API UDebriefListItem : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY() FString Col0;
    UPROPERTY() FString Col1;
    UPROPERTY() FString Col2;
    UPROPERTY() FString Col3;
    UPROPERTY() int32 DataIndex = INDEX_NONE;

    static UDebriefListItem* Make(UObject* Outer,
        const FString& In0,
        const FString& In1 = TEXT(""),
        const FString& In2 = TEXT(""),
        const FString& In3 = TEXT(""),
        int32 InDataIndex = INDEX_NONE);
};

UCLASS()
class STARSHATTERWARS_API UDebriefRowWidget : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

protected:
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
    void EnsureRow();

    UPROPERTY() TObjectPtr<UTextBlock> RowText;
};

UCLASS()
class STARSHATTERWARS_API UDebriefListView : public UListView
{
    GENERATED_BODY()

public:
    void SetEntry(TSubclassOf<UUserWidget> EntryClass) { EntryWidgetClass = EntryClass; }
};

UCLASS()
class STARSHATTERWARS_API UMissionDebriefDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UMissionDebriefDlg(const FObjectInitializer& ObjectInitializer);

    virtual void BindFormWidgets() override;
    virtual void HandleAccept() override;
    virtual void HandleCancel() override;

    void Show();
    void Hide();
    void SetManager(UMissionPlanner* InManager) { Manager = InManager; }

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UFUNCTION() void OnCloseClicked();
    UFUNCTION() void OnUnitSelectionChanged(UObject* SelectedItem);

    void ExecFrame(float DeltaSeconds);
    void DrawUnits();
    void DrawSelectedUnit(int32 StatsIndex);
    void EnsureUi();

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> CloseButtonWidget = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MissionNameWidget = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MissionSystemWidget = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MissionSectorWidget = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MissionTimeStartWidget = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<URichTextBlock> ObjectivesWidget = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<URichTextBlock> SituationWidget = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MissionScoreWidget = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UListView> UnitListWidget = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UListView> SummaryListWidget = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UListView> EventListWidget = nullptr;

    UMissionPlanner* Manager = nullptr;
    Campaign* CampaignPtr = nullptr;
    Mission* MissionPtr = nullptr;
    Sim* SimPtr = nullptr;
    Ship* PlayerShip = nullptr;
    bool bIsShown = false;
};
