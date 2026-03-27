/*Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    UI
	FILE:         CmdForceDlg.cpp
	AUTHOR:       Carlos Bott
	ORIGINAL:     John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	UCmdForceDlg implementation.
*/

#include "CmdForceDlg.h"
#include "CmdForceListItem.h"

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
#include "Mouse.h"
#include "UIButton.h"
#include "CmdMsgDlg.h"
#include "GameStructs.h"

// Campaign screen:
#include "CmpnScreen.h"

DEFINE_LOG_CATEGORY_STATIC(LogCmdForceDlg, Log, All);

UCmdForceDlg::UCmdForceDlg(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UCmdForceDlg::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogCmdForceDlg, Log, TEXT("NativeConstruct: begin"));

	BindFormWidgets();

	Stars = Starshatter::GetInstance();
	CampaignPtr = Campaign::GetCampaign();

	UE_LOG(LogCmdForceDlg, Log, TEXT("NativeConstruct: Stars=%p Campaign=%p"), Stars, CampaignPtr);

	if (ForcesComboBox)
	{
		ForcesComboBox->OnSelectionChanged.RemoveDynamic(this, &UCmdForceDlg::OnForceSelectionChanged);
		ForcesComboBox->OnSelectionChanged.AddDynamic(this, &UCmdForceDlg::OnForceSelectionChanged);
	}

	if (TransferButton)
	{
		TransferButton->OnClicked.RemoveDynamic(this, &UCmdForceDlg::OnTransferClicked);
		TransferButton->OnClicked.AddDynamic(this, &UCmdForceDlg::OnTransferClicked);
		TransferButton->SetIsEnabled(false);
	}

	if (CombatantList)
	{
		CombatantList->OnItemSelectionChanged().RemoveAll(this);
		CombatantList->OnItemSelectionChanged().AddUObject(this, &UCmdForceDlg::OnCombatItemSelected);
	}

	UE_LOG(LogCmdForceDlg, Log, TEXT("NativeConstruct: end"));
}

void UCmdForceDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	ExecFrame();
}

void UCmdForceDlg::PopulateForcesComboBox()
{
	if (!ForcesComboBox)
	{
		UE_LOG(LogCmdForceDlg, Warning, TEXT("PopulateForcesComboBox: ForcesComboBox is null"));
		return;
	}

	if (!CampaignPtr)
	{
		CampaignPtr = Campaign::GetCampaign();
	}

	if (!CampaignPtr)
	{
		UE_LOG(LogCmdForceDlg, Warning, TEXT("PopulateForcesComboBox: CampaignPtr is null"));
		return;
	}

	ForcesComboBox->ClearOptions();

	const List<Combatant>& Combatants = CampaignPtr->GetCombatants();

	UE_LOG(LogCmdForceDlg, Log, TEXT("PopulateForcesComboBox: combatants=%d"), Combatants.size());

	TSet<FString> AddedNames;

	for (int i = 0; i < Combatants.size(); ++i)
	{
		Combatant* C = Combatants[i];
		if (!C)
		{
			UE_LOG(LogCmdForceDlg, Warning, TEXT("PopulateForcesComboBox: combatant[%d] is null"), i);
			continue;
		}

		if (!IsVisibleCombatant(C))
		{
			UE_LOG(LogCmdForceDlg, Log,
				TEXT("PopulateForcesComboBox: combatant[%d] '%s' skipped (not visible)"),
				i,
				UTF8_TO_TCHAR(C->GetName()));
			continue;
		}

		const FString CombatantName = UTF8_TO_TCHAR(C->GetName());

		if (CombatantName.IsEmpty())
		{
			UE_LOG(LogCmdForceDlg, Warning,
				TEXT("PopulateForcesComboBox: combatant[%d] has empty name"),
				i);
			continue;
		}

		if (!AddedNames.Contains(CombatantName))
		{
			AddedNames.Add(CombatantName);
			ForcesComboBox->AddOption(CombatantName);

			UE_LOG(LogCmdForceDlg, Log,
				TEXT("PopulateForcesComboBox: added '%s'"),
				*CombatantName);
		}
	}

	UE_LOG(LogCmdForceDlg, Log,
		TEXT("PopulateForcesComboBox: final option count=%d"),
		ForcesComboBox->GetOptionCount());

	if (ForcesComboBox->GetOptionCount() > 0)
	{
		ForcesComboBox->SetSelectedIndex(0);

		const FString SelectedName = ForcesComboBox->GetSelectedOption();
		UE_LOG(LogCmdForceDlg, Log,
			TEXT("PopulateForcesComboBox: auto-selected '%s'"),
			*SelectedName);

		Combatant* Found = nullptr;

		for (int i = 0; i < Combatants.size(); ++i)
		{
			Combatant* C = Combatants[i];
			if (C && SelectedName.Equals(UTF8_TO_TCHAR(C->GetName())))
			{
				Found = C;
				break;
			}
		}

		CurrentCombatant = Found;

		UE_LOG(LogCmdForceDlg, Log,
			TEXT("PopulateForcesComboBox: resolved CurrentCombatant=%p"),
			CurrentCombatant);
	}
	else
	{
		CurrentCombatant = nullptr;
		UE_LOG(LogCmdForceDlg, Warning, TEXT("PopulateForcesComboBox: no visible combatants found"));
	}
}
void UCmdForceDlg::BindFormWidgets()
{
	// Widget bindings are done directly through BindWidgetOptional
	// in the UPROPERTY declarations for this version of the dialog.
}

