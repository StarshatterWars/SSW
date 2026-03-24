/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         CmdForceDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmdForceDlg implementation (Unreal port)
*/

#include "CmdForceDlg.h"

// UMG:
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Components/ListView.h"

// Starshatter core:
#include "Starshatter.h"
#include "Campaign.h"
#include "Combatant.h"
#include "CombatGroup.h"
#include "CombatUnit.h"
#include "PlayerCharacter.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Weapon.h"
#include "FormatUtil.h"
#include "Mouse.h"
#include "UIButton.h"
#include "CmdMsgDlg.h"
#include "GameStructs.h"

// Your campaign screen:
#include "CmpnScreen.h"

namespace
{
    struct FWepGroup
    {
        FString Name;
        int32 Count = 0;
    };

    static FWepGroup* FindWepGroup(TArray<FWepGroup>& Groups, const FString& Name)
    {
        // up to 8 groups like legacy
        for (FWepGroup& G : Groups)
        {
            if (!G.Name.IsEmpty() && G.Name.Equals(Name, ESearchCase::IgnoreCase))
                return &G;
        }

        for (FWepGroup& G : Groups)
        {
            if (G.Name.IsEmpty())
            {
                G.Name = Name;
                return &G;
            }
        }

        return nullptr;
    }
}

UCmdForceDlg::UCmdForceDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCmdForceDlg::NativeConstruct()
{
    Super::NativeConstruct();

    BindFormWidgets();

    // Cache pointers
    Stars = Starshatter::GetInstance();
    CampaignPtr = Campaign::GetCampaign();

    // Bind clicks (no AddLambda on UButton OnClicked)
    if (btn_save)     btn_save->OnClicked.AddDynamic(this, &UCmdForceDlg::OnSaveClicked);
    if (btn_exit)     btn_exit->OnClicked.AddDynamic(this, &UCmdForceDlg::OnExitClicked);

    if (btn_orders)   btn_orders->OnClicked.AddDynamic(this, &UCmdForceDlg::OnModeOrdersClicked);
    if (btn_theater)  btn_theater->OnClicked.AddDynamic(this, &UCmdForceDlg::OnModeTheaterClicked);
    if (btn_forces)   btn_forces->OnClicked.AddDynamic(this, &UCmdForceDlg::OnModeForcesClicked);
    if (btn_intel)    btn_intel->OnClicked.AddDynamic(this, &UCmdForceDlg::OnModeIntelClicked);
    if (btn_missions) btn_missions->OnClicked.AddDynamic(this, &UCmdForceDlg::OnModeMissionsClicked);

    if (cmb_forces)
        cmb_forces->OnSelectionChanged.AddDynamic(this, &UCmdForceDlg::OnForceSelectionChanged);

    if (btn_transfer)
        btn_transfer->OnClicked.AddDynamic(this, &UCmdForceDlg::OnTransferClicked);

    if (btn_transfer)
        btn_transfer->SetIsEnabled(false);

    // Parse/apply legacy form defaults if you use it:
    const FString Frm = GetLegacyFormText();
    if (!Frm.IsEmpty())
    {
        FString Err;
        FParsedForm Parsed;
        if (ParseLegacyForm(Frm, Parsed, Err))
        {
            ParsedForm = Parsed;
            ApplyLegacyFormDefaults(ParsedForm);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("CmdForceDlg: ParseLegacyForm failed: %s"), *Err);
        }
    }
}

void UCmdForceDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ExecFrame();
}

// --------------------------------------------------------------------
// UBaseScreen overrides
// --------------------------------------------------------------------

void UCmdForceDlg::BindFormWidgets()
{
    // Shared header:
    BindLabel(200, txt_group);
    BindLabel(201, txt_score);
    BindLabel(300, txt_name);
    BindLabel(301, txt_time);

    BindButton(100, btn_orders);
    BindButton(101, btn_theater);
    BindButton(102, btn_forces);
    BindButton(103, btn_intel);
    BindButton(104, btn_missions);

    BindButton(1, btn_save);
    BindButton(2, btn_exit);

    // Forces tab:
    BindCombo(400, cmb_forces);
    BindList(401, lst_combat);
    BindList(402, lst_desc);
    BindButton(403, btn_transfer);
}


