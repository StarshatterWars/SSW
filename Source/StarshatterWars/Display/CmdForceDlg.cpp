/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    UI
    FILE:         CmdForceDlg.cpp
    AUTHOR:       Carlos Bott
    ORIGINAL:     John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    UCmdForceDlg implementation.
    Uses a single hierarchical ListView rooted at the selected combatant force.
    The dropdown resolves directly to combatants and displays:
    <Empire Name>: <Force Name>
*/

#include "CmdForceDlg.h"
#include "CmdForceListItem.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Components/ListView.h"

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

        if (IsSquadronUnitType(Unit->GetType()))
        {
            OutSquadronTypeCounts.FindOrAdd(TypeName) += Unit->GetCount();
        }
        else
        {
            OutUnitTypeCounts.FindOrAdd(TypeName) += Unit->GetCount();
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

    ListIter<CombatUnit> UnitIter = Group->GetUnits();
    while (++UnitIter)
    {
        CombatUnit* Unit = UnitIter.value();
        if (!Unit)
        {
            continue;
        }

        OutTotalCount += Unit->GetCount();
        OutLiveCount += Unit->LiveCount();
    }

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

FString UCmdForceDlg::GetEmpireDisplayName(EEMPIRE_NAME Empire) const
{
    const UEnum* Enum = StaticEnum<EEMPIRE_NAME>();

    if (!Enum)
    {
        return TEXT("UNKNOWN");
    }

    return Enum->GetDisplayNameTextByValue((int64)Empire).ToString();
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

        if (TransferButtonText)
        {
            TransferButtonText->SetText(FText::FromString(TEXT("TRANSFER")));
        }
    }

    if (CombatantList)
    {
        CombatantList->OnItemSelectionChanged().RemoveAll(this);
        CombatantList->OnItemSelectionChanged().AddUObject(this, &UCmdForceDlg::OnCombatItemSelected);
    }

    if (CmdMsgDlg)
    {
        CmdMsgDlg->HideMsgDlg();
    }
}

void UCmdForceDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ExecFrame();
}

FString UCmdForceDlg::BuildCombatantDropdownLabel(Combatant* C) const
{
    if (!C)
    {
        return TEXT("");
    }

    CombatGroup* Force = C->GetForce();

    FString EmpireName = TEXT("UNKNOWN EMPIRE");
    FString ForceName = TEXT("UNKNOWN FORCE");

    if (Force)
    {
        EmpireName = GetEmpireDisplayName((EEMPIRE_NAME)Force->GetEmpire());

        const FString ResolvedForceName = UTF8_TO_TCHAR(Force->GetDescription());
        if (!ResolvedForceName.IsEmpty())
        {
            ForceName = ResolvedForceName;
        }
    }

    return FString::Printf(TEXT("%s"), *EmpireName);
}

Combatant* UCmdForceDlg::ResolveCombatantFromDropdownLabel(const FString& SelectedItem) const
{
    if (Combatant* const* FoundPtr = DropdownCombatantMap.Find(SelectedItem))
    {
        return *FoundPtr;
    }

    return nullptr;
}

void UCmdForceDlg::PopulateForcesComboBox()
{
    if (!ForcesComboBox || !CampaignPtr)
    {
        return;
    }

    DropdownCombatantMap.Empty();
    ForcesComboBox->ClearOptions();

    const List<Combatant>& Combatants = CampaignPtr->GetCombatants();

    FString FirstOption;

    for (int i = 0; i < Combatants.size(); ++i)
    {
        Combatant* C = Combatants[i];
        if (!C)
        {
            continue;
        }

        if (!IsVisibleCombatant(C))
        {
            continue;
        }

        FString Label = BuildCombatantDropdownLabel(C);

        DropdownCombatantMap.Add(Label, C);
        ForcesComboBox->AddOption(Label);

        if (FirstOption.IsEmpty())
        {
            FirstOption = Label;
        }
    }

    if (!FirstOption.IsEmpty())
    {
        CurrentCombatant = ResolveCombatantFromDropdownLabel(FirstOption);
        ForcesComboBox->SetSelectedOption(FirstOption);
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
        ClearDescList();

        if (CombatantList)
        {
            CombatantList->ClearListItems();
        }

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
        UE_LOG(LogTemp, Warning, TEXT("[CmdForceDlg] CmdForceDlg: Manager is null (SetModeAndHighlight)."));
    }
}