FString UCmdForceDlg::GetLegacyFormText() const
{
	return FString();
}

void UCmdForceDlg::SetParentCmdDlg(UCmdDlg* InParentCmdDlg)
{
	ParentCmdDlg = InParentCmdDlg;
}

void UCmdForceDlg::SetManager(UCmpnScreen* InManager)
{
	Manager = InManager;
}

void UCmdForceDlg::ShowForceDlg()
{
	Mode = ECOMMAND_MODE::MODE_FORCES;
	CampaignPtr = Campaign::GetCampaign();

	UE_LOG(LogCmdForceDlg, Log, TEXT("ShowForceDlg: Campaign=%p"), CampaignPtr);

	PopulateForcesComboBox();

	if (CurrentCombatant)
	{
		ShowCombatant(CurrentCombatant);
	}
	else
	{
		UE_LOG(LogCmdForceDlg, Warning, TEXT("ShowForceDlg: CurrentCombatant is null after PopulateForcesComboBox"));
	}

	SetVisibility(ESlateVisibility::Visible);
}

void UCmdForceDlg::ExecFrame()
{
	if (!CampaignPtr)
	{
		CampaignPtr = Campaign::GetCampaign();
	}

	if (!CampaignPtr)
	{
		return;
	}

	UpdateTransferEnabled();
}

void UCmdForceDlg::SetModeAndHighlight(ECOMMAND_MODE InMode)
{
	Mode = InMode;

	if (!Manager)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("CmdForceDlg: Manager is null (SetModeAndHighlight)."));
	}
}

void UCmdForceDlg::OnForceSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	UE_LOG(LogCmdForceDlg, Log,
		TEXT("OnForceSelectionChanged: item='%s' selectinfo=%d"),
		*SelectedItem,
		(int32)SelectionType);

	if (!CampaignPtr)
	{
		CampaignPtr = Campaign::GetCampaign();
	}

	if (!CampaignPtr)
	{
		UE_LOG(LogCmdForceDlg, Warning, TEXT("OnForceSelectionChanged: CampaignPtr is null"));
		return;
	}

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

	UE_LOG(LogCmdForceDlg, Log, TEXT("OnForceSelectionChanged: resolved combatant=%p"), CurrentCombatant);

	ShowCombatant(CurrentCombatant);
}

