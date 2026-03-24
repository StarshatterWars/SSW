/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         CmdTheaterDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmdTheaterDlg implementation (Unreal port)
*/

#include "CmdTheaterDlg.h"

// UMG
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

// Starshatter core
#include "Starshatter.h"
#include "Campaign.h"
#include "CombatGroup.h"
#include "FormatUtil.h"
#include "Mouse.h"

// Your screen manager
#include "CmpnScreen.h"
#include "GameStructs.h"

UCmdTheaterDlg::UCmdTheaterDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCmdTheaterDlg::NativeConstruct()
{
    Super::NativeConstruct();

    Stars = Starshatter::GetInstance();
    CampaignPtr = Campaign::GetCampaign();

    // Bind Save/Exit
    if (btn_save) btn_save->OnClicked.AddDynamic(this, &UCmdTheaterDlg::OnSaveClicked);
    if (btn_exit) btn_exit->OnClicked.AddDynamic(this, &UCmdTheaterDlg::OnExitClicked);

    // Bind tab buttons
    if (btn_orders)   btn_orders->OnClicked.AddDynamic(this, &UCmdTheaterDlg::OnModeOrdersClicked);
    if (btn_theater)  btn_theater->OnClicked.AddDynamic(this, &UCmdTheaterDlg::OnModeTheaterClicked);
    if (btn_forces)   btn_forces->OnClicked.AddDynamic(this, &UCmdTheaterDlg::OnModeForcesClicked);
    if (btn_intel)    btn_intel->OnClicked.AddDynamic(this, &UCmdTheaterDlg::OnModeIntelClicked);
    if (btn_missions) btn_missions->OnClicked.AddDynamic(this, &UCmdTheaterDlg::OnModeMissionsClicked);

    // Bind view buttons
    if (btn_view_galaxy) btn_view_galaxy->OnClicked.AddDynamic(this, &UCmdTheaterDlg::OnViewGalaxyClicked);
    if (btn_view_system) btn_view_system->OnClicked.AddDynamic(this, &UCmdTheaterDlg::OnViewSystemClicked);
    if (btn_view_sector) btn_view_sector->OnClicked.AddDynamic(this, &UCmdTheaterDlg::OnViewSectorClicked);

    // Bind zoom buttons (pressed state would normally be handled via repeatable input; clicks are a reasonable approximation)
    if (btn_zoom_in)  btn_zoom_in->OnClicked.AddDynamic(this, &UCmdTheaterDlg::OnViewGalaxyClicked); // placeholder (see ExecFrame notes)
    if (btn_zoom_out) btn_zoom_out->OnClicked.AddDynamic(this, &UCmdTheaterDlg::OnViewGalaxyClicked); // placeholder (see ExecFrame notes)

    // NOTE:
    // In legacy, zoom buttons were polled via GetButtonState() in ExecFrame.
    // In Unreal, prefer:
    //   - Enhanced Input actions for ZoomIn/ZoomOut, and/or
    //   - Bind OnPressed/OnReleased (if using UCommonButtonBase) for repeat behavior.
}

void UCmdTheaterDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ExecFrame();
}

void UCmdTheaterDlg::SetManager(UCmpnScreen* InManager)
{
    Manager = InManager;
}

void UCmdTheaterDlg::SetParentCmdDlg(UCmdDlg* InParentCmdDlg)
{
    ParentCmdDlg = InParentCmdDlg;
}

void UCmdTheaterDlg::ShowTheaterDlg()
{
    Mode = 1; // If you have UCmdDlg::ECmdMode, replace with MODE_THEATER.

    CampaignPtr = Campaign::GetCampaign();

    // Title/campaign name
    if (txt_name)
    {
        if (CampaignPtr)
            txt_name->SetText(FText::FromString(UTF8_TO_TCHAR(CampaignPtr->Name())));
        else
            txt_name->SetText(FText::FromString(TEXT("No Campaign Selected")));
    }

    // TODO: map view hookup (ported MapView)
    // if (MapView && CampaignPtr) { MapView->SetCampaign(CampaignPtr); }

    SetVisibility(ESlateVisibility::Visible);
}