void UCmdForceDlg::OnForceSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    UE_LOG(
        LogCmdForceDlg,
        Log,
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

    CurrentCombatant = ResolveCombatantFromDropdownLabel(SelectedItem);

    if (CurrentCombatant)
    {
        CombatGroup* Force = CurrentCombatant->GetForce();

        UE_LOG(
            LogCmdForceDlg,
            Log,
            TEXT("[CmdForceDlg] OnForceSelectionChanged: resolved combatant name='%s' forceId=%d forceName='%s' forceEmpire=%d"),
            UTF8_TO_TCHAR(CurrentCombatant->GetName()),
            Force ? Force->GetID() : -1,
            Force ? UTF8_TO_TCHAR(Force->GetDescription()) : TEXT("NULL"),
            Force ? (int32)Force->GetEmpire() : -1);
    }
    else
    {
        UE_LOG(
            LogCmdForceDlg,
            Warning,
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

        UpdateTransferButtonState();
        PopulateDescForGroup(CurrentGroup);
    }
    else if (Item->IsUnit())
    {
        CurrentUnit = Item->Unit;
        UpdateTransferButtonState();
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
    if (!C)
    {
        return false;
    }

    CombatGroup* Force = C->GetForce();
    if (!Force)
    {
        return false;
    }

    if (Force->GetType() < ECOMBATGROUP_TYPE::CIVILIAN &&
        Force->GetIntelLevel() >= Intel::KNOWN &&
        (Force->CountUnits() > 0 || Force->GetLiveComponents().size() > 0))
    {
        return true;
    }

    List<CombatGroup>& Groups = Force->GetComponents();
    for (int i = 0; i < Groups.size(); ++i)
    {
        CombatGroup* G = Groups[i];
        if (G &&
            G->GetType() < ECOMBATGROUP_TYPE::CIVILIAN &&
            G->GetIntelLevel() >= Intel::KNOWN &&
            (G->CountUnits() > 0 || G->GetLiveComponents().size() > 0))
        {
            return true;
        }
    }

    return false;
}

void UCmdForceDlg::ShowCombatant(Combatant* C)
{
    if (!CombatantList)
    {
        UE_LOG(LogCmdForceDlg, Warning, TEXT("[CmdForceDlg] ShowCombatant: CombatantList is NULL"));
        return;
    }

    CombatantList->ClearListItems();

    if (!C)
    {
        UE_LOG(LogCmdForceDlg, Warning, TEXT("[CmdForceDlg] ShowCombatant: Combatant is NULL"));
        ClearDescList();

        if (TransferButton)
        {
            TransferButton->SetIsEnabled(false);
        }
        return;
    }

    CurrentGroup = nullptr;
    CurrentUnit = nullptr;
    CurrentCombatant = C;

    CombatGroup* Force = C->GetForce();
    if (!Force)
    {
        UE_LOG(LogCmdForceDlg, Warning,
            TEXT("[CmdForceDlg] ShowCombatant: Force is NULL for combatant='%s'"),
            UTF8_TO_TCHAR(C->GetName()));

        ClearDescList();

        if (TransferButton)
        {
            TransferButton->SetIsEnabled(false);
        }
        return;
    }

    Force->SetExpanded(true);
    AddCombatGroupRecursive(Force, true, 0);

    if (GroupNameText)
    {
        GroupNameText->SetText(FText::FromString(UTF8_TO_TCHAR(Force->GetDescription())));
    }

    if (GroupEmpireText)
    {
        GroupEmpireText->SetText(FText::FromString(
            GetEmpireDisplayName((EEMPIRE_NAME)Force->GetEmpire())));
    }

    if (GroupTypeText)
    {
        GroupTypeText->SetText(FText::FromString(
            CombatGroupTypeToDisplayString(Force->GetType())));
    }

    if (GroupLocationText)
    {
        GroupLocationText->SetText(FText::FromString(
            UTF8_TO_TCHAR(Force->GetRegion().data())));
    }

    PopulateDescForGroup(Force);

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
    if (!CombatantList || !CurrentCombatant)
    {
        return;
    }

    CombatantList->ClearListItems();

    CombatGroup* Force = CurrentCombatant->GetForce();
    if (!Force)
    {
        return;
    }

    Force->SetExpanded(true);

    AddCombatGroupRecursive(Force, true, 0);
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

    UCmdForceListItem* GroupItem = NewObject<UCmdForceListItem>(this);
    GroupItem->InitAsGroup(
        UTF8_TO_TCHAR(Group->GetDescription()),
        Depth,
        Group,
        Group->IsExpanded(),
        bHasChildrenOrUnits);

    CombatantList->AddItem(GroupItem);

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

            UCmdForceListItem* UnitItem = NewObject<UCmdForceListItem>(this);
            UnitItem->InitAsUnit(BuildSafeUnitDisplayText(Unit), Depth + 1, Unit);
            CombatantList->AddItem(UnitItem);
        }
    }

    if (Group->IsExpanded() && Group->GetLiveComponents().size() > 0)
    {
        TArray<CombatGroup*> DirectChildren;

        List<CombatGroup>& Groups = Group->GetLiveComponents();

        for (int i = 0; i < Groups.size(); ++i)
        {
            CombatGroup* Child = Groups[i];
            if (!Child)
            {
                continue;
            }

            if (Child->GetIntelLevel() < Intel::KNOWN)
            {
                continue;
            }

            if (Child->GetParent() != Group)
            {
                continue;
            }

            DirectChildren.Add(Child);
        }

        for (int32 i = 0; i < DirectChildren.Num(); ++i)
        {
            AddCombatGroupRecursive(
                DirectChildren[i],
                i == DirectChildren.Num() - 1,
                Depth + 1);
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
        GroupNameText->SetText(FText::FromString(UTF8_TO_TCHAR(Group->GetDescription())));
    }

    if (GroupTypeText)
    {
        GroupTypeText->SetText(FText::FromString(CombatGroupTypeToDisplayString(Group->GetType())));
    }

    if (GroupLocationText)
    {
        GroupLocationText->SetText(FText::FromString(UTF8_TO_TCHAR(Group->GetRegion().data())));
    }

    if (GroupEmpireText)
    {
        GroupEmpireText->SetText(FText::FromString(
            GetEmpireDisplayName((EEMPIRE_NAME)Group->GetEmpire())));
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
    Info += FString::Printf(TEXT("%s\n"), *GetEmpireDisplayName((EEMPIRE_NAME)Group->GetEmpire()));

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
    if (!Unit)
    {
        ClearDescList();
        return;
    }

    if (GroupNameText)
    {
        GroupNameText->SetText(FText::FromString(BuildSafeUnitDisplayText(Unit)));
    }

    if (GroupTypeText)
    {
        FString TypeText = TEXT("UNKNOWN");
        switch ((CLASSIFICATION)Unit->GetType())
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
        GroupTypeText->SetText(FText::FromString(TypeText));
    }

    if (GroupLocationText)
    {
        GroupLocationText->SetText(FText::FromString(UTF8_TO_TCHAR(Unit->GetRegion().data())));
    }

    if (GroupEmpireText)
    {
        CombatGroup* OwnerGroup = Unit->GetCombatGroup();

        const FString EmpireText = OwnerGroup
            ? UFormattingUtils::EmpireToString(OwnerGroup->GetEmpire())
            : TEXT("UNKNOWN");

        GroupEmpireText->SetText(FText::FromString(EmpireText));
    }

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

        AddLine(TEXT("UNIT:"), BuildSafeUnitDisplayText(Unit));
        AddLine(TEXT("SECTOR:"), UTF8_TO_TCHAR(Unit->GetRegion().data()));

        FString TypeText = TEXT("UNKNOWN");
        switch ((CLASSIFICATION)Unit->GetType())
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

            if (Unit->HasResolvedDescription())
            {
                Info += TEXT("\n");
                Info += UTF8_TO_TCHAR(Unit->ResolvedDescription().data());
                Info += TEXT("\n");
            }

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
    UE_LOG(LogTemp, Warning, TEXT("[CmdForceDlg] CmdMsgDlg=%p"), CmdMsgDlg);

    if (!CampaignPtr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Transfer] CampaignPtr NULL"));
        return;
    }

    if (!CurrentGroup)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Transfer] CurrentGroup NULL"));

        if (CmdMsgDlg)
        {
            CmdMsgDlg->SetTitleText(TEXT("Transfer Denied"));
            CmdMsgDlg->SetMessageText(TEXT("No group selected."));
            CmdMsgDlg->ShowMsgDlg();
        }
        return;
    }

    PlayerCharacter* PlayerPtr = PlayerCharacter::EnsureCurrentPlayer();
    if (!PlayerPtr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Transfer] Player NULL"));
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

    default:
        break;
    }

    FString TransferInfo;

    if (!CmdMsgDlg)
    {
        return;
    }

    const int32 RequiredRank = PlayerCharacter::CommandRankRequired(CmdClass);
    const bool bCanCommand = PlayerPtr->GetRank() >= RequiredRank;

    if (bCanCommand)
    {
        if (CurrentUnit)
        {
            CampaignPtr->SetPlayerUnit(CurrentUnit);

            TransferInfo = FString::Printf(
                TEXT("Your transfer request has been approved, %s %s. You are now assigned to the %s. Good luck.\n\nFleet Admiral A. Evars FORCOM\nCommanding"),
                ANSI_TO_TCHAR(PlayerCharacter::RankName(PlayerPtr->GetRank())),
                *PlayerPtr->GetName(),
                ANSI_TO_TCHAR(CurrentUnit->GetDescription()));
        }
        else
        {
            CampaignPtr->SetPlayerGroup(CurrentGroup);

            TransferInfo = FString::Printf(
                TEXT("Your transfer request has been approved, %s %s. You are now assigned to the %s. Good luck.\n\nFleet Admiral A. Evars FORCOM\nCommanding"),
                ANSI_TO_TCHAR(PlayerCharacter::RankName(PlayerPtr->GetRank())),
                *PlayerPtr->GetName(),
                UTF8_TO_TCHAR(CurrentGroup->GetDescription()));
        }

        UIButton::PlaySound(UIButton::SND_ACCEPT);

        CmdMsgDlg->SetTitleText(TEXT("Transfer Approved"));
        CmdMsgDlg->SetMessageText(TransferInfo);
        CmdMsgDlg->ShowMsgDlg();
    }
    else
    {
        UIButton::PlaySound(UIButton::SND_REJECT);

        TransferInfo = FString::Printf(
            TEXT("Your transfer request has been denied, %s %s. The %s requires a command rank of %s. Please return to your unit and your duties.\n\nFleet Admiral A. Evars FORCOM\nCommanding"),
            ANSI_TO_TCHAR(PlayerCharacter::RankName(PlayerPtr->GetRank())),
            *PlayerPtr->GetName(),
            UTF8_TO_TCHAR(CurrentGroup->GetDescription()),
            ANSI_TO_TCHAR(PlayerCharacter::RankName(RequiredRank)));

        CmdMsgDlg->SetTitleText(TEXT("Transfer Denied"));
        CmdMsgDlg->SetMessageText(TransferInfo);
        CmdMsgDlg->ShowMsgDlg();
    }
}

