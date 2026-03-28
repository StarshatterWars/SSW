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

static bool IsSquadronUnitType(int UnitType)
{
	switch ((CLASSIFICATION)UnitType)
	{
	case CLASSIFICATION::FIGHTER:
	case CLASSIFICATION::ATTACK:
	case CLASSIFICATION::LCA:
		return true;

	default:
		return false;
	}
}

static void GatherGroupTypeCountsRecursive(
	CombatGroup* Group,
	TMap<FString, int32>& OutUnitTypeCounts,
	TMap<FString, int32>& OutSquadronTypeCounts)
{
	if (!Group)
	{
		return;
	}

	ListIter<CombatUnit> UnitIter = Group->GetUnits();
	while (++UnitIter)
	{
		CombatUnit* Unit = UnitIter.value();
		if (!Unit)
		{
			continue;
		}

		FString TypeName;

		if (Unit->HasResolvedDesignData())
		{
			const FString Indicator = UFormattingUtils::GetUnitDesignIndicator(Unit);
			const FString DisplayName = UTF8_TO_TCHAR(Unit->ResolvedDisplayName().data());

			if (!Indicator.IsEmpty() && !DisplayName.IsEmpty())
			{
				TypeName = FString::Printf(TEXT("%s %s"), *Indicator, *DisplayName);
			}
			else if (!DisplayName.IsEmpty())
			{
				TypeName = DisplayName;
			}
			else if (!Indicator.IsEmpty())
			{
				TypeName = Indicator;
			}
		}

		if (TypeName.IsEmpty())
		{
			const FString Indicator = UFormattingUtils::GetUnitDesignIndicator(Unit);
			const FString DesignName = UTF8_TO_TCHAR(Unit->GetDesignName().data());

			if (!Indicator.IsEmpty() && !DesignName.IsEmpty())
			{
				TypeName = FString::Printf(TEXT("%s %s"), *Indicator, *DesignName);
			}
			else if (!DesignName.IsEmpty())
			{
				TypeName = DesignName;
			}
			else if (!Indicator.IsEmpty())
			{
				TypeName = Indicator;
			}
			else
			{
				TypeName = TEXT("UNKNOWN");
			}
		}

		if (IsSquadronUnitType(Unit->Type()))
		{
			OutSquadronTypeCounts.FindOrAdd(TypeName) += Unit->Count();
		}
		else
		{
			OutUnitTypeCounts.FindOrAdd(TypeName) += Unit->Count();
		}
	}

	ListIter<CombatGroup> GroupIter = Group->GetComponents();
	while (++GroupIter)
	{
		CombatGroup* Child = GroupIter.value();
		if (!Child)
		{
			continue;
		}

		GatherGroupTypeCountsRecursive(Child, OutUnitTypeCounts, OutSquadronTypeCounts);
	}
}

static void GetGroupTotalsRecursive(CombatGroup* Group, int32& OutTotalCount, int32& OutLiveCount)
{
	OutTotalCount = 0;
	OutLiveCount = 0;

	if (!Group)
	{
		return;
	}

	// Direct units
	ListIter<CombatUnit> UnitIter = Group->GetUnits();
	while (++UnitIter)
	{
		CombatUnit* Unit = UnitIter.value();
		if (!Unit)
		{
			continue;
		}

		OutTotalCount += Unit->Count();
		OutLiveCount += Unit->LiveCount();
	}

	// Child groups
	ListIter<CombatGroup> GroupIter = Group->GetComponents();
	while (++GroupIter)
	{
		CombatGroup* Child = GroupIter.value();
		if (!Child)
		{
			continue;
		}

		int32 ChildTotal = 0;
		int32 ChildLive = 0;
		GetGroupTotalsRecursive(Child, ChildTotal, ChildLive);

		OutTotalCount += ChildTotal;
		OutLiveCount += ChildLive;
	}
}

