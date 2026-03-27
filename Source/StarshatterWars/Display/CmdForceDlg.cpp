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
#include "FormattingUtils.h"

// Campaign screen:
#include "CmpnScreen.h"

DEFINE_LOG_CATEGORY_STATIC(LogCmdForceDlg, Log, All);

static FString CombatGroupTypeToDisplayString(ECOMBATGROUP_TYPE Type)
{
	const char* TypeName = CombatGroup::NameFromType(Type);
	FString Out = TypeName ? UTF8_TO_TCHAR(TypeName) : TEXT("UNKNOWN");
	Out = Out.Replace(TEXT("_"), TEXT(" "));
	Out = Out.ToUpper();
	return Out;
}

static FString BuildSafeUnitDisplayText(CombatUnit* Unit)
{
	if (!Unit)
	{
		return TEXT("UNKNOWN UNIT");
	}

	const FString Registry = UTF8_TO_TCHAR(Unit->GetRegistryNumber().data());
	const FString Indicator = UFormattingUtils::GetUnitDesignIndicator(Unit);

	FString Name = UTF8_TO_TCHAR(Unit->GetName().data());

	if (Name.IsEmpty() && Unit->HasResolvedDesignData())
	{
		Name = UTF8_TO_TCHAR(Unit->ResolvedDisplayName().data());
	}

	FString Prefix;

	if (!Indicator.IsEmpty() && !Registry.IsEmpty())
	{
		Prefix = FString::Printf(TEXT("%s-%s"), *Indicator, *Registry);
	}
	else if (!Registry.IsEmpty())
	{
		Prefix = Registry;
	}
	else if (!Indicator.IsEmpty())
	{
		Prefix = Indicator;
	}

	if (!Name.IsEmpty())
	{
		if (!Prefix.IsEmpty())
		{
			return FString::Printf(TEXT("%s %s"), *Prefix, *Name);
		}

		return Name;
	}

	return Prefix.IsEmpty() ? TEXT("UNKNOWN UNIT") : Prefix;
}

UCmdForceDlg::UCmdForceDlg(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UCmdForceDlg::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogCmdForceDlg, Log, TEXT("[CmdForceDlg] NativeConstruct: begin"));

	Stars = Starshatter::GetInstance();
	CampaignPtr = Campaign::GetCampaign();

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
		UE_LOG(LogCmdForceDlg, Warning, TEXT("[CmdForceDlg] PopulateForcesComboBox: ForcesComboBox is null"));
		return;
	}

	if (!CampaignPtr)
	{
		CampaignPtr = Campaign::GetCampaign();
	}

	if (!CampaignPtr)
	{
		UE_LOG(LogCmdForceDlg, Warning, TEXT("[CmdForceDlg] PopulateForcesComboBox: CampaignPtr is null"));
		return;
	}

	ForcesComboBox->ClearOptions();

	const List<Combatant>& Combatants = CampaignPtr->GetCombatants();

	UE_LOG(LogCmdForceDlg, Log, TEXT("[CmdForceDlg] PopulateForcesComboBox: combatants=%d"), Combatants.size());

	TSet<FString> AddedNames;

	for (int i = 0; i < Combatants.size(); ++i)
	{
		Combatant* C = Combatants[i];
		if (!C)
		{
			UE_LOG(LogCmdForceDlg, Warning, TEXT("[CmdForceDlg] PopulateForcesComboBox: combatant[%d] is null"), i);
			continue;
		}

		if (!IsVisibleCombatant(C))
		{
			UE_LOG(LogCmdForceDlg, Log,
				TEXT("[CmdForceDlg] PopulateForcesComboBox: combatant[%d] '%s' skipped (not visible)"),
				i,
				UTF8_TO_TCHAR(C->GetName()));
			continue;
		}

		const FString CombatantName = UTF8_TO_TCHAR(C->GetName());

		if (CombatantName.IsEmpty())
		{
			UE_LOG(LogCmdForceDlg, Warning,
				TEXT("[CmdForceDlg] PopulateForcesComboBox: combatant[%d] has empty name"),
				i);
			continue;
		}

		if (!AddedNames.Contains(CombatantName))
		{
			AddedNames.Add(CombatantName);
			ForcesComboBox->AddOption(CombatantName);

			UE_LOG(LogCmdForceDlg, Log,
				TEXT("[CmdForceDlg] PopulateForcesComboBox: added '%s'"),
				*CombatantName);
		}
	}

	UE_LOG(LogCmdForceDlg, Log,
		TEXT("[CmdForceDlg] PopulateForcesComboBox: final option count=%d"),
		ForcesComboBox->GetOptionCount());

	if (ForcesComboBox->GetOptionCount() > 0)
	{
		ForcesComboBox->SetSelectedIndex(0);

		const FString SelectedName = ForcesComboBox->GetSelectedOption();
		UE_LOG(LogCmdForceDlg, Log,
			TEXT("[CmdForceDlg] PopulateForcesComboBox: auto-selected '%s'"),
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
			TEXT("[CmdForceDlg] PopulateForcesComboBox: resolved CurrentCombatant=%p"),
			CurrentCombatant);
	}
	else
	{
		CurrentCombatant = nullptr;
		UE_LOG(LogCmdForceDlg, Warning, TEXT("[CmdForceDlg] PopulateForcesComboBox: no visible combatants found"));
	}
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

	UE_LOG(LogCmdForceDlg, Log, TEXT("[CmdForceDlg] ShowForceDlg: Campaign=%p"), CampaignPtr);

	PopulateForcesComboBox();

	if (CurrentCombatant)
	{
		ShowCombatant(CurrentCombatant);
	}
	else
	{
		UE_LOG(LogCmdForceDlg, Warning, TEXT("[CmdForceDlg] ShowForceDlg: CurrentCombatant is null after PopulateForcesComboBox"));
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
			TEXT("[CmdForceDlg] CmdForceDlg: Manager is null (SetModeAndHighlight)."));
	}
}