void UCmdForceDlg::OnCombatItemSelected(UObject* ItemObject)
{
	UCmdForceListItem* Item = Cast<UCmdForceListItem>(ItemObject);
	if (!Item)
	{
		UE_LOG(LogCmdForceDlg, Warning, TEXT("OnCombatItemSelected: invalid item"));
		return;
	}

	UE_LOG(LogCmdForceDlg, Log,
		TEXT("OnCombatItemSelected: rowType=%d text='%s'"),
		(int32)Item->RowType,
		*Item->DisplayText);

	CurrentGroup = nullptr;
	CurrentUnit = nullptr;

	if (Item->IsGroup())
	{
		CurrentGroup = Item->Group;
		UE_LOG(LogCmdForceDlg, Log, TEXT("OnCombatItemSelected: selected group=%p"), CurrentGroup);
		PopulateDescForGroup(CurrentGroup);
	}
	else if (Item->IsUnit())
	{
		CurrentUnit = Item->Unit;
		UE_LOG(LogCmdForceDlg, Log, TEXT("OnCombatItemSelected: selected unit=%p"), CurrentUnit);
		PopulateDescForUnit(CurrentUnit);
	}
	else
	{
		ClearDescList();
	}

	UpdateTransferEnabled();
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
	UE_LOG(LogCmdForceDlg, Log, TEXT("ShowCombatant: combatant=%p"), C);

	if (!CombatantList || !C)
	{
		UE_LOG(LogCmdForceDlg, Warning,
			TEXT("ShowCombatant: CombatantList=%p combatant=%p"),
			CombatantList, C);
		return;
	}

	CurrentGroup = nullptr;
	CurrentUnit = nullptr;
	CurrentCombatant = C;

	PipeStack.Empty();
	bBlankLine = false;

	CombatantList->ClearListItems();

	CombatGroup* Force = C->GetForce();
	UE_LOG(LogCmdForceDlg, Log, TEXT("ShowCombatant: force=%p"), Force);

	if (Force)
	{
		List<CombatGroup>& Groups = Force->GetComponents();

		UE_LOG(LogCmdForceDlg, Log, TEXT("ShowCombatant: top groups=%d"), Groups.size());

		for (int i = 0; i < Groups.size(); ++i)
		{
			CombatGroup* G = Groups[i];

			if (G)
			{
				UE_LOG(LogCmdForceDlg, Log,
					TEXT("ShowCombatant: group[%d] id=%d name='%s' type=%d units=%d intel=%d"),
					i,
					G->GetID(),
					UTF8_TO_TCHAR(G->GetDescription()),
					(int32)G->GetType(),
					G->CountUnits(),
					G->GetIntelLevel());
			}

			if (G &&
				G->GetType() < ECOMBATGROUP_TYPE::CIVILIAN &&
				G->CountUnits() > 0)
			{
				AddCombatGroupRecursive(G, i == Groups.size() - 1);
			}
		}
	}

	ClearDescList();

	if (TransferButton)
	{
		TransferButton->SetIsEnabled(false);
	}

	UE_LOG(LogCmdForceDlg, Log, TEXT("ShowCombatant: list items now=%d"),
		CombatantList ? CombatantList->GetNumItems() : -1);
}

void UCmdForceDlg::RebuildCombatListForCurrentCombatant()
{
	if (!CombatantList)
	{
		return;
	}

	CombatantList->ClearListItems();
	PipeStack.Empty();
	bBlankLine = false;

	if (!CurrentCombatant)
	{
		return;
	}

	CombatGroup* Force = CurrentCombatant->GetForce();
	if (!Force)
	{
		return;
	}

	List<CombatGroup>& Groups = Force->GetComponents();

	for (int i = 0; i < Groups.size(); ++i)
	{
		CombatGroup* G = Groups[i];
		if (G &&
			G->GetType() < ECOMBATGROUP_TYPE::CIVILIAN &&
			G->CountUnits() > 0)
		{
			AddCombatGroupRecursive(G, i == Groups.size() - 1);
		}
	}
}

void UCmdForceDlg::ClearDescList()
{
	if (DescList)
	{
		DescList->ClearListItems();
	}

	if (GroupInfoText)
	{
		GroupInfoText->SetText(FText::GetEmpty());
	}

	if (GroupTypeText)
	{
		GroupTypeText->SetText(FText::GetEmpty());
	}

	if (GroupLocationText)
	{
		GroupLocationText->SetText(FText::GetEmpty());
	}

	if (GroupEmpireText)
	{
		GroupEmpireText->SetText(FText::GetEmpty());
	}
}

