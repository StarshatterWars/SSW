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
#include "CmdTheaterDlg.generated.h"

class UButton;
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

class Starshatter;
class Campaign;
class StarSystem;
class MissionElement;

UCLASS()
class STARSHATTERWARS_API UCmdTheaterDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCmdTheaterDlg(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
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
    void BuildRuntimeLayout();
    void BuildMapPanels();

    UButton* CreateRuntimeButton(
        const FString& Label,
        UHorizontalBox* ParentBox,
        float Width);

    void SetViewMode(EViewMode NewMode);
    void RefreshViewButtons();

    UStarshatterEnvironmentSubsystem*
        GetEnvironmentSubsystem() const;

    StarSystem* FindRuntimeSystemByName(
        const FString& InSystemName) const;

    void EnsureDefaultSystemSelection();
    void SyncMapContext();

    void HandleGalaxySystemSelected(
        const FString& InSystemName);

    void HandleGalaxySystemActivated(
        const FString& InSystemName);

    void HandleSectorElementSelected(
        MissionElement* InElement);

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

protected:
    // -----------------------------------------------------------------
    // The ONLY required map-layout widget in CmdTheaterPanel Blueprint.
    // -----------------------------------------------------------------

    UPROPERTY(meta = (BindWidgetOptional))
    USizeBox* RuntimeHost = nullptr;

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
    UButton* GalaxyButton = nullptr;

    UPROPERTY()
    UButton* SystemButton = nullptr;

    UPROPERTY()
    UButton* SectorButton = nullptr;

    UPROPERTY()
    UButton* ZoomOutButton = nullptr;

    UPROPERTY()
    UButton* ZoomInButton = nullptr;

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

    Starshatter* Stars = nullptr;
    Campaign* CampaignPtr = nullptr;

    int32 Mode = 0;

    EViewMode CurrentViewMode = VIEW_GALAXY;
    ESelectionMode CurrentSelectionMode = SELECT_SYSTEM;

    UPROPERTY()
    FString SelectedSystemName;

    UPROPERTY()
    FString SelectedSectorName;

    // Legacy Starshatter class; not a UObject.
    MissionElement* SelectedStructureElement = nullptr;

    bool bRuntimeLayoutBuilt = false;
    bool bMapsBuilt = false;
};