void UCmdForceDlg::OnForceSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	UE_LOG(LogCmdForceDlg, Log,
		TEXT("[CmdForceDlg] OnForceSelectionChanged: item='%s' selectinfo=%d"),
		*SelectedItem,
		(int32)SelectionType);

	if (!CampaignPtr)
	{
		CampaignPtr = Campaign::GetCampaign();
	}

	if (!CampaignPtr)
	{
		UE_LOG(LogCmdForceDlg, Warning, TEXT("[CmdForceDlg] OnForceSelectionChanged: CampaignPtr is null"));
		return;
	}

	Combatant* Found = nullptr;

	ListIter<Combatant> Iter = CampaignPtr->GetCombatants();
	while (++Iter)
	{
		Combatant* C = Iter.value();
		if (!C)
		{
			continue;
		}

		UE_LOG(LogCmdForceDlg, Verbose,
			TEXT("[CmdForceDlg] OnForceSelectionChanged: checking combatant name='%s'"),
			UTF8_TO_TCHAR(C->GetName()));

		if (SelectedItem.Equals(UTF8_TO_TCHAR(C->GetName())))
		{
			Found = C;
			break;
		}
	}

	CurrentCombatant = Found;

	if (CurrentCombatant)
	{
		CombatGroup* Force = CurrentCombatant->GetForce();

		UE_LOG(LogCmdForceDlg, Log,
			TEXT("[CmdForceDlg] OnForceSelectionChanged: resolved combatant name='%s' forceId=%d forceName='%s' forceEmpire=%d"),
			UTF8_TO_TCHAR(CurrentCombatant->GetName()),
			Force ? Force->GetID() : -1,
			Force ? UTF8_TO_TCHAR(Force->GetDescription()) : TEXT("NULL"),
			Force ? (int32)Force->GetEmpire() : -1);
	}
	else
	{
		UE_LOG(LogCmdForceDlg, Warning,
			TEXT("[CmdForceDlg] OnForceSelectionChanged: no combatant found for '%s'"),
			*SelectedItem);
	}

	ShowCombatant(CurrentCombatant);
}

