/*  Project Starshatter Wars
    Fractal Dev Studios LLC
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL SYSTEM
    ===============
    Starshatter 4.5 (Destroyer Studios)

    SUBSYSTEM:    Stars.exe (Unreal Port)
    FILE:         MissionNavDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UMissionNavDlg

    Navigation panel hosted by UMissionBriefingDlg.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "MissionNavObjectListObject.h"
#include "MissionNavDlg.generated.h"

class UBorder;
class UButton;
class UGalaxyMapPanel;
class UHorizontalBox;
class UMenuButton;
class UScrollBox;
class USectorMapPanel;
class USizeBox;
class USystemMapPanel;
class UTextBlock;
class UTexture2D;
class UUniformGridPanel;
class UUserWidget;
class UVerticalBox;
class UWidgetSwitcher;

class UMissionBriefingDlg;
class UMissionPlanner;
class UMissionNavObjectListView;

class Campaign;
class MapView;
class Mission;
class MissionElement;
class MissionInfo;
class OrbitalBody;
class OrbitalRegion;
class StarSystem;
class UStarshatterEnvironmentSubsystem;

UENUM()
enum class EMissionNavMode : uint8
{
    GALAXY = 0,
    SYSTEM,
    SECTOR
};

UENUM()
enum class EMissionNavFilterMode : uint8
{
    SYSTEM = 0,
    PLANET,
    SECTOR,
    STATION,
    STARSHIP,
    FIGHTER
};

UCLASS()
class STARSHATTERWARS_API UMissionNavDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UMissionNavDlg(const FObjectInitializer& ObjectInitializer);

    void SetManager(UMissionPlanner* InManager) { Manager = InManager; }
    void SetParentDlg(UMissionBriefingDlg* InParentDlg);

    void RefreshFromMission();

    UFUNCTION()
    void HandleGalaxySystemSelected(const FString& InSystemName);

    void HandleGalaxySystemActivated(const FString& InSystemName);

    const FString& GetSelectedGalaxySystemName() const { return SelectedSystemName; }
    void HandleSectorMissionElementSelected(MissionElement* InElement);

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

    virtual FReply NativeOnMouseWheel(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

    virtual FReply NativeOnMouseButtonDown(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

    virtual FReply NativeOnMouseButtonUp(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

    virtual FReply NativeOnMouseMove(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

    Mission* ResolveMission() const;

    void BuildRuntimeLayout();
    void BuildNavModeButtons();
    void BuildRightPanels();
    void BuildFilterButtons();

    void ApplyPanelStyles();

    void RefreshNavModeSelection();
    void RefreshFilterSelection();
    void RefreshObjectListPanel();
    void RefreshDetailPanel();

    void RebuildObjectList();

    void SetNavMode(EMissionNavMode NewMode);
    void SetFilterMode(EMissionNavFilterMode NewMode);

    FString GetFilterModeLabel(EMissionNavFilterMode Mode) const;
    FString GetObjectPanelTitle() const;
    FString GetDetailPanelTitle() const;

    EMissionNavObjectType GetCurrentObjectType() const;

    UMenuButton* CreateNavModeButton(const FString& Label, UHorizontalBox* ParentBox);
    UMenuButton* CreateFilterButton(const FString& Label, int32 Row, int32 Column);

    void BuildSystemObjects();
    void BuildPlanetObjects();
    void BuildSectorObjects();
    void BuildMissionElementObjects(EMissionNavObjectType ObjectType);
    bool ShouldShowMissionElementInBriefing(const MissionElement* Elem) const;

    void AddObjectItem(
        EMissionNavObjectType ObjectType,
        int32 Index,
        const FString& PrimaryText,
        const FString& SecondaryText,
        const FString& DetailText);

    TArray<FString> FindShortestGalaxyRoute(
        const FString& StartSystem,
        const FString& GoalSystem) const;

    void UpdateSystemDetailsPanel(const FString& InSystemName);
    void SyncGalaxyMissionAndSelectionState();
    void SyncSubPanels();
    FString BuildGalaxySystemDetailText(const FString& InSystemName) const;

    UStarshatterEnvironmentSubsystem* GetEnvironmentSubsystem() const;
    StarSystem* FindRuntimeSystemByName(const FString& InSystemName) const;
    TArray<FString> GetRuntimeLinkedSystemNames(StarSystem* InSystem) const;

protected:
    UPROPERTY()
    UMissionBriefingDlg* ParentDlg = nullptr;

    UPROPERTY(Transient)
    UMissionPlanner* Manager = nullptr;

    UPROPERTY(EditAnywhere, Category = "Mission Nav")
    TSubclassOf<UGalaxyMapPanel> GalaxyMapPanelClass;

    UPROPERTY(EditAnywhere, Category = "Mission Nav")
    TSubclassOf<USystemMapPanel> SystemMapPanelClass;

    UPROPERTY(EditAnywhere, Category = "Mission Nav")
    TSubclassOf<USectorMapPanel> SectorMapPanelClass;

private:
    Campaign* CampaignPtr = nullptr;
    Mission* MissionPtr = nullptr;
    MissionInfo* MissionInfoPtr = nullptr;
    MapView* MapViewPtr = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USizeBox* RuntimeHost = nullptr;

    UPROPERTY()
    UHorizontalBox* RootContentRow = nullptr;

    UPROPERTY()
    UBorder* LeftPanelBorder = nullptr;

    UPROPERTY()
    UVerticalBox* LeftPanelColumn = nullptr;

    UPROPERTY()
    UHorizontalBox* TopButtonRow = nullptr;

    UPROPERTY()
    UHorizontalBox* NavModeButtonBox = nullptr;

    UPROPERTY()
    UHorizontalBox* ZoomButtonBox = nullptr;

    UPROPERTY()
    USizeBox* MainViewHost = nullptr;

    UPROPERTY()
    UBorder* RightPanelBorder = nullptr;

    UPROPERTY()
    UVerticalBox* RightPanelColumn = nullptr;

    UPROPERTY(EditAnywhere, Category = "Mission Nav")
    TSubclassOf<UMenuButton> MenuButtonClass;

    UPROPERTY(EditAnywhere, Category = "Mission Nav")
    TSubclassOf<UUserWidget> ObjectListEntryWidgetClass;

    UPROPERTY()
    TArray<TObjectPtr<UMenuButton>> NavModeButtons;

    UPROPERTY()
    UButton* ZoomOutButton = nullptr;

    UPROPERTY()
    UButton* ZoomInButton = nullptr;

    UPROPERTY()
    UTextBlock* ZoomOutText = nullptr;

    UPROPERTY()
    UTextBlock* ZoomInText = nullptr;

    UPROPERTY()
    UWidgetSwitcher* NavSwitcher = nullptr;

    UPROPERTY()
    USizeBox* GalaxyPanelHost = nullptr;

    UPROPERTY()
    UGalaxyMapPanel* GalaxyMapPanel = nullptr;

    UPROPERTY()
    USizeBox* SystemPanelHost = nullptr;

    UPROPERTY()
    USystemMapPanel* SystemMapPanel = nullptr;

    UPROPERTY()
    USizeBox* SectorPanelHost = nullptr;

    UPROPERTY()
    USectorMapPanel* SectorMapPanel = nullptr;

    UPROPERTY()
    UTextBlock* NavBodyText = nullptr;

    UPROPERTY()
    UVerticalBox* FilterPanelHost = nullptr;

    UPROPERTY()
    UUniformGridPanel* FilterButtonGrid = nullptr;

    UPROPERTY()
    TArray<TObjectPtr<UMenuButton>> FilterButtons;

    UPROPERTY()
    UBorder* ObjectListBorder = nullptr;

    UPROPERTY()
    UVerticalBox* ObjectListPanel = nullptr;

    UPROPERTY()
    UBorder* ObjectListTitleBar = nullptr;

    UPROPERTY()
    UTextBlock* ObjectListTitleText = nullptr;

    UPROPERTY()
    USizeBox* ObjectListHost = nullptr;

    UPROPERTY()
    UMissionNavObjectListView* ObjectListView = nullptr;

    UPROPERTY()
    TArray<TObjectPtr<UMissionNavObjectListObject>> ObjectItems;

    UPROPERTY()
    UMissionNavObjectListObject* SelectedObjectItem = nullptr;

    UPROPERTY()
    UBorder* DetailBorder = nullptr;

    UPROPERTY()
    UVerticalBox* DetailPanel = nullptr;

    UPROPERTY()
    UBorder* DetailTitleBar = nullptr;

    UPROPERTY()
    UTextBlock* DetailTitleText = nullptr;

    UPROPERTY()
    USizeBox* DetailHost = nullptr;

    UPROPERTY()
    UScrollBox* DetailScrollBox = nullptr;

    UPROPERTY()
    UTextBlock* DetailBodyText = nullptr;

    UPROPERTY()
    UTexture2D* RightPanelBackgroundTexture = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MissionNav|State", meta = (AllowPrivateAccess = "true"))
    EMissionNavMode CurrentNavMode = EMissionNavMode::SYSTEM;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MissionNav|State", meta = (AllowPrivateAccess = "true"))
    EMissionNavFilterMode CurrentFilterMode = EMissionNavFilterMode::SYSTEM;

private:
    UFUNCTION()
    void OnNavModeButtonSelected(UMenuButton* SelectedButton);

    UFUNCTION()
    void OnNavModeButtonHovered(UMenuButton* HoveredButton);

    UFUNCTION()
    void OnFilterButtonSelected(UMenuButton* SelectedButton);

    UFUNCTION()
    void OnFilterButtonHovered(UMenuButton* HoveredButton);

    UFUNCTION()
    void OnObjectSelectionChanged(UObject* SelectedItem);

    UFUNCTION()
    void OnZoomInClicked();

    UFUNCTION()
    void OnZoomOutClicked();

    UPROPERTY()
    FString SelectedSystemName;

    UPROPERTY()
    FString CurrentMissionSystemName;
};