void UCmdForceDlg::ShowTransferPopup(const FString& TransferTitle, const FString& TransferMessage, bool bApproved)
{
    if (CmdMsgDlg)
    {
        CmdMsgDlg->SetTitleText(TransferTitle);
        CmdMsgDlg->SetMessageText(TransferMessage);
        CmdMsgDlg->ShowMsgDlg();
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[CmdForceDlg] %s\n%s"), *TransferTitle, *TransferMessage);
}

void UCmdForceDlg::UpdateTransferButtonState()
{
    if (!TransferButton)
    {
        return;
    }

    const bool bEnable = (CurrentUnit == nullptr && CurrentGroup != nullptr);
    TransferButton->SetIsEnabled(bEnable);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[CmdForceDlg] TransferButton Enabled=%d (Unit=%p Group=%p)"),
        bEnable ? 1 : 0,
        CurrentUnit,
        CurrentGroup);
}

/*
=============================================================================
    OLD OOB / PIPE-STACK HELPERS (DISABLED)
=============================================================================

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
        (int32) Group->GetType(),
        Group->GetUnits().size(),
        Group->GetLiveComponents().size(),
        (int32) Group->GetEmpire());

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
            Unit->GetType(),
            Unit->GetIFF());
    }

    List<CombatGroup>& Children = Group->GetLiveComponents();
    for (int i = 0; i < Children.size(); ++i)
    {
        DumpCombatGroupRecursive(Children[i], Depth + 1);
    }
}
*/