void UCmdForceDlg::AddCombatGroupRecursive(CombatGroup* Group, bool bLastChild)
{
	if (!Group || Group->GetIntelLevel() < Intel::KNOWN || !CombatantList)
	{
		return;
	}

	UE_LOG(LogCmdForceDlg, Log,
		TEXT("AddCombatGroupRecursive: id=%d desc='%s' type=%d units=%d liveChildren=%d expanded=%d pipe='%s'"),
		Group->GetID(),
		UTF8_TO_TCHAR(Group->GetDescription()),
		(int32)Group->GetType(),
		Group->GetUnits().size(),
		Group->GetLiveComponents().size(),
		Group->IsExpanded(),
		*PipeStack);

	FString Prefix;

	const bool bTopLevel =
		(!Group->GetParent() ||
			Group->GetParent()->GetType() == ECOMBATGROUP_TYPE::FORCE);

	if (bTopLevel)
	{
		Prefix = Group->IsExpanded() ? TEXT("[-] ") : TEXT("[+] ");
	}
	else
	{
		Prefix = bLastChild ? TEXT("\\-") : TEXT("+-");

		const bool bHasChildrenOrUnits =
			Group->GetLiveComponents().size() > 0 ||
			Group->GetUnits().size() > 0;

		if (bHasChildrenOrUnits)
		{
			Prefix += Group->IsExpanded() ? TEXT("[-] ") : TEXT("[+] ");
		}
		else
		{
			Prefix += TEXT("   ");
		}
	}

	const FString Line = PipeStack + Prefix + UTF8_TO_TCHAR(Group->GetDescription());

	const bool bHasChildrenOrUnits =
		Group->GetLiveComponents().size() > 0 ||
		Group->GetUnits().size() > 0;

	UCmdForceListItem* GroupItem = NewObject<UCmdForceListItem>(this);
	GroupItem->InitAsGroup(
		Line,
		PipeStack.Len(),
		Group,
		Group->IsExpanded(),
		bHasChildrenOrUnits);

	CombatantList->AddItem(GroupItem);

	UE_LOG(LogCmdForceDlg, Log, TEXT("AddCombatGroupRecursive: added group row '%s'"), *Line);

	bBlankLine = false;

	const int32 PrevLen = PipeStack.Len();

	if (!bTopLevel)
	{
		PipeStack += (bLastChild ? TEXT("  ") : TEXT("| "));
	}

	if (Group->IsExpanded() && Group->GetUnits().size() > 0)
	{
		ListIter<CombatUnit> UnitIter = Group->GetUnits();
		while (++UnitIter)
		{
			CombatUnit* Unit = UnitIter.value();
			if (!Unit)
			{
				continue;
			}

			FString UnitLine = PipeStack + TEXT("  ") + UTF8_TO_TCHAR(Unit->GetDescription());

			UCmdForceListItem* UnitItem = NewObject<UCmdForceListItem>(this);
			UnitItem->InitAsUnit(UnitLine, PipeStack.Len(), Unit);
			CombatantList->AddItem(UnitItem);

			UE_LOG(LogCmdForceDlg, Log,
				TEXT("AddCombatGroupRecursive: added unit row '%s' iff=%d"),
				*UnitLine, Unit->GetIFF());
		}

		UCmdForceListItem* SpacerItem = NewObject<UCmdForceListItem>(this);
		SpacerItem->InitAsSpacer();
		CombatantList->AddItem(SpacerItem);

		bBlankLine = true;
	}

	if (Group->IsExpanded() && Group->GetLiveComponents().size() > 0)
	{
		List<CombatGroup>& Groups = Group->GetLiveComponents();
		for (int i = 0; i < Groups.size(); ++i)
		{
			AddCombatGroupRecursive(Groups[i], i == Groups.size() - 1);
		}

		if (!bBlankLine)
		{
			UCmdForceListItem* SpacerItem = NewObject<UCmdForceListItem>(this);
			SpacerItem->InitAsSpacer();
			CombatantList->AddItem(SpacerItem);

			bBlankLine = true;
		}
	}

	PipeStack.LeftInline(PrevLen);
}

