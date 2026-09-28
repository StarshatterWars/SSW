/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         CmdTheaterDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmdTheaterDlg

    Campaign Operations / Theater map dialog.

    The Widget Blueprint is only a shell.  All Theater map controls and the
    Galaxy/System/Sector layout are constructed at runtime inside RuntimeHost,
    matching the Mission Navigation architecture.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "Types/SlateEnums.h"
#include "CmdTheaterDlg.generated.h"

class UBorder;
class UMenuButton;
class UComboBoxString;
class UHorizontalBox;
class USizeBox;
class USpacer;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;

class UCmdDlg;
class UCmpnScreen;

class UGalaxyMapPanel;
class USystemMapPanel;
class USectorMapPanel;

class UStarshatterEnvironmentSubsystem;

class APlanetActor;
class ACentralSun;
class ACameraActor;

class Starshatter;
class Campaign;
class StarSystem;
class MissionElement;
struct FS_CombatGroup;

UCLASS()
class STARSHATTERWARS_API UCmdTheaterDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCmdTheaterDlg(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(
        const FGeometry& MyGeometry,
        float InDeltaTime) override;

    virtual FReply NativeOnMouseWheel(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

public:
    void SetManager(UCmpnScreen* InManager);
    void SetParentCmdDlg(UCmdDlg* InParentCmdDlg);

    void ShowTheaterDlg();
    void ExecFrame();

protected:
    enum ESelectionMode
    {
        SELECT_NONE = -1,
        SELECT_SYSTEM = 0,
        SELECT_PLANET = 1,
        SELECT_REGION = 2,
        SELECT_STATION = 3,
        SELECT_STARSHIP = 4,
        SELECT_FIGHTER = 5
    };

    enum EViewMode
    {
        VIEW_GALAXY = 0,
        VIEW_SYSTEM = 1,
        VIEW_REGION = 2
    };

protected:
    UPROPERTY(EditAnywhere, Category = "Theater|Buttons")
    TSubclassOf<UMenuButton> TheaterMenuButtonClass;

    UFUNCTION()
    void HandleTheaterButtonSelected(UMenuButton* SelectedButton);

    void BuildRuntimeLayout();
    void BuildMapPanels();

    UMenuButton* CreateRuntimeButton(
        const FString& Label,
        UHorizontalBox* ParentBox,
        float Width);

    void SetViewMode(EViewMode NewMode);
    void SetPanelBackgroundVisible(bool bVisible);
    void RefreshViewButtons();

    void UpdateSystemPlanets();
    void ClearSystemPlanets();
    void UpdateSystemSunCamera();
    void RestoreSystemSunCamera();
    void EnsureCentralSun();
    void UpdateCentralSunVisibility();

    UStarshatterEnvironmentSubsystem*
        GetEnvironmentSubsystem() const;

    StarSystem* FindRuntimeSystemByName(
        const FString& InSystemName) const;

    void EnsureDefaultSystemSelection();
    void SyncMapContext();
    void RefreshSystemSelector();
    void RefreshRegionSelector();

    void HandleGalaxySystemSelected(
        const FString& InSystemName);

    void HandleGalaxySystemActivated(
        const FString& InSystemName);

    void HandleSystemPrimaryStarActivated(
        const FString& InSystemName);

    void HandleSectorElementSelected(
        MissionElement* InElement);

    void HandleOperationsGroupSelected(
        const FS_CombatGroup* InGroup);

protected:
    UFUNCTION()
    void OnViewGalaxyClicked();

    UFUNCTION()
    void OnViewSystemClicked();

    UFUNCTION()
    void OnViewSectorClicked();

    UFUNCTION()
    void OnZoomInClicked();

    UFUNCTION()
    void OnZoomOutClicked();

    UFUNCTION()
    void OnSystemSelectionChanged(
        FString SelectedItem,
        ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnRegionSelectionChanged(
        FString SelectedItem,
        ESelectInfo::Type SelectionType);

protected:
    // -----------------------------------------------------------------
    // Required widgets in CmdTheaterPanel Blueprint.
    // -----------------------------------------------------------------

    UPROPERTY(meta = (BindWidget))
    USizeBox* RuntimeHost = nullptr;

    UPROPERTY(meta = (BindWidget))
    UBorder* Border_0 = nullptr;

protected:
    // -----------------------------------------------------------------
    // Map widget classes
    // -----------------------------------------------------------------

    UPROPERTY(EditAnywhere, Category = "Theater|Maps")
    TSubclassOf<UGalaxyMapPanel> GalaxyMapPanelClass;

    UPROPERTY(EditAnywhere, Category = "Theater|Maps")
    TSubclassOf<USystemMapPanel> SystemMapPanelClass;

    UPROPERTY(EditAnywhere, Category = "Theater|Maps")
    TSubclassOf<USectorMapPanel> SectorMapPanelClass;

protected:
    // -----------------------------------------------------------------
    // Runtime-created Theater UI
    // -----------------------------------------------------------------

    UPROPERTY()
    UVerticalBox* RootColumn = nullptr;

    UPROPERTY()
    UHorizontalBox* TopButtonRow = nullptr;

    UPROPERTY()
    UHorizontalBox* ViewButtonBox = nullptr;

    UPROPERTY()
    UHorizontalBox* ZoomButtonBox = nullptr;

    UPROPERTY()
    UMenuButton* GalaxyButton = nullptr;

    UPROPERTY()
    UMenuButton* SystemButton = nullptr;

    UPROPERTY()
    UMenuButton* SectorButton = nullptr; // Intentionally unused: Sector navigation is a dropdown on System view.

    UPROPERTY()
    UHorizontalBox* SystemSelectorBox = nullptr;

    UPROPERTY()
    UTextBlock* SystemSelectorLabel = nullptr;

    UPROPERTY()
    USizeBox* SystemComboHost = nullptr;

    UPROPERTY()
    UComboBoxString* SystemComboBox = nullptr;

    UPROPERTY()
    UHorizontalBox* RegionSelectorBox = nullptr;

    UPROPERTY()
    UTextBlock* RegionSelectorLabel = nullptr;

    UPROPERTY()
    USizeBox* RegionComboHost = nullptr;

    UPROPERTY()
    UComboBoxString* RegionComboBox = nullptr;

    UPROPERTY()
    UMenuButton* ZoomOutButton = nullptr;

    UPROPERTY()
    UMenuButton* ZoomInButton = nullptr;

    UPROPERTY()
    USizeBox* MainViewHost = nullptr;

    UPROPERTY()
    UWidgetSwitcher* RuntimeMapSwitcher = nullptr;

    UPROPERTY()
    USizeBox* GalaxyPanelHost = nullptr;

    UPROPERTY()
    USizeBox* SystemPanelHost = nullptr;

    UPROPERTY()
    USizeBox* SectorPanelHost = nullptr;

    UPROPERTY()
    UGalaxyMapPanel* GalaxyMapPanel = nullptr;

    UPROPERTY()
    USystemMapPanel* SystemMapPanel = nullptr;

    UPROPERTY()
    USectorMapPanel* SectorMapPanel = nullptr;

protected:
    UPROPERTY(Transient)
    UCmpnScreen* Manager = nullptr;

    UPROPERTY(Transient)
    UCmdDlg* ParentCmdDlg = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<ACentralSun> CentralSun = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<ACameraActor> SystemSunCamera = nullptr;

    UPROPERTY(Transient)
    TWeakObjectPtr<AActor> PreviousSunViewTarget;

    UPROPERTY(EditAnywhere, Category = "Theater|Sun", meta = (ClampMin = "1.0"))
    float SunDiameterPixels = 64.0f;

    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<AActor>> SystemPlanetActors;

    TMap<FString, float> SystemPlanetRadii;
    FString PlanetActorSystemName;

    Starshatter* Stars = nullptr;
    Campaign* CampaignPtr = nullptr;

    int32 Mode = 0;

    EViewMode CurrentViewMode = VIEW_GALAXY;
    ESelectionMode CurrentSelectionMode = SELECT_SYSTEM;

    UPROPERTY()
    FString SelectedSystemName;

    UPROPERTY()
    FString SelectedSectorName;

    // MissionElement is retained only for the legacy/fallback callback.
    // Operations selections come from the static CombatGroupRegistry.
    MissionElement* SelectedStructureElement = nullptr;
    const FS_CombatGroup* SelectedOperationsGroup = nullptr;

    bool bUpdatingSystemSelector = false;
    bool bUpdatingRegionSelector = false;
    bool bRuntimeLayoutBuilt = false;
    bool bMapsBuilt = false;
};
