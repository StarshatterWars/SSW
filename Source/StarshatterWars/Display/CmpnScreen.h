/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CmpnScreen.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmpnScreen
    - Unreal port of legacy CmpnScreen (campaign screen manager).
    - Inherits UBaseScreen (UUserWidget) for unified input/flow control.
    - Creates and routes between campaign dialogs (Orders/Force/Missions/Intel/Theater).
    - Also hosts Save/Message/Complete/Cutscene dialogs.
    - Uses raw pointers (no UPROPERTY) per current porting policy.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "CmpnScreen.generated.h"

// Forward decls (Unreal dialogs):
class UCmdForceDlg;
class UCmdMissionsDlg;
class UCmdOrdersDlg;
class UCmdIntelDlg;
class UCmdTheaterDlg;

class UCmdMsgDlg;
class UCmpFileDlg;
class UCmpCompleteDlg;
class UCampaignSceneDlg;

// Forward decls (legacy singletons/objects):
class Campaign;
class Starshatter;

UCLASS()
class STARSHATTERWARS_API UCmpnScreen : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCmpnScreen(const FObjectInitializer& ObjectInitializer);

    // Minimal lifecycle:
    void Setup();
    void TearDown();

    // Visibility / flow:
    bool IsShown() const { return isShown; }
    void Show();
    void Hide();
    void HideAll();

    // Routing:
    void ShowCmdDlg();

    void ShowCmdForceDlg();
    void HideCmdForceDlg();
    bool IsCmdForceShown() const;
    UCmdForceDlg* GetCmdForceDlg() const { return cmd_force_dlg; }

    void ShowCmdMissionsDlg();
    void HideCmdMissionsDlg();
    bool IsCmdMissionsShown() const;
    UCmdMissionsDlg* GetCmdMissionsDlg() const { return cmd_missions_dlg; }

    void ShowCmdOrdersDlg();
    void HideCmdOrdersDlg();
    bool IsCmdOrdersShown() const;
    UCmdOrdersDlg* GetCmdOrdersDlg() const { return cmd_orders_dlg; }

    void ShowCmdIntelDlg();
    void HideCmdIntelDlg();
    bool IsCmdIntelShown() const;
    UCmdIntelDlg* GetCmdIntelDlg() const { return cmd_intel_dlg; }

    void ShowCmdTheaterDlg();
    void HideCmdTheaterDlg();
    bool IsCmdTheaterShown() const;
    UCmdTheaterDlg* GetCmdTheaterDlg() const { return cmd_theater_dlg; }

    void ShowCmpFileDlg();
    void HideCmpFileDlg();
    bool IsCmpFileShown() const;
    UCmpFileDlg* GetCmpFileDlg() const { return cmp_file_dlg; }

    void ShowCmdMsgDlg();
    void HideCmdMsgDlg();
    bool IsCmdMsgShown() const;
    UCmdMsgDlg* GetCmdMsgDlg() const { return cmd_msg_dlg; }

    void ShowCmpCompleteDlg();
    void HideCmpCompleteDlg();
    bool IsCmpCompleteShown() const;
    UCmpCompleteDlg* GetCmpCompleteDlg() const { return cmp_end_dlg; }

    void ShowCmpSceneDlg();
    void HideCmpSceneDlg();
    bool IsCmpSceneShown() const;
    UCampaignSceneDlg* GetCmpSceneDlg() const { return cmp_scene_dlg; }

    // Close highest-priority modal:
    bool CloseTopmost();

    float GetFieldOfView() const { return CurrentFOV; }
    void  SetFieldOfView(float InFOV);

    // Accessors used by dialogs (fixes your Manager->GetCampaign() issue):
    Campaign* GetCampaign() const { return campaign; }
    Starshatter* GetStars() const { return stars; }

public:
    // Set these from code (or later expose to BP if desired):
    TSubclassOf<UCmdOrdersDlg>    CmdOrdersDlgClass;
    TSubclassOf<UCmdForceDlg>     CmdForceDlgClass;
    TSubclassOf<UCmdMissionsDlg>  CmdMissionsDlgClass;
    TSubclassOf<UCmdIntelDlg>     CmdIntelDlgClass;
    TSubclassOf<UCmdTheaterDlg>   CmdTheaterDlgClass;

    TSubclassOf<UCmpFileDlg>      CmpFileDlgClass;
    TSubclassOf<UCmdMsgDlg>       CmdMsgDlgClass;
    TSubclassOf<UCmpCompleteDlg>    CmpCompleteDlgClass;
    TSubclassOf<UCampaignSceneDlg>  CmpSceneDlgClass;

private:
    // IMPORTANT: avoid Win32 macro collisions. Do NOT name anything CreateDialog.
    UUserWidget* MakeDlg(TSubclassOf<UUserWidget> Class);

private:
    // Dialog instances (raw pointers, per your request):
    UCmdForceDlg* cmd_force_dlg = nullptr;
    UCmdOrdersDlg* cmd_orders_dlg = nullptr;
    UCmdMissionsDlg* cmd_missions_dlg = nullptr;
    UCmdIntelDlg* cmd_intel_dlg = nullptr;
    UCmdTheaterDlg* cmd_theater_dlg = nullptr;

    UCmdMsgDlg* cmd_msg_dlg = nullptr;
    UCmpFileDlg* cmp_file_dlg = nullptr;
    UCmpCompleteDlg* cmp_end_dlg = nullptr;
    UCampaignSceneDlg* cmp_scene_dlg = nullptr;

    bool                 isShown = false;

    // Legacy pointers:
    Campaign* campaign = nullptr;
    Starshatter* stars = nullptr;

    // Stored desired FOV (degrees)
    float CurrentFOV = 90.0f;

    // Optional clamps to keep warp effects sane
    float MinFOV = 30.0f;
    float MaxFOV = 170.0f;
};