void UCmdTheaterDlg::ExecFrame()
{
    if (!CampaignPtr)
        CampaignPtr = Campaign::GetCampaign();

    if (!CampaignPtr)
        return;

    // Header updates (mirrors CmdDlg::ExecFrame style you used elsewhere)
    if (txt_group)
    {
        CombatGroup* G = CampaignPtr->GetPlayerGroup();
        if (G)
            txt_group->SetText(FText::FromString(UTF8_TO_TCHAR(G->GetDescription())));
    }

    if (txt_score)
    {
        const int32 TeamScore = CampaignPtr->GetPlayerTeamScore();
        const FString ScoreStr = FString::Printf(TEXT("Team Score: %d"), TeamScore);
        txt_score->SetText(FText::FromString(ScoreStr));
        txt_score->SetJustification(ETextJustify::Right);
    }

    if (txt_time)
    {
        char DayTime[32] = { 0 };
        FormatDayTime(DayTime, CampaignPtr->GetTime());
        txt_time->SetText(FText::FromString(UTF8_TO_TCHAR(DayTime)));
    }

    // Zoom behavior (legacy polled keyboard + mouse wheel + button state)
    // In Unreal, hook these to input:
    //   - Enhanced Input axis for mouse wheel
    //   - Action mappings for +/- or keypad add/subtract
    //
    // If you already have an input layer driving zoom, call into your MapView here:
    //
    // if (bZoomIn)  MapView->ZoomIn();
    // if (bZoomOut) MapView->ZoomOut();
}

void UCmdTheaterDlg::SetModeAndHighlight(int32 InMode)
{
    Mode = InMode;

    if (!Manager)
    {
        UE_LOG(LogTemp, Warning, TEXT("CmdTheaterDlg: Manager is null (SetModeAndHighlight)."));
        return;
    }

    switch (Mode)
    {
    case 0: Manager->ShowCmdOrdersDlg();   break;
    case 1: Manager->ShowCmdTheaterDlg();  break;
    case 2: Manager->ShowCmdForceDlg();    break;
    case 3: Manager->ShowCmdIntelDlg();    break;
    case 4: Manager->ShowCmdMissionsDlg(); break;
    default: Manager->ShowCmdOrdersDlg();  break;
    }
}

void UCmdTheaterDlg::OnModeOrdersClicked() { SetModeAndHighlight(0); }
void UCmdTheaterDlg::OnModeTheaterClicked() { SetModeAndHighlight(1); }
void UCmdTheaterDlg::OnModeForcesClicked() { SetModeAndHighlight(2); }
void UCmdTheaterDlg::OnModeIntelClicked() { SetModeAndHighlight(3); }
void UCmdTheaterDlg::OnModeMissionsClicked() { SetModeAndHighlight(4); }

void UCmdTheaterDlg::OnSaveClicked()
{
    if (Manager)
        Manager->ShowCmpFileDlg();
    else
        UE_LOG(LogTemp, Warning, TEXT("CmdTheaterDlg: Manager is null (OnSaveClicked)."));
}

void UCmdTheaterDlg::OnExitClicked()
{
    if (Stars)
    {
        Mouse::Show(false);
        Stars->SetGameMode(EGameMode::MENU);
    }
}

void UCmdTheaterDlg::OnViewGalaxyClicked()
{
    CurrentViewMode = VIEW_GALAXY;
    CurrentSelectionMode = SELECT_SYSTEM;

    // TODO:
    // if (MapView) { MapView->SetViewMode(VIEW_GALAXY); MapView->SetSelectionMode(SELECT_SYSTEM); }

    // Visually latch the buttons (if desired) via SetIsEnabled/SetStyle/selected state in your UMG setup
}

void UCmdTheaterDlg::OnViewSystemClicked()
{
    CurrentViewMode = VIEW_SYSTEM;
    CurrentSelectionMode = SELECT_REGION;

    // TODO:
    // if (MapView) { MapView->SetViewMode(VIEW_SYSTEM); MapView->SetSelectionMode(SELECT_REGION); }
}

void UCmdTheaterDlg::OnViewSectorClicked()
{
    CurrentViewMode = VIEW_REGION;
    CurrentSelectionMode = SELECT_STARSHIP;

    // TODO:
    // if (MapView) { MapView->SetViewMode(VIEW_REGION); MapView->SetSelectionMode(SELECT_STARSHIP); }
}