void UCmdForceDlg::SetParentCmdDlg(UCmdDlg* InParentCmdDlg)
{
    ParentCmdDlg = InParentCmdDlg;
}

FString UCmdForceDlg::GetLegacyFormText() const
{
    // Same guidance as before: return a single form block, not multiple.
    // You pasted ONE CmdForceDlg.frm block, so you can embed it here if desired.
    return FString();
}

// --------------------------------------------------------------------
// Public API
// --------------------------------------------------------------------

void UCmdForceDlg::SetManager(UCmpnScreen* InManager)
{
    Manager = InManager;
}

void UCmdForceDlg::ShowForceDlg()
{
    Mode = ECOMMAND_MODE::MODE_FORCES;
    CampaignPtr = Campaign::GetCampaign();

    // Title/campaign name:
    if (txt_name)
    {
        if (CampaignPtr)
            txt_name->SetText(FText::FromString(CampaignPtr->Name()));
        else
            txt_name->SetText(FText::FromString(TEXT("No Campaign Selected")));
    }

    // Enable/disable based on training:
    if (CampaignPtr)
    {
        const bool bTraining = CampaignPtr->IsTraining();

        if (btn_save)   btn_save->SetIsEnabled(!bTraining);
        if (btn_forces) btn_forces->SetIsEnabled(!bTraining);
        if (btn_intel)  btn_intel->SetIsEnabled(!bTraining);
    }

    // Populate forces combo:
    if (cmb_forces)
    {
        cmb_forces->ClearOptions();

        if (CampaignPtr)
        {
            const List<Combatant>& Combatants = CampaignPtr->GetCombatants();
            for (int i = 0; i < Combatants.size(); ++i)
            {
                Combatant* C = Combatants[i];
                if (IsVisibleCombatant(C))
                    cmb_forces->AddOption(UTF8_TO_TCHAR(C->GetName()));
            }

            // Select first visible combatant:
            if (cmb_forces->GetOptionCount() > 0)
            {
                cmb_forces->SetSelectedIndex(0);
                const FString Name = cmb_forces->GetSelectedOption();

                // resolve combatant by name:
                const List<Combatant>& All = CampaignPtr->GetCombatants();
                Combatant* Found = All.size() ? All[0] : nullptr;

                for (int i = 0; i < All.size(); ++i)
                {
                    Combatant* C = All[i];
                    if (C && Name.Equals(UTF8_TO_TCHAR(C->GetName())))
                    {
                        Found = C;
                        break;
                    }
                }

                CurrentCombatant = Found;
                ShowCombatant(CurrentCombatant);
            }
        }
    }

    SetVisibility(ESlateVisibility::Visible);
}

void UCmdForceDlg::ExecFrame()
{
    if (!CampaignPtr)
        CampaignPtr = Campaign::GetCampaign();

    if (!CampaignPtr)
        return;

    // Mirrors CmdDlg::ExecFrame() for header data:
    if (txt_group)
    {
        CombatGroup* G = CampaignPtr->GetPlayerGroup();
        if (G)
            txt_group->SetText(FText::FromString(G->GetDescription()));
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

    // Intel unread count label update (optional):
    const int32 Unread = CampaignPtr->CountNewEvents();
    if (txt_btn_intel)
    {
        if (Unread > 0)
            txt_btn_intel->SetText(FText::FromString(FString::Printf(TEXT("INTEL (%d)"), Unread)));
        else
            txt_btn_intel->SetText(FText::FromString(TEXT("INTEL")));
    }

    UpdateTransferEnabled();
}

// --------------------------------------------------------------------
// Tab routing
// --------------------------------------------------------------------

void UCmdForceDlg::SetModeAndHighlight(ECOMMAND_MODE InMode)
{
    Mode = InMode;

    if (!Manager)
    {
        UE_LOG(LogTemp, Warning, TEXT("CmdForceDlg: Manager is null (SetModeAndHighlight)."));
        return;
    }

    switch (Mode)
    {
    case ECOMMAND_MODE::MODE_ORDERS:   Manager->ShowCmdOrdersDlg();   break;
    case ECOMMAND_MODE::MODE_THEATER:  Manager->ShowCmdTheaterDlg();  break;
    case ECOMMAND_MODE::MODE_FORCES:   Manager->ShowCmdForceDlg();    break;
    case ECOMMAND_MODE::MODE_INTEL:    Manager->ShowCmdIntelDlg();    break;
    case ECOMMAND_MODE::MODE_MISSIONS: Manager->ShowCmdMissionsDlg(); break;
    default:                               Manager->ShowCmdOrdersDlg();   break;
    }
}

void UCmdForceDlg::OnModeOrdersClicked() { SetModeAndHighlight(ECOMMAND_MODE::MODE_ORDERS); }
void UCmdForceDlg::OnModeTheaterClicked() { SetModeAndHighlight(ECOMMAND_MODE::MODE_THEATER); }
void UCmdForceDlg::OnModeForcesClicked() { SetModeAndHighlight(ECOMMAND_MODE::MODE_FORCES); }
void UCmdForceDlg::OnModeIntelClicked() { SetModeAndHighlight(ECOMMAND_MODE::MODE_INTEL); }
void UCmdForceDlg::OnModeMissionsClicked() { SetModeAndHighlight(ECOMMAND_MODE::MODE_MISSIONS); }

void UCmdForceDlg::OnSaveClicked()
{
    if (Manager)
        Manager->ShowCmpFileDlg();
    else
        UE_LOG(LogTemp, Warning, TEXT("CmdForceDlg: Manager is null (OnSaveClicked)."));
}

void UCmdForceDlg::OnExitClicked()
{
    if (Stars)
    {
        Mouse::Show(false);
        Stars->SetGameMode(EGameMode::MENU);
    }
}

// --------------------------------------------------------------------
// Forces selection / list interaction
// --------------------------------------------------------------------

void UCmdForceDlg::OnForceSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (!CampaignPtr)
        CampaignPtr = Campaign::GetCampaign();

    if (!CampaignPtr)
        return;

    // Resolve combatant by name:
    Combatant* Found = nullptr;
    ListIter<Combatant> Iter = CampaignPtr->GetCombatants();
    while (++Iter)
    {
        Combatant* C = Iter.value();
        if (C && SelectedItem.Equals(UTF8_TO_TCHAR(C->GetName())))
        {
            Found = C;
            break;
        }
    }

    CurrentCombatant = Found;
    ShowCombatant(CurrentCombatant);
}

bool UCmdForceDlg::IsVisibleCombatant(Combatant* C) const
{
    int32 VisibleCount = 0;

    if (C)
    {
        CombatGroup* Force = C->GetForce();
        if (Force)
        {
            List<CombatGroup>& Groups = Force->GetComponents();
            for (int i = 0; i < Groups.size(); ++i)
            {
                CombatGroup* G = Groups[i];
                if (G &&
                    G->GetType() < ECOMBATGROUP_TYPE::CIVILIAN &&
                    G->CountUnits() > 0 &&
                    G->IntelLevel() >= Intel::KNOWN)
                {
                    ++VisibleCount;
                }
            }
        }
    }

    return VisibleCount > 0;
}

void UCmdForceDlg::ShowCombatant(Combatant* C)
{
    if (!lst_combat || !C)
        return;

    CurrentGroup = nullptr;
    CurrentUnit = nullptr;

    PipeStack.Empty();
    bBlankLine = false;

    // Clear combat list (ListView):
    lst_combat->ClearListItems();

    CombatGroup* Force = C->GetForce();
    if (Force)
    {
        List<CombatGroup>& Groups = Force->GetComponents();
        for (int i = 0; i < Groups.size(); ++i)
        {
            CombatGroup* G = Groups[i];
            if (G && G->GetType() < ECOMBATGROUP_TYPE::CIVILIAN && G->CountUnits() > 0)
                AddCombatGroupRecursive(G, i == Groups.size() - 1);
        }
    }

    ClearDescList();

    if (btn_transfer)
        btn_transfer->SetIsEnabled(false);
}

void UCmdForceDlg::ClearDescList()
{
    if (lst_desc)
        lst_desc->ClearListItems();
}

// NOTE:
// The legacy listbox stored pointer data per row and a "type" column (0 group, 1 unit),
// plus it drew pipe glyphs and clicked on the plus/minus region.
// In Unreal, the correct approach is:
//   - Make a UObject row item (e.g., UCmdForceListItem) holding:
//        - DisplayText
//        - RowType (Group/Unit/Blank)
//        - GroupPtr / UnitPtr
//        - bHasExpandGlyph / bExpanded
//   - Make the row widget handle click on the expand glyph and selection.
// I am keeping the logic intact, but you will need those row item classes to finish UI behavior.

void UCmdForceDlg::AddCombatGroupRecursive(CombatGroup* Group, bool bLastChild)
{
    if (!Group || Group->IntelLevel() < Intel::KNOWN || !lst_combat)
        return;

    // Build prefix similar to legacy (conceptual; real glyph rendering should be in the row widget)
    FString Prefix;

    const bool bTopLevel = (!Group->GetParent() || Group->GetParent()->GetType() == ECOMBATGROUP_TYPE::FORCE);

    if (bTopLevel)
    {
        Prefix = Group->IsExpanded() ? TEXT("[-] ") : TEXT("[+] ");
    }
    else
    {
        // child marker (ASCII only)
        Prefix = bLastChild ? TEXT("\\-") : TEXT("+-");

        const bool bHasChildrenOrUnits =
            Group->GetLiveComponents().size() > 0 || Group->GetUnits().size() > 0;

        if (bHasChildrenOrUnits)
            Prefix += Group->IsExpanded() ? TEXT("[-] ") : TEXT("[+] ");
        else
            Prefix += TEXT("   ");
    }

    const FString Line = PipeStack + Prefix + UTF8_TO_TCHAR(Group->GetDescription());

    // TODO: replace with your row item object:
    // lst_combat->AddItem(NewObject<UCmdForceListItem>(...));
    // For now, we cannot add plain strings to UListView without an item class.

    bBlankLine = false;

    // Update pipe stack (ASCII only)
    const int32 PrevLen = PipeStack.Len();

    if (!bTopLevel)
    {
        PipeStack += (bLastChild ? TEXT("  ") : TEXT("| "));
    }

    // Units
    if (Group->IsExpanded() && Group->GetUnits().size() > 0)
    {
        ListIter<CombatUnit> UnitIter = Group->GetUnits();
        while (++UnitIter)
        {
            CombatUnit* Unit = UnitIter.value();
            if (!Unit) continue;

            const ShipDesign* Design = Unit->GetDesign();
            const int32 Integrity = Design ? (int32)Design->integrity : 1;
            const int32 DamagePct = (int32)(100.0 * Unit->GetSustainedDamage() / (double)Integrity);

            FString UnitLine = PipeStack + TEXT("  ") + UTF8_TO_TCHAR(Unit->GetDescription());
            if (DamagePct >= 1 && Unit->DeadCount() < Unit->Count())
                UnitLine += FString::Printf(TEXT(" %d%% damage"), DamagePct);

            // TODO: add unit row item to lst_combat (type=Unit)
        }

        // TODO: add blank line item after units
        bBlankLine = true;
    }

    // Child groups
    if (Group->IsExpanded() && Group->GetLiveComponents().size() > 0)
    {
        List<CombatGroup>& Groups = Group->GetLiveComponents();
        for (int i = 0; i < Groups.size(); ++i)
        {
            AddCombatGroupRecursive(Groups[i], i == Groups.size() - 1);
        }

        // TODO: blank line after last group if needed
        if (!bBlankLine)
            bBlankLine = true;
    }

    // Pop pipe stack
    PipeStack.LeftInline(PrevLen);
}

bool UCmdForceDlg::CanTransfer(CombatGroup* Group) const
{
    if (!Group || !CampaignPtr)
        return false;

    if (Group->GetType() < ECOMBATGROUP_TYPE::WING)
        return false;

    if (Group->GetType() > ECOMBATGROUP_TYPE::CARRIER_GROUP)
        return false;

    if (Group->GetType() == ECOMBATGROUP_TYPE::FLEET || Group->GetType() == ECOMBATGROUP_TYPE::LCA_SQUADRON)
        return false;

    CombatGroup* PlayerGroup = CampaignPtr->GetPlayerGroup();
    if (!PlayerGroup || PlayerGroup->GetIFF() != Group->GetIFF())
        return false;

    return true;
}

