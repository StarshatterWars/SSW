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
#include "CmdDlg.h"
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

    // Cache pointers
    Stars = Starshatter::GetInstance();
    CampaignPtr = Campaign::GetCampaign();

    if (ForcesComboBox)
        ForcesComboBox->OnSelectionChanged.AddDynamic(this, &UCmdForceDlg::OnForceSelectionChanged);

    if (TransferButton)
        TransferButton->OnClicked.AddDynamic(this, &UCmdForceDlg::OnTransferClicked);

    if (TransferButton)
        TransferButton->SetIsEnabled(false);
}

void UCmdForceDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ExecFrame();
}

void UCmdForceDlg::SetParentCmdDlg(UCmdDlg* InParentCmdDlg)
{
    ParentCmdDlg = InParentCmdDlg;
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

    // Populate forces combo:
    if (ForcesComboBox)
    {
        ForcesComboBox->ClearOptions();

        if (CampaignPtr)
        {
            const List<Combatant>& Combatants = CampaignPtr->GetCombatants();
            for (int i = 0; i < Combatants.size(); ++i)
            {
                Combatant* C = Combatants[i];
                if (IsVisibleCombatant(C))
                    ForcesComboBox->AddOption(UTF8_TO_TCHAR(C->GetName()));
            }

            // Select first visible combatant:
            if (ForcesComboBox->GetOptionCount() > 0)
            {
                ForcesComboBox->SetSelectedIndex(0);
                const FString Name = ForcesComboBox->GetSelectedOption();

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
                    G->GetIntelLevel() >= Intel::KNOWN)
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
    if (!CombatantList || !C)
        return;

    CurrentGroup = nullptr;
    CurrentUnit = nullptr;

    PipeStack.Empty();
    bBlankLine = false;

    // Clear combat list (ListView):
    CombatantList->ClearListItems();

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

    if (TransferButton)
        TransferButton->SetIsEnabled(false);
}

void UCmdForceDlg::ClearDescList()
{
    if (DescriptionList)
        DescriptionList->ClearListItems();

    if (GroupInfoText)     GroupInfoText->SetText(FText::GetEmpty());
    if (GroupTypeText)     GroupTypeText->SetText(FText::GetEmpty());
    if (GroupLocationText) GroupLocationText->SetText(FText::GetEmpty());
    if (GroupEmpireText)   GroupEmpireText->SetText(FText::GetEmpty());
}

void UCmdForceDlg::PopulateDescForGroup(CombatGroup* Group)
{
    if (!Group)
    {
        ClearDescList();
        return;
    }

    // ------------------------------------------------------------
    // NAME / DESCRIPTION
    // ------------------------------------------------------------
    if (GroupInfoText)
    {
        GroupInfoText->SetText(FText::FromString(
            UTF8_TO_TCHAR(Group->GetDescription())));
    }

    // ------------------------------------------------------------
    // TYPE
    // ------------------------------------------------------------
    if (GroupTypeText)
    {
        const char* TypeName = CombatGroup::NameFromType(Group->GetType());

        GroupTypeText->SetText(FText::FromString(
            TypeName ? UTF8_TO_TCHAR(TypeName) : TEXT("UNKNOWN")));
    }

    // ------------------------------------------------------------
    // LOCATION
    // ------------------------------------------------------------
    if (GroupLocationText)
    {
        GroupLocationText->SetText(FText::FromString(
            UTF8_TO_TCHAR(Group->GetRegion().data())));
    }

    // ------------------------------------------------------------
    // FACTION / EMPIRE (NOT AVAILABLE YET)
    // ------------------------------------------------------------
    if (GroupEmpireText)
    {
        /*
        // TODO: proper faction support once available in runtime model

        Combatant* Owner = Group->GetCombatant();
        if (Owner)
        {
            const char* FactionName = Owner->GetName();
            GroupEmpireText->SetText(FText::FromString(
                UTF8_TO_TCHAR(FactionName)));
            return;
        }
        */

        // Temporary fallback: use IFF
        GroupEmpireText->SetText(FText::FromString(
            FString::Printf(TEXT("IFF %d"), Group->GetIFF())));
    }
}

void UCmdForceDlg::PopulateDescForUnit(CombatUnit* Unit)
{
    if (!Unit)
    {
        ClearDescList();
        return;
    }

    // ------------------------------------------------------------
    // NAME / DESCRIPTION
    // ------------------------------------------------------------
    if (GroupInfoText)
    {
        GroupInfoText->SetText(FText::FromString(
            UTF8_TO_TCHAR(Unit->GetDescription())));
    }

    // ------------------------------------------------------------
   // TYPE (from ShipDesign)
   // ------------------------------------------------------------
    if (GroupTypeText)
    {
        const ShipDesign* Design = Unit->GetDesign();

        FString TypeText;
        
       //if (Design && Design->GetDesignClass()) // && strlen(Design->GetDesignClass()) > 0)
        //{
         //   TypeText = UTF8_TO_TCHAR(Design->GetDesignClass());
        //}
        //else
        //{
            TypeText = TEXT("UNKNOWN");
        //}

        GroupTypeText->SetText(FText::FromString(TypeText));
    }

    // ------------------------------------------------------------
    // LOCATION
    // ------------------------------------------------------------
    if (GroupLocationText)
    {
        GroupLocationText->SetText(FText::FromString(
            UTF8_TO_TCHAR(Unit->GetRegion().data())));
    }

    // ------------------------------------------------------------
    // FACTION / EMPIRE (NOT AVAILABLE YET)
    // ------------------------------------------------------------
    if (GroupEmpireText)
    {
        /*
        // TODO: proper faction system when available

        Combatant* Owner = Unit->GetCombatant();
        if (Owner)
        {
            GroupEmpireText->SetText(FText::FromString(
                UTF8_TO_TCHAR(Owner->GetName())));
            return;
        }
        */

        // Correct fallback: use unit's IFF
        GroupEmpireText->SetText(FText::FromString(
            FString::Printf(TEXT("IFF %d"), Unit->GetIFF())));
    }
}

void UCmdForceDlg::RebuildCombatListForCurrentCombatant()
{
}

void UCmdForceDlg::AddCombatGroupRecursive(CombatGroup* Group, bool bLastChild)
{
    if (!Group || Group->GetIntelLevel() < Intel::KNOWN || !CombatantList)
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
    // CombatantList->AddItem(NewObject<UCmdForceListItem>(...));
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

            // TODO: add unit row item to CombatantList (type=Unit)
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
    if (!TransferButton || !CampaignPtr || !CurrentGroup)
        return;

    const bool bEnable = CampaignPtr->IsActive() && CanTransfer(CurrentGroup);
    TransferButton->SetIsEnabled(bEnable);
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