void UCmdForceDlg::OnCombatItemSelected(UObject* ItemObject)
{
	UCmdForceListItem* Item = Cast<UCmdForceListItem>(ItemObject);
	if (!Item)
	{
		return;
	}

	CurrentGroup = nullptr;
	CurrentUnit = nullptr;

	if (Item->IsGroup())
	{
		CurrentGroup = Item->Group;

		if (CurrentGroup && Item->bHasChildren)
		{
			CurrentGroup->SetExpanded(!CurrentGroup->IsExpanded());
			RebuildCombatListForCurrentCombatant();
		}

		PopulateDescForGroup(CurrentGroup);
	}
	else if (Item->IsUnit())
	{
		CurrentUnit = Item->Unit;
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
	if (!CombatantList)
	{
		UE_LOG(LogCmdForceDlg, Warning,
			TEXT("[CmdForceDlg] ShowCombatant: CombatantList is NULL"));
		return;
	}

	if (!C)
	{
		UE_LOG(LogCmdForceDlg, Warning,
			TEXT("[CmdForceDlg] ShowCombatant: Combatant is NULL"));
		return;
	}

	UE_LOG(LogCmdForceDlg, Log,
		TEXT("========================================"));
	UE_LOG(LogCmdForceDlg, Log,
		TEXT("[CmdForceDlg] ShowCombatant: combatant='%s'"),
		UTF8_TO_TCHAR(C->GetName()));

	CurrentGroup = nullptr;
	CurrentUnit = nullptr;
	CurrentCombatant = C;

	bBlankLine = false;

	CombatantList->ClearListItems();

	CombatGroup* Force = C->GetForce();

	if (!Force)
	{
		UE_LOG(LogCmdForceDlg, Warning,
			TEXT("[CmdForceDlg] ShowCombatant: Force is NULL for combatant='%s'"),
			UTF8_TO_TCHAR(C->GetName()));
		return;
	}

	UE_LOG(LogCmdForceDlg, Log,
		TEXT("[CmdForceDlg] Force ROOT: id=%d name='%s' type=%d iff=%d empire=%d region='%s' children=%d units=%d"),
		Force->GetID(),
		UTF8_TO_TCHAR(Force->GetDescription()),
		(int32)Force->GetType(),
		Force->GetIFF(),
		(int32)Force->GetEmpire(),
		UTF8_TO_TCHAR(Force->GetRegion().data()),
		Force->GetComponents().size(),
		Force->GetUnits().size());

	List<CombatGroup>& Groups = Force->GetComponents();

	UE_LOG(LogCmdForceDlg, Log,
		TEXT("[CmdForceDlg] Top-level groups: %d"),
		Groups.size());

	for (int i = 0; i < Groups.size(); ++i)
	{
		CombatGroup* G = Groups[i];

		if (!G)
		{
			UE_LOG(LogCmdForceDlg, Warning,
				TEXT("[CmdForceDlg] Top-level group[%d] is NULL"),
				i);
			continue;
		}

		UE_LOG(LogCmdForceDlg, Log,
			TEXT("[CmdForceDlg] TOP GROUP [%d]: id=%d name='%s' type=%d iff=%d empire=%d region='%s' units=%d children=%d intel=%d expanded=%d"),
			i,
			G->GetID(),
			UTF8_TO_TCHAR(G->GetDescription()),
			(int32)G->GetType(),
			G->GetIFF(),
			(int32)G->GetEmpire(),
			UTF8_TO_TCHAR(G->GetRegion().data()),
			G->CountUnits(),
			G->GetLiveComponents().size(),
			G->GetIntelLevel(),
			G->IsExpanded());

		DumpCombatGroupRecursive(G, 0);

		if (G->GetType() < ECOMBATGROUP_TYPE::CIVILIAN &&
			G->CountUnits() > 0)
		{
			AddCombatGroupRecursive(G, i == Groups.size() - 1, 0);
		}
		else
		{
			UE_LOG(LogCmdForceDlg, Log,
				TEXT("[CmdForceDlg] Skipping group id=%d name='%s' type=%d countUnits=%d"),
				G->GetID(),
				UTF8_TO_TCHAR(G->GetDescription()),
				(int32)G->GetType(),
				G->CountUnits());
		}
	}

	ClearDescList();

	if (TransferButton)
	{
		TransferButton->SetIsEnabled(false);
	}

	UE_LOG(LogCmdForceDlg, Log,
		TEXT("[CmdForceDlg] ShowCombatant complete: generated list items=%d"),
		CombatantList->GetNumItems());
}

void UCmdForceDlg::RebuildCombatListForCurrentCombatant()
{
	if (!CombatantList)
	{
		return;
	}

	CombatantList->ClearListItems();
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
			AddCombatGroupRecursive(G, i == Groups.size() - 1, 0);
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

void UCmdForceDlg::AddCombatGroupRecursive(CombatGroup* Group, bool bLastChild, int32 Depth)
{
	if (!Group || Group->GetIntelLevel() < Intel::KNOWN || !CombatantList)
	{
		return;
	}

	const bool bHasChildrenOrUnits =
		Group->GetLiveComponents().size() > 0 ||
		Group->GetUnits().size() > 0;

	UE_LOG(LogCmdForceDlg, Log,
		TEXT("AddCombatGroupRecursive: id=%d desc='%s' type=%d units=%d liveChildren=%d expanded=%d depth=%d"),
		Group->GetID(),
		UTF8_TO_TCHAR(Group->GetDescription()),
		(int32)Group->GetType(),
		Group->GetUnits().size(),
		Group->GetLiveComponents().size(),
		Group->IsExpanded(),
		Depth);

	const FString Line = UTF8_TO_TCHAR(Group->GetDescription());
	UCmdForceListItem* GroupItem = NewObject<UCmdForceListItem>(this);
	GroupItem->InitAsGroup(
		Line,
		Depth,
		Group,
		Group->IsExpanded(),
		bHasChildrenOrUnits);

	CombatantList->AddItem(GroupItem);

	UE_LOG(LogCmdForceDlg, Log,
		TEXT("AddCombatGroupRecursive: added group row '%s' depth=%d"),
		*Line,
		Depth);

	bBlankLine = false;

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

			const FString UnitLine = BuildSafeUnitDisplayText(Unit);

			UCmdForceListItem* UnitItem = NewObject<UCmdForceListItem>(this);
			UnitItem->InitAsUnit(UnitLine, Depth + 1, Unit);
			CombatantList->AddItem(UnitItem);
		}

		UCmdForceListItem* SpacerItem = NewObject<UCmdForceListItem>(this);
		SpacerItem->InitAsSpacer();
		CombatantList->AddItem(SpacerItem);

		bBlankLine = true;
	}

	// Children
	if (Group->IsExpanded() && Group->GetLiveComponents().size() > 0)
	{
		List<CombatGroup>& Groups = Group->GetLiveComponents();

		for (int i = 0; i < Groups.size(); ++i)
		{
			AddCombatGroupRecursive(Groups[i], i == Groups.size() - 1, Depth + 1);
		}

		if (!bBlankLine)
		{
			UCmdForceListItem* SpacerItem = NewObject<UCmdForceListItem>(this);
			SpacerItem->InitAsSpacer();
			CombatantList->AddItem(SpacerItem);

			bBlankLine = true;
		}
	}
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
		GroupTypeText->SetText(FText::FromString(
			CombatGroupTypeToDisplayString(Group->GetType())));
	}

	if (GroupLocationText)
	{
		GroupLocationText->SetText(FText::FromString(
			UTF8_TO_TCHAR(Group->GetRegion().data())));
	}

	if (GroupEmpireText)
	{
		// Uses DT-loaded empire from CombatGroup
		GroupEmpireText->SetText(FText::FromString(
			UFormattingUtils::EmpireToString(Group->GetEmpire())));
	}
}