void UCmdForceDlg::UpdateTransferEnabled()
{
    if (!btn_transfer || !CampaignPtr || !CurrentGroup)
        return;

    const bool bEnable = CampaignPtr->IsActive() && CanTransfer(CurrentGroup);
    btn_transfer->SetIsEnabled(bEnable);
}

void UCmdForceDlg::OnTransferClicked()
{
    if (!CampaignPtr || !CurrentGroup || !Manager)
        return;

    PlayerCharacter* PlayerPtr = PlayerCharacter::GetCurrentPlayer();
    if (!PlayerPtr)
        return;

    int CmdClass = (int)CLASSIFICATION::FIGHTER;

    switch (CurrentGroup->GetType())
    {
    case ECOMBATGROUP_TYPE::WING:
    case ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON:
    case ECOMBATGROUP_TYPE::FIGHTER_SQUADRON:
        CmdClass = (int)CLASSIFICATION::FIGHTER;
        break;

    case ECOMBATGROUP_TYPE::ATTACK_SQUADRON:
        CmdClass = (int)CLASSIFICATION::ATTACK;
        break;

    case ECOMBATGROUP_TYPE::LCA_SQUADRON:
        CmdClass = (int)CLASSIFICATION::LCA;
        break;

    case ECOMBATGROUP_TYPE::DESTROYER_SQUADRON:
        CmdClass = (int)CLASSIFICATION::DESTROYER;
        break;

    case ECOMBATGROUP_TYPE::BATTLE_GROUP:
        CmdClass = (int)CLASSIFICATION::CRUISER;
        break;

    case ECOMBATGROUP_TYPE::CARRIER_GROUP:
    case ECOMBATGROUP_TYPE::FLEET:
        CmdClass = (int)CLASSIFICATION::CARRIER;
        break;
    }

    FString TransferInfo;

    UCmdMsgDlg* MsgDlg = Manager->GetCmdMsgDlg();
    if (!MsgDlg)
        return;

    if (PlayerPtr->CanCommand(CmdClass))
    {
        if (CurrentUnit)
        {
            CampaignPtr->SetPlayerUnit(CurrentUnit);
            TransferInfo = FString::Printf(
                TEXT("Your transfer request has been approved, %s %s.  You are now assigned to the %s.  Good luck.\n\nFleet Admiral A. Evars FORCOM\nCommanding"),
                UTF8_TO_TCHAR(PlayerCharacter::RankName(PlayerPtr->GetRank())),
                UTF8_TO_TCHAR(*PlayerPtr->Name()),
                UTF8_TO_TCHAR(CurrentUnit->GetDescription()));
        }
        else
        {
            CampaignPtr->SetPlayerGroup(CurrentGroup);
            TransferInfo = FString::Printf(
                TEXT("Your transfer request has been approved, %s %s.  You are now assigned to the %s.  Good luck.\n\nFleet Admiral A. Evars FORCOM\nCommanding"),
                UTF8_TO_TCHAR(PlayerCharacter::RankName(PlayerPtr->GetRank())),
                UTF8_TO_TCHAR(*PlayerPtr->Name()),
                UTF8_TO_TCHAR(CurrentGroup->GetDescription()));
        }

        UIButton::PlaySound(UIButton::SND_ACCEPT);

        MsgDlg->SetTitleText(TEXT("Transfer Approved"));
        MsgDlg->SetMessageText(TransferInfo);
        Manager->ShowCmdMsgDlg();
    }
    else
    {
        UIButton::PlaySound(UIButton::SND_REJECT);

        const char* Required = PlayerCharacter::RankName(PlayerCharacter::CommandRankRequired(CmdClass));
        TransferInfo = FString::Printf(
            TEXT("Your transfer request has been denied, %s %s.  The %s requires a command rank of %s.  Please return to your unit and your duties.\n\nFleet Admiral A. Evars FORCOM\nCommanding"),
            UTF8_TO_TCHAR(PlayerCharacter::RankName(PlayerPtr->GetRank())),
            UTF8_TO_TCHAR(*PlayerPtr->Name()),
            UTF8_TO_TCHAR(CurrentGroup->GetDescription()),
            UTF8_TO_TCHAR(Required));

        MsgDlg->SetTitleText(TEXT("Transfer Denied"));
        MsgDlg->SetMessageText(TransferInfo);
        Manager->ShowCmdMsgDlg();
    }
}