void UCmdForceDlg::PopulateDescForGroup(CombatGroup* Group)
{
	if (!Group)
	{
		ClearDescList();
		return;
	}

	if (GroupInfoText)
	{
		GroupInfoText->SetText(FText::FromString(
			UTF8_TO_TCHAR(Group->GetDescription())));
	}

	if (GroupTypeText)
	{
		const char* TypeName = CombatGroup::NameFromType(Group->GetType());

		GroupTypeText->SetText(FText::FromString(
			TypeName ? UTF8_TO_TCHAR(TypeName) : TEXT("UNKNOWN")));
	}

	if (GroupLocationText)
	{
		GroupLocationText->SetText(FText::FromString(
			UTF8_TO_TCHAR(Group->GetRegion().data())));
	}

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

	if (GroupInfoText)
	{
		GroupInfoText->SetText(FText::FromString(
			UTF8_TO_TCHAR(Unit->GetDescription())));
	}

	if (GroupTypeText)
	{
		FString TypeText = TEXT("UNKNOWN");

		switch ((CLASSIFICATION)Unit->Type())
		{
		case CLASSIFICATION::FIGHTER:
			TypeText = TEXT("FIGHTER");
			break;

		case CLASSIFICATION::ATTACK:
			TypeText = TEXT("ATTACK");
			break;

		case CLASSIFICATION::LCA:
			TypeText = TEXT("LANDING CRAFT");
			break;

		case CLASSIFICATION::DESTROYER:
			TypeText = TEXT("DESTROYER");
			break;

		case CLASSIFICATION::CRUISER:
			TypeText = TEXT("CRUISER");
			break;

		case CLASSIFICATION::CARRIER:
			TypeText = TEXT("CARRIER");
			break;

		case CLASSIFICATION::STATION:
			TypeText = TEXT("STATION");
			break;

		case CLASSIFICATION::STARBASE:
			TypeText = TEXT("STARBASE");
			break;

		case CLASSIFICATION::MINE:
			TypeText = TEXT("MINE");
			break;

		default:
			break;
		}

		GroupTypeText->SetText(FText::FromString(TypeText));
	}

	if (GroupLocationText)
	{
		GroupLocationText->SetText(FText::FromString(
			UTF8_TO_TCHAR(Unit->GetRegion().data())));
	}

	if (GroupEmpireText)
	{
		/*
		// TODO: proper faction support once runtime ownership exposes a name

		CombatGroup* OwnerGroup = Unit->GetCombatGroup();
		if (OwnerGroup)
		{
			Combatant* Owner = OwnerGroup->GetCombatant();
			if (Owner)
			{
				GroupEmpireText->SetText(FText::FromString(
					UTF8_TO_TCHAR(Owner->GetName())));
				return;
			}
		}
		*/

		GroupEmpireText->SetText(FText::FromString(
			FString::Printf(TEXT("IFF %d"), Unit->GetIFF())));
	}
}

bool UCmdForceDlg::CanTransfer(CombatGroup* Group) const
{
	if (!Group || !CampaignPtr)
	{
		return false;
	}

	if (Group->GetType() < ECOMBATGROUP_TYPE::WING)
	{
		return false;
	}

	if (Group->GetType() > ECOMBATGROUP_TYPE::CARRIER_GROUP)
	{
		return false;
	}

	if (Group->GetType() == ECOMBATGROUP_TYPE::FLEET ||
		Group->GetType() == ECOMBATGROUP_TYPE::LCA_SQUADRON)
	{
		return false;
	}

	CombatGroup* PlayerGroup = CampaignPtr->GetPlayerGroup();
	if (!PlayerGroup || PlayerGroup->GetIFF() != Group->GetIFF())
	{
		return false;
	}

	return true;
}

void UCmdForceDlg::UpdateTransferEnabled()
{
	if (!TransferButton || !CampaignPtr || !CurrentGroup)
	{
		return;
	}

	const bool bEnable = CampaignPtr->IsActive() && CanTransfer(CurrentGroup);
	TransferButton->SetIsEnabled(bEnable);
}

void UCmdForceDlg::OnTransferClicked()
{
	if (!CampaignPtr || !CurrentGroup || !Manager)
	{
		return;
	}

	PlayerCharacter* PlayerPtr = PlayerCharacter::GetCurrentPlayer();
	if (!PlayerPtr)
	{
		return;
	}

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
	{
		return;
	}

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

		const char* Required =
			PlayerCharacter::RankName(PlayerCharacter::CommandRankRequired(CmdClass));

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

CombatGroup* UCmdForceDlg::GetTopForceGroup(CombatGroup* Group) const
{
	CombatGroup* Current = Group;

	while (Current && Current->GetParent())
	{
		Current = Current->GetParent();
	}

	return Current;
}