void UCmdForceDlg::PopulateDescForUnit(CombatUnit* Unit)
{
	if (!Unit)
	{
		ClearDescList();
		return;
	}

	GroupInfoText->SetText(FText::FromString(
		BuildSafeUnitDisplayText(Unit)));

	if (GroupLocationText)
	{
		GroupLocationText->SetText(FText::FromString(
			UTF8_TO_TCHAR(Unit->GetRegion().data())));
	}

	if (GroupEmpireText)
	{
		CombatGroup* OwnerGroup = Unit->GetCombatGroup();

		const FString EmpireText = OwnerGroup
			? UFormattingUtils::EmpireToString(OwnerGroup->GetEmpire())
			: TEXT("UNKNOWN");

		GroupEmpireText->SetText(FText::FromString(EmpireText));
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
void UCmdForceDlg::DumpCombatGroupRecursive(CombatGroup* Group, int32 Depth)
{
	if (!Group)
	{
		return;
	}

	FString Indent;
	for (int i = 0; i < Depth; ++i)
	{
		Indent += TEXT("  ");
	}

	UE_LOG(LogCmdForceDlg, Log,
		TEXT("%sGROUP: id=%d name='%s' type=%d units=%d children=%d empire=%d"),
		*Indent,
		Group->GetID(),
		UTF8_TO_TCHAR(Group->GetDescription()),
		(int32)Group->GetType(),
		Group->GetUnits().size(),
		Group->GetLiveComponents().size(),
		(int32)Group->GetEmpire());

	// Units
	ListIter<CombatUnit> UnitIter = Group->GetUnits();
	while (++UnitIter)
	{
		CombatUnit* Unit = UnitIter.value();
		if (!Unit)
		{
			continue;
		}

		UE_LOG(LogCmdForceDlg, Log,
			TEXT("%s  UNIT: name='%s' type=%d iff=%d"),
			*Indent,
			UTF8_TO_TCHAR(Unit->GetDescription()),
			Unit->Type(),
			Unit->GetIFF());
	}

	// Children
	List<CombatGroup>& Children = Group->GetLiveComponents();

	for (int i = 0; i < Children.size(); ++i)
	{
		DumpCombatGroupRecursive(Children[i], Depth + 1);
	}
}