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
    UCmdTheaterDlg (Unreal port of CmdTheaterDlg)
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CmdDlg.h"
#include "CmdTheaterDlg.generated.h"

// UMG forward declarations
class UButton;
class UTextBlock;
class UWidget;

// Your campaign screen manager
class UCmpnScreen;

// Starshatter core forward declarations
class Starshatter;
class Campaign;

UCLASS()
class STARSHATTERWARS_API UCmdTheaterDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCmdTheaterDlg(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
    void SetManager(UCmpnScreen* InManager);

    void SetParentCmdDlg(UCmdDlg* InParentCmdDlg);
    // Show this tab/dialog (mirrors legacy Show)
    void ShowTheaterDlg();

    // Called per-frame (mirrors legacy CmdDlg::ExecFrame + map zoom)
    void ExecFrame();

protected:

    // Theater view buttons
    UFUNCTION() void OnViewGalaxyClicked();
    UFUNCTION() void OnViewSystemClicked();
    UFUNCTION() void OnViewSectorClicked();

protected:
    // Theater controls
    UPROPERTY(meta = (BindWidgetOptional)) UWidget* map_theater = nullptr; // placeholder for your map widget host
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_view_galaxy = nullptr; // legacy 401
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_view_system = nullptr; // legacy 402
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_view_sector = nullptr; // legacy 403
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_zoom_in = nullptr; // legacy 410
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_zoom_out = nullptr; // legacy 411

protected:
    UCmpnScreen* Manager = nullptr;

    Starshatter* Stars = nullptr;
    Campaign* CampaignPtr = nullptr;

    int32 Mode = 0;

    // Legacy selection/view constants preserved in ASCII
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

    int32 CurrentViewMode = VIEW_GALAXY;
    int32 CurrentSelectionMode = SELECT_SYSTEM;

    // TODO: replace with your Unreal map view class (ported MapView)
    // UPROPERTY() UObject* MapView = nullptr;

protected:
    UPROPERTY()
    UCmdDlg* ParentCmdDlg = nullptr;
};