static void GatherGroupTypeCountsRecursive(CombatGroup* Group, TMap<FString, int32>& OutTypeCounts)
{
	if (!Group)
	{
		return;
	}

	// Direct units
	ListIter<CombatUnit> UnitIter = Group->GetUnits();
	while (++UnitIter)
	{
		CombatUnit* Unit = UnitIter.value();
		if (!Unit)
		{
			continue;
		}

		FString TypeName;

		if (Unit->HasResolvedDesignData())
		{
			const FString Indicator = UFormattingUtils::GetUnitDesignIndicator(Unit);
			const FString DisplayName = UTF8_TO_TCHAR(Unit->ResolvedDisplayName().data());

			if (!Indicator.IsEmpty() && !DisplayName.IsEmpty())
			{
				TypeName = FString::Printf(TEXT("%s %s"), *Indicator, *DisplayName);
			}
			else if (!DisplayName.IsEmpty())
			{
				TypeName = DisplayName;
			}
			else if (!Indicator.IsEmpty())
			{
				TypeName = Indicator;
			}
		}

		if (TypeName.IsEmpty())
		{
			const FString Indicator = UFormattingUtils::GetUnitDesignIndicator(Unit);
			const FString DesignName = UTF8_TO_TCHAR(Unit->GetDesignName().data());

			if (!Indicator.IsEmpty() && !DesignName.IsEmpty())
			{
				TypeName = FString::Printf(TEXT("%s %s"), *Indicator, *DesignName);
			}
			else if (!DesignName.IsEmpty())
			{
				TypeName = DesignName;
			}
			else if (!Indicator.IsEmpty())
			{
				TypeName = Indicator;
			}
			else
			{
				TypeName = TEXT("UNKNOWN");
			}
		}

		OutTypeCounts.FindOrAdd(TypeName) += Unit->Count();
	}

	// Child groups
	ListIter<CombatGroup> GroupIter = Group->GetComponents();
	while (++GroupIter)
	{
		CombatGroup* Child = GroupIter.value();
		if (!Child)
		{
			continue;
		}

		GatherGroupTypeCountsRecursive(Child, OutTypeCounts);
	}
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

		if(TransferButtonText)
		 {
			 TransferButtonText->SetText(FText::FromString(TEXT("TRANSFER")));
		}
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

	if (GroupNameText)
	{
		GroupNameText->SetText(FText::GetEmpty());
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

	if (GroupNameText)
	{
		GroupNameText->SetText(FText::FromString(
			UTF8_TO_TCHAR(Group->GetDescription())));
	}

	if (GroupLocationText)
	{
		GroupLocationText->SetText(FText::FromString(
			UTF8_TO_TCHAR(Group->GetRegion().data())));
	}

	if (GroupEmpireText)
	{
		GroupEmpireText->SetText(FText::FromString(
			UFormattingUtils::EmpireToString(Group->GetEmpire())));
	}

	if (!GroupInfoText)
	{
		return;
	}

	int32 TotalCount = 0;
	int32 LiveCount = 0;
	GetGroupTotalsRecursive(Group, TotalCount, LiveCount);

	TMap<FString, int32> UnitTypeCounts;
	TMap<FString, int32> SquadronTypeCounts;
	GatherGroupTypeCountsRecursive(Group, UnitTypeCounts, SquadronTypeCounts);

	const FString GroupName = UTF8_TO_TCHAR(Group->GetDescription());
	const FString GroupType = CombatGroupTypeToDisplayString(Group->GetType());

	FString Info;
	Info += FString::Printf(TEXT("%s\n"), *GroupName);
	Info += FString::Printf(TEXT("%s\n"), *GroupType);

	auto AppendSortedSection = [&Info](const FString& Header, const TMap<FString, int32>& SourceMap)
		{
			if (SourceMap.Num() <= 0)
			{
				return;
			}

			TArray<TPair<FString, int32>> SortedTypes;
			for (const auto& KVP : SourceMap)
			{
				SortedTypes.Add(KVP);
			}

			SortedTypes.Sort([](const TPair<FString, int32>& A, const TPair<FString, int32>& B)
				{
					return A.Key < B.Key;
				});

			Info += FString::Printf(TEXT("\n%s:\n"), *Header);

			for (const auto& KVP : SortedTypes)
			{
				Info += FString::Printf(TEXT("  %s x%d\n"), *KVP.Key, KVP.Value);
			}
		};

	switch (Group->GetType())
	{
	case ECOMBATGROUP_TYPE::FIGHTER_SQUADRON:
	case ECOMBATGROUP_TYPE::ATTACK_SQUADRON:
	case ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON:
	case ECOMBATGROUP_TYPE::LCA_SQUADRON:
		AppendSortedSection(TEXT("FIGHTER TYPES"), SquadronTypeCounts);
		break;

	default:
		AppendSortedSection(TEXT("UNIT TYPES"), UnitTypeCounts);
		AppendSortedSection(TEXT("SQUADRON TYPES"), SquadronTypeCounts);
		break;
	}

	const int32 Losses = TotalCount - LiveCount;

	Info += TEXT("\n");
	Info += FString::Printf(TEXT("READY: %d / %d"), LiveCount, TotalCount);

	if (Losses > 0)
	{
		Info += FString::Printf(TEXT("  (LOSSES: %d)"), Losses);
	}

	GroupInfoText->SetText(FText::FromString(Info.TrimEnd()));
}

void UCmdForceDlg::PopulateDescForUnit(CombatUnit* Unit)
{
	UE_LOG(LogTemp, Warning,
		TEXT("[CmdForceDlg] PopulateDescForUnit: Unit=%p Name='%s' HasDesign=%d HasStats=%d Class='%s' Mass=%.0f Detect=%.0f Repair=%d"),
		Unit,
		Unit ? UTF8_TO_TCHAR(Unit->GetName().data()) : TEXT("NULL"),
		Unit ? Unit->HasResolvedDesignData() : 0,
		Unit ? Unit->HasResolvedDesignStats() : 0,
		Unit ? UTF8_TO_TCHAR(Unit->ResolvedClass().data()) : TEXT("NULL"),
		Unit ? Unit->ResolvedMass() : 0.0,
		Unit ? Unit->ResolvedDetect() : 0.0,
		Unit ? Unit->ResolvedRepairTeams() : 0); 
	
	if (!Unit)
	{
		ClearDescList();
		return;
	}

	// ------------------------------------------------------------
	// Header
	// ------------------------------------------------------------
	if (GroupNameText)
	{
		GroupNameText->SetText(FText::FromString(
			BuildSafeUnitDisplayText(Unit)));
	}

	// ------------------------------------------------------------
	// Location
	// ------------------------------------------------------------
	if (GroupLocationText)
	{
		GroupLocationText->SetText(FText::FromString(
			UTF8_TO_TCHAR(Unit->GetRegion().data())));
	}

	// ------------------------------------------------------------
	// Empire
	// ------------------------------------------------------------
	if (GroupEmpireText)
	{
		CombatGroup* OwnerGroup = Unit->GetCombatGroup();

		const FString EmpireText = OwnerGroup
			? UFormattingUtils::EmpireToString(OwnerGroup->GetEmpire())
			: TEXT("UNKNOWN");

		GroupEmpireText->SetText(FText::FromString(EmpireText));
	}

	// ------------------------------------------------------------
	// Info block (ALL remaining data)
	// ------------------------------------------------------------
	if (GroupInfoText)
	{
		FString Info;

		auto AddLine = [&Info](const FString& Label, const FString& Value)
			{
				if (!Value.IsEmpty())
				{
					Info += FString::Printf(TEXT("%-10s %s\n"), *Label, *Value);
				}
			};

		// --------------------------------------------------------
		// Core runtime info
		// --------------------------------------------------------
		AddLine(TEXT("UNIT:"), BuildSafeUnitDisplayText(Unit));
		AddLine(TEXT("SECTOR:"), UTF8_TO_TCHAR(Unit->GetRegion().data()));

		// Type
		FString TypeText = TEXT("UNKNOWN");
		switch ((CLASSIFICATION)Unit->Type())
		{
		case CLASSIFICATION::FIGHTER:   TypeText = TEXT("FIGHTER"); break;
		case CLASSIFICATION::ATTACK:    TypeText = TEXT("ATTACK"); break;
		case CLASSIFICATION::LCA:       TypeText = TEXT("LANDING CRAFT"); break;
		case CLASSIFICATION::DESTROYER: TypeText = TEXT("DESTROYER"); break;
		case CLASSIFICATION::CRUISER:   TypeText = TEXT("CRUISER"); break;
		case CLASSIFICATION::CARRIER:   TypeText = TEXT("CARRIER"); break;
		case CLASSIFICATION::STATION:   TypeText = TEXT("STATION"); break;
		case CLASSIFICATION::STARBASE:  TypeText = TEXT("STARBASE"); break;
		default: break;
		}
		AddLine(TEXT("TYPE:"), TypeText);

		// --------------------------------------------------------
		// Design data (from DT)
		// --------------------------------------------------------
		if (Unit->HasResolvedDesignData())
		{
			AddLine(TEXT("CLASS:"), UTF8_TO_TCHAR(Unit->ResolvedClass().data()));

			if (Unit->HasResolvedDesignStats())
			{
				AddLine(TEXT("MASS:"), FString::Printf(TEXT("%.0f T"), Unit->ResolvedMass()));
				AddLine(TEXT("SCALE:"), FString::Printf(TEXT("%.1f"), Unit->ResolvedScale()));
				AddLine(TEXT("V LIMIT:"), FString::Printf(TEXT("%.0f"), Unit->ResolvedVLimit()));
				AddLine(TEXT("AGILITY:"), FString::Printf(TEXT("%.1f"), Unit->ResolvedAgility()));
				AddLine(TEXT("DETECT:"), FString::Printf(TEXT("%.0f"), Unit->ResolvedDetect()));
				AddLine(TEXT("REPAIR:"), FString::Printf(TEXT("%d TEAMS"), Unit->ResolvedRepairTeams()));
			}

			// Description
			if (Unit->HasResolvedDescription())
			{
				Info += TEXT("\n");
				Info += UTF8_TO_TCHAR(Unit->ResolvedDescription().data());
				Info += TEXT("\n");
			}

			// Weapons
			if (Unit->HasResolvedWeaponSummary())
			{
				Info += TEXT("\nWEAPONS:\n");
				Info += UTF8_TO_TCHAR(Unit->ResolvedWeaponSummary().data());
			}
		}

		GroupInfoText->SetText(FText::FromString(Info.TrimEnd()));
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