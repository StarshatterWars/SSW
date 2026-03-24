/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CmpnScreen.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmpnScreen
    - Unreal port of legacy CmpnScreen (campaign screen manager).
*/

#include "CmpnScreen.h"

// UMG:
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

// Dialog headers (adjust include paths to match your folder layout):
#include "CmdOrdersDlg.h"
#include "CmdForceDlg.h"
#include "CmdMissionsDlg.h"
#include "CmdIntelDlg.h"
#include "CmdTheaterDlg.h"
#include "CmpFileDlg.h"
#include "CmdMsgDlg.h"
#include "CmpCompleteDlg.h"
#include "CampaignSceneDlg.h"

// Legacy:
#include "Campaign.h"
#include "Starshatter.h"

UCmpnScreen::UCmpnScreen(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

UUserWidget* UCmpnScreen::MakeDlg(TSubclassOf<UUserWidget> Class)
{
    if (!Class)
        return nullptr;

    UWorld* World = GetWorld();
    if (!World)
        return nullptr;

    UUserWidget* W = CreateWidget<UUserWidget>(World, Class);
    if (W)
    {
        W->AddToViewport();
        W->SetVisibility(ESlateVisibility::Hidden);
    }

    return W;
}

void UCmpnScreen::Setup()
{
    stars = Starshatter::GetInstance();
    campaign = Campaign::GetCampaign();

    cmd_orders_dlg = Cast<UCmdOrdersDlg>(MakeDlg(CmdOrdersDlgClass));
    cmd_force_dlg = Cast<UCmdForceDlg>(MakeDlg(CmdForceDlgClass));
    cmd_missions_dlg = Cast<UCmdMissionsDlg>(MakeDlg(CmdMissionsDlgClass));
    cmd_intel_dlg = Cast<UCmdIntelDlg>(MakeDlg(CmdIntelDlgClass));
    cmd_theater_dlg = Cast<UCmdTheaterDlg>(MakeDlg(CmdTheaterDlgClass));

    cmp_file_dlg = Cast<UCmpFileDlg>(MakeDlg(CmpFileDlgClass));
    cmd_msg_dlg = Cast<UCmdMsgDlg>(MakeDlg(CmdMsgDlgClass));
    cmp_end_dlg = Cast<UCmpCompleteDlg>(MakeDlg(CmpCompleteDlgClass));
    cmp_scene_dlg = Cast<UCampaignSceneDlg>(MakeDlg(CmpSceneDlgClass));

    HideAll();
}

void UCmpnScreen::TearDown()
{
    auto Kill = [](UUserWidget*& W)
        {
            if (W)
            {
                W->RemoveFromParent();
                W = nullptr;
            }
        };

    UUserWidget* W = nullptr;

    W = cmd_orders_dlg;   Kill(W); cmd_orders_dlg = nullptr;
    W = cmd_force_dlg;    Kill(W); cmd_force_dlg = nullptr;
    W = cmd_missions_dlg; Kill(W); cmd_missions_dlg = nullptr;
    W = cmd_intel_dlg;    Kill(W); cmd_intel_dlg = nullptr;
    W = cmd_theater_dlg;  Kill(W); cmd_theater_dlg = nullptr;

    W = cmp_file_dlg;     Kill(W); cmp_file_dlg = nullptr;
    W = cmd_msg_dlg;      Kill(W); cmd_msg_dlg = nullptr;
    W = cmp_end_dlg;      Kill(W); cmp_end_dlg = nullptr;
    W = cmp_scene_dlg;    Kill(W); cmp_scene_dlg = nullptr;

    isShown = false;
}

void UCmpnScreen::Show()
{
    if (isShown)
        return;

    isShown = true;

    campaign = Campaign::GetCampaign();
    stars = Starshatter::GetInstance();

    ShowCmdDlg();
}

void UCmpnScreen::Hide()
{
    if (!isShown)
        return;

    HideAll();
    isShown = false;
}

void UCmpnScreen::HideAll()
{
    auto HideW = [](UUserWidget* W)
        {
            if (W)
                W->SetVisibility(ESlateVisibility::Hidden);
        };

    HideW(cmd_orders_dlg);
    HideW(cmd_force_dlg);
    HideW(cmd_missions_dlg);
    HideW(cmd_intel_dlg);
    HideW(cmd_theater_dlg);

    HideW(cmp_file_dlg);
    HideW(cmd_msg_dlg);
    HideW(cmp_end_dlg);
    HideW(cmp_scene_dlg);
}

void UCmpnScreen::ShowCmdDlg()
{
    ShowCmdOrdersDlg();
}

void UCmpnScreen::ShowCmdOrdersDlg()
{
    HideAll();
    if (cmd_orders_dlg)
        cmd_orders_dlg->SetVisibility(ESlateVisibility::Visible);
}

void UCmpnScreen::HideCmdOrdersDlg()
{
    if (cmd_orders_dlg && IsCmdOrdersShown())
        cmd_orders_dlg->SetVisibility(ESlateVisibility::Hidden);
}

bool UCmpnScreen::IsCmdOrdersShown() const
{
    return cmd_orders_dlg && cmd_orders_dlg->GetVisibility() == ESlateVisibility::Visible;
}

void UCmpnScreen::ShowCmdForceDlg()
{
    HideAll();
    if (cmd_force_dlg)
        cmd_force_dlg->SetVisibility(ESlateVisibility::Visible);
}

void UCmpnScreen::HideCmdForceDlg()
{
    if (cmd_force_dlg && IsCmdForceShown())
        cmd_force_dlg->SetVisibility(ESlateVisibility::Hidden);
}

bool UCmpnScreen::IsCmdForceShown() const
{
    return cmd_force_dlg && cmd_force_dlg->GetVisibility() == ESlateVisibility::Visible;
}

void UCmpnScreen::ShowCmdMissionsDlg()
{
    HideAll();
    if (cmd_missions_dlg)
        cmd_missions_dlg->SetVisibility(ESlateVisibility::Visible);
}

void UCmpnScreen::HideCmdMissionsDlg()
{
    if (cmd_missions_dlg && IsCmdMissionsShown())
        cmd_missions_dlg->SetVisibility(ESlateVisibility::Hidden);
}

bool UCmpnScreen::IsCmdMissionsShown() const
{
    return cmd_missions_dlg && cmd_missions_dlg->GetVisibility() == ESlateVisibility::Visible;
}

void UCmpnScreen::ShowCmdIntelDlg()
{
    HideAll();
    if (cmd_intel_dlg)
        cmd_intel_dlg->SetVisibility(ESlateVisibility::Visible);
}

void UCmpnScreen::HideCmdIntelDlg()
{
    if (cmd_intel_dlg && IsCmdIntelShown())
        cmd_intel_dlg->SetVisibility(ESlateVisibility::Hidden);
}

bool UCmpnScreen::IsCmdIntelShown() const
{
    return cmd_intel_dlg && cmd_intel_dlg->GetVisibility() == ESlateVisibility::Visible;
}

void UCmpnScreen::ShowCmdTheaterDlg()
{
    HideAll();
    if (cmd_theater_dlg)
        cmd_theater_dlg->SetVisibility(ESlateVisibility::Visible);
}

void UCmpnScreen::HideCmdTheaterDlg()
{
    if (cmd_theater_dlg && IsCmdTheaterShown())
        cmd_theater_dlg->SetVisibility(ESlateVisibility::Hidden);
}

bool UCmpnScreen::IsCmdTheaterShown() const
{
    return cmd_theater_dlg && cmd_theater_dlg->GetVisibility() == ESlateVisibility::Visible;
}

void UCmpnScreen::ShowCmpFileDlg()
{
    if (cmp_file_dlg)
        cmp_file_dlg->SetVisibility(ESlateVisibility::Visible);
}

void UCmpnScreen::HideCmpFileDlg()
{
    if (cmp_file_dlg)
        cmp_file_dlg->SetVisibility(ESlateVisibility::Hidden);
}

bool UCmpnScreen::IsCmpFileShown() const
{
    return cmp_file_dlg && cmp_file_dlg->GetVisibility() == ESlateVisibility::Visible;
}

void UCmpnScreen::ShowCmdMsgDlg()
{
    if (cmd_msg_dlg)
        cmd_msg_dlg->SetVisibility(ESlateVisibility::Visible);
}

void UCmpnScreen::HideCmdMsgDlg()
{
    if (cmd_msg_dlg)
        cmd_msg_dlg->SetVisibility(ESlateVisibility::Hidden);
}

bool UCmpnScreen::IsCmdMsgShown() const
{
    return cmd_msg_dlg && cmd_msg_dlg->GetVisibility() == ESlateVisibility::Visible;
}

void UCmpnScreen::ShowCmpCompleteDlg()
{
    HideAll();
    if (cmp_end_dlg)
        cmp_end_dlg->SetVisibility(ESlateVisibility::Visible);
}

void UCmpnScreen::HideCmpCompleteDlg()
{
    if (cmp_end_dlg && IsCmpCompleteShown())
        cmp_end_dlg->SetVisibility(ESlateVisibility::Hidden);
}

bool UCmpnScreen::IsCmpCompleteShown() const
{
    return cmp_end_dlg && cmp_end_dlg->GetVisibility() == ESlateVisibility::Visible;
}

void UCmpnScreen::ShowCmpSceneDlg()
{
    HideAll();
    if (cmp_scene_dlg)
        cmp_scene_dlg->SetVisibility(ESlateVisibility::Visible);
}

void UCmpnScreen::HideCmpSceneDlg()
{
    if (cmp_scene_dlg && IsCmpSceneShown())
        cmp_scene_dlg->SetVisibility(ESlateVisibility::Hidden);
}

bool UCmpnScreen::IsCmpSceneShown() const
{
    return cmp_scene_dlg && cmp_scene_dlg->GetVisibility() == ESlateVisibility::Visible;
}

bool UCmpnScreen::CloseTopmost()
{
    if (IsCmdMsgShown())
    {
        HideCmdMsgDlg();
        return true;
    }

    if (IsCmpFileShown())
    {
        HideCmpFileDlg();
        return true;
    }

    return false;
}

void UCmpnScreen::SetFieldOfView(float InFOV)
{
    CurrentFOV = FMath::Clamp(InFOV, MinFOV, MaxFOV);

    // Apply to the active player camera (works for “screen overlays” too):
    UWorld* World = GetWorld();
    if (!World)
        return;

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (!PC)
        return;

    if (PC->PlayerCameraManager)
    {
        PC->PlayerCameraManager->SetFOV(CurrentFOV);
    }
}
