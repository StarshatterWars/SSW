/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionWeaponDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Mission weapon dialog implementation.

    Displays preset loadouts for the selected player ship and
    shows the currently selected runtime MissionLoad stations.
*/

#include "MissionWeaponDlg.h"

#include "MissionBriefingDlg.h"
#include "MissionPlanner.h"
#include "MissionWeaponLoadoutListObject.h"
#include "MissionWeaponStationRowObject.h"
#include "MissionLoadoutListView.h"
#include "MissionWeaponStationLVElement.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#include "WeaponDesignRegistry.h"
#include "GameStructs_System.h"
#include "MissionUIStyle.h"

#include "MissionLoad.h"
#include "MissionLoadoutTypes.h"

#include "Mission.h"
#include "MissionElement.h"
#include "Engine/DataTable.h"

// +--------------------------------------------------------------------+
// Runtime Loadout Helpers
// +--------------------------------------------------------------------+

static const bool bLogLoadoutsVerbose = false;
static const bool bLogRuntimeLoadout = true;
static const TCHAR* CustomLoadoutName = TEXT("CUSTOM LOADOUT");

static FMissionRuntimeLoadout ConvertMissionLoad(const MissionLoad* InLoad)
{
    FMissionRuntimeLoadout Out;

    if (!InLoad)
    {
        return Out;
    }

    Out.ShipIndex = InLoad->GetShip();
    Out.Name = FString(InLoad->GetName().data());

    const int32 NumStations = InLoad->GetNumStations();
    Out.Stations.SetNum(NumStations);

    for (int32 i = 0; i < NumStations; ++i)
    {
        const int32 Selection = InLoad->GetStation(i);
        Out.Stations[i] = (Selection >= 0) ? Selection : INDEX_NONE;
    }

    return Out;
}

static MissionLoad* GetActivePlayerMissionLoad(Mission* InMission)
{
    if (!InMission)
    {
        return nullptr;
    }

    MissionElement* PlayerElem = InMission->GetPlayer();
    if (!PlayerElem)
    {
        return nullptr;
    }

    if (PlayerElem->GetLoadouts().size() < 1)
    {
        return nullptr;
    }

    return PlayerElem->GetLoadouts().at(0);
}

static void ApplyShipLoadoutToMissionLoad(
    MissionLoad* Load,
    const FShipLoadout& Src)
{
    if (!Load)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionWeaponDlg] ApplyShipLoadoutToMissionLoad: Load is null"));
        return;
    }

    Load->Clear();
    Load->SetName(TCHAR_TO_ANSI(*Src.Name));

    const int32 Count = FMath::Min(Load->GetNumStations(), Src.Stations.Num());

    if (bLogLoadoutsVerbose)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionWeaponDlg] ApplyShipLoadoutToMissionLoad: Name='%s' Count=%d"),
            *Src.Name,
            Count);
    }

    for (int32 i = 0; i < Count; ++i)
    {
        Load->SetStation(i, Src.Stations[i]);

        if (bLogLoadoutsVerbose)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[MissionWeaponDlg]   Apply Station[%d] = %d"),
                i,
                Src.Stations[i]);
        }
    }

    if (bLogLoadoutsVerbose)
    {
        for (int32 i = 0; i < Count; ++i)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[MissionWeaponDlg]   Verify Station[%d] = %d"),
                i,
                Load->GetStation(i));
        }
    }
}

// +--------------------------------------------------------------------+

UMissionWeaponDlg::UMissionWeaponDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UMissionWeaponDlg::NativeConstruct()
{
    Super::NativeConstruct();
    BuildRuntimeLayout();
}

void UMissionWeaponDlg::SetParentDlg(UMissionBriefingDlg* InParentDlg)
{
    ParentDlg = InParentDlg;

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionWeaponDlg] SetParentDlg: ParentDlg=%p Mission=%p"),
        ParentDlg,
        ParentDlg ? ParentDlg->GetMissionPtr() : nullptr);
}

Mission* UMissionWeaponDlg::ResolveMission() const
{
    return ParentDlg ? ParentDlg->GetMissionPtr() : nullptr;
}

MissionElement* UMissionWeaponDlg::ResolvePlayerElement() const
{
    Mission* M = ResolveMission();
    if (!M)
    {
        return nullptr;
    }

    ListIter<MissionElement> It = M->GetElements();
    while (++It)
    {
        MissionElement* E = It.value();
        if (E && E->IsPlayer())
        {
            return E;
        }
    }

    return M->GetPlayer();
}

const FShipDesign* UMissionWeaponDlg::ResolvePlayerShipDesign() const
{
    MissionElement* E = ResolvePlayerElement();
    const FShipDesign* MissionDesign = E ? E->GetShipDesign() : nullptr;
    if (!MissionDesign)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionWeaponDlg] No mission element ship design"));
        return nullptr;
    }

    // Configurable fighter loadouts are owned by the mission's design and
    // MissionLoad. Do not change their selection or source here.
    if (!MissionDesign->Loadout.IsEmpty())
    {
        return MissionDesign;
    }

    // Tactical Reference reads this authoritative DataTable. A mission
    // element can instead point to a fallback/placeholder registry row with
    // an empty Weapon array, so resolve the complete authored design by name.
    UDataTable* DesignTable = LoadObject<UDataTable>(
        nullptr, TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));

    if (!DesignTable)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionWeaponDlg] Cannot load TacRef ship design table; Ship='%s'"),
            *MissionDesign->ShipName);
        return MissionDesign;
    }

    auto IsSameIdentity = [](const FString& A, const FString& B) -> bool
    {
        return !A.IsEmpty() && !B.IsEmpty() &&
            A.TrimStartAndEnd().Equals(B.TrimStartAndEnd(), ESearchCase::IgnoreCase);
    };

    // First prefer the actual DataTable row key (the legacy design token).
    const FString DesignKey = MissionDesign->ShipName.TrimStartAndEnd();
    if (!DesignKey.IsEmpty())
    {
        if (const FShipDesign* Row = DesignTable->FindRow<FShipDesign>(
            FName(*DesignKey), TEXT("MissionWeaponDlg"), false))
        {
            UE_LOG(LogTemp, Log,
                TEXT("[MissionWeaponDlg] TacRef row='%s' weapons=%d"),
                *DesignKey, Row->Weapon.Num());
            return Row;
        }
    }

    // Imported row names may differ from ShipName/DisplayName. Match exact
    // authored identities only; never use fuzzy substring matching.
    const TArray<FName> RowNames = DesignTable->GetRowNames();
    for (const FName& RowName : RowNames)
    {
        const FShipDesign* Row = DesignTable->FindRow<FShipDesign>(
            RowName, TEXT("MissionWeaponDlg"), false);
        if (!Row)
        {
            continue;
        }
        if (IsSameIdentity(Row->ShipName, MissionDesign->ShipName) ||
            IsSameIdentity(Row->DisplayName, MissionDesign->ShipName) ||
            IsSameIdentity(RowName.ToString(), MissionDesign->DisplayName) ||
            IsSameIdentity(Row->ShipName, MissionDesign->DisplayName))
        {
            UE_LOG(LogTemp, Log,
                TEXT("[MissionWeaponDlg] TacRef matched row='%s' ship='%s' weapons=%d"),
                *RowName.ToString(), *MissionDesign->ShipName, Row->Weapon.Num());
            return Row;
        }
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionWeaponDlg] No TacRef match for mission ship='%s' display='%s'; mission weapons=%d"),
        *MissionDesign->ShipName, *MissionDesign->DisplayName, MissionDesign->Weapon.Num());
    return MissionDesign;
}

void UMissionWeaponDlg::RefreshFromMission()
{
    if (bRefreshingLoadouts)
    {
        return;
    }

    bRefreshingLoadouts = true;

    Mission* MissionPtr = ResolveMission();

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionWeaponDlg] RefreshFromMission: this=%p ParentDlg=%p Mission=%p"),
        this,
        ParentDlg,
        MissionPtr);

    if (!MissionPtr)
    {
        bRefreshingLoadouts = false;
        return;
    }

    ClearLoadouts();

    MissionElement* Elem = ResolvePlayerElement();
    const FShipDesign* Design = ResolvePlayerShipDesign();

    if (!Elem || !Design)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionWeaponDlg] RefreshFromMission: Elem=%p Design=%p"),
            Elem,
            Design);

        bRefreshingLoadouts = false;
        return;
    }

    if (ElementNameValueText)
    {
        ElementNameValueText->SetText(FText::FromString(GetElementName(Elem)));
    }

    if (DesignNameValueText)
    {
        DesignNameValueText->SetText(FText::FromString(GetDesignName(Design)));
    }

    if (WeightValueText)
    {
        const double CurrentMass = ComputeCurrentCustomMass(Elem, Design);
        WeightValueText->SetText(FText::FromString(FormatWeight(CurrentMass)));
    }

    // Capital ships use authored installed weapons, not fighter loadout presets.
    const bool bHasConfigurableLoadouts = Design->Loadout.Num() > 0;
    if (FighterLoadoutPanel)
        FighterLoadoutPanel->SetVisibility(bHasConfigurableLoadouts ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (FixedShipWeaponsPanel)
        FixedShipWeaponsPanel->SetVisibility(bHasConfigurableLoadouts ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

    if (bHasConfigurableLoadouts)
    {
        BuildLoadouts(Elem, Design);
        RefreshWeaponList();
        RefreshSelectedLoadoutStations();
    }
    else
    {
        PopulateFixedShipWeapons(Design);
        UE_LOG(LogTemp, Log, TEXT("[MissionWeaponDlg] Fixed weapons for '%s': %d installed entries"),
            *GetDesignName(Design), Design->Weapon.Num());
    }

    bRefreshingLoadouts = false;
}

void UMissionWeaponDlg::ClearLoadouts()
{
    Items.Empty();

    if (WeaponListView)
    {
        WeaponListView->ClearListItems();
    }

    StationItems.Empty();

    if (StationListView)
    {
        StationListView->ClearListItems();
    }
}

bool UMissionWeaponDlg::GetSelectedLoadoutName(MissionElement* Element, FString& OutName) const
{
    if (!Element || Element->GetLoadouts().size() == 0)
    {
        return false;
    }

    MissionLoad* Load = Element->GetLoadouts().at(0);
    if (!Load)
    {
        return false;
    }

    if (Load->GetName().length() > 0)
    {
        OutName = ANSI_TO_TCHAR(Load->GetName().data());
        return true;
    }

    return false;
}

void UMissionWeaponDlg::BuildLoadouts(MissionElement* Element, const FShipDesign* Design)
{
    if (!Element || !Design)
    {
        return;
    }

    FString SelectedName;
    const bool bHasSelected = GetSelectedLoadoutName(Element, SelectedName);

    if (bLogLoadoutsVerbose)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionWeaponDlg] BuildLoadouts: Count BEFORE = %d DesignLoadouts = %d"),
            Items.Num(),
            Design->Loadout.Num());
    }

    for (int32 i = 0; i < Design->Loadout.Num(); ++i)
    {
        const FShipLoadout& L = Design->Loadout[i];

        const bool bSelected =
            bHasSelected &&
            L.Name.Equals(SelectedName, ESearchCase::IgnoreCase);

        const double LoadoutMass = ComputeLoadoutMass(Design, L);
        const FString WeightStr = FormatWeight(LoadoutMass);

        UMissionWeaponLoadoutListObject* Item =
            NewObject<UMissionWeaponLoadoutListObject>(this);

        Item->InitFromShipLoadout(L, i, WeightStr, bSelected);

        Items.Add(Item);

        if (WeaponListView)
        {
            WeaponListView->AddItem(Item);

            if (bSelected)
            {
                WeaponListView->SetSelectedItem(Item);
            }
        }
    }

    if (!bHasSelected && Items.Num() > 0 && WeaponListView)
    {
        WeaponListView->SetSelectedItem(Items[0]);
    }

    if (bLogLoadoutsVerbose)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionWeaponDlg] BuildLoadouts: Count AFTER = %d"),
            Items.Num());
    }
}

FString UMissionWeaponDlg::GetElementName(MissionElement* E) const
{
    return E ? ANSI_TO_TCHAR(E->GetName().data()) : TEXT("UNKNOWN");
}

FString UMissionWeaponDlg::GetDesignName(const FShipDesign* D) const
{
    if (!D)
    {
        return TEXT("UNKNOWN");
    }

    if (!D->DisplayName.IsEmpty())
    {
        return D->DisplayName;
    }

    return D->ShipName;
}

FString UMissionWeaponDlg::FormatWeight(double Mass) const
{
    return FString::Printf(TEXT("%d KG"), FMath::RoundToInt(Mass * 1000));
}

UTextBlock* UMissionWeaponDlg::BuildLabelText(const FString& Text) const
{
    UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
    T->SetText(FText::FromString(Text));
    T->SetColorAndOpacity(MissionUIStyle::InfoLabelText);
    T->SetFont(MissionUIStyle::GetInfoLabelFont(13));
    return T;
}

UTextBlock* UMissionWeaponDlg::BuildValueText(const FString& Text) const
{
    UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
    T->SetText(FText::FromString(Text));
    T->SetColorAndOpacity(MissionUIStyle::InfoValueText);
    T->SetFont(MissionUIStyle::GetInfoValueFont(14));
    return T;
}

UBorder* UMissionWeaponDlg::BuildHeader(const FString& Text) const
{
    UBorder* Outer = WidgetTree->ConstructWidget<UBorder>();
    Outer->SetBrushColor(MissionUIStyle::HeaderBG);

    UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
    T->SetText(FText::FromString(Text));
    T->SetColorAndOpacity(MissionUIStyle::HeaderText);
    T->SetFont(MissionUIStyle::GetHeaderFont(18));

    Outer->SetContent(T);
    return Outer;
}

void UMissionWeaponDlg::BuildRuntimeLayout()
{
    if (!WidgetTree || !RuntimeHost)
    {
        return;
    }

    if (WeaponListView || StationListView)
    {
        return;
    }

    RuntimeHost->SetContent(nullptr);

    USizeBox* Root =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("MissionWeaponRoot"));
    Root->SetWidthOverride(1400.f);
    Root->SetHeightOverride(650.f);
    RuntimeHost->SetContent(Root);

    UHorizontalBox* MainRow =
        WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(),
            TEXT("MissionWeaponRootRow"));
    Root->SetContent(MainRow);

    // ------------------------------------------------------------
    // LEFT COLUMN
    // ------------------------------------------------------------

    UVerticalBox* Left =
        WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            TEXT("MissionWeaponLeftColumn"));

    if (UHorizontalBoxSlot* LeftSlot = MainRow->AddChildToHorizontalBox(Left))
    {
        LeftSlot->SetPadding(FMargin(12.f, 12.f, 16.f, 12.f));
        LeftSlot->SetHorizontalAlignment(HAlign_Left);
        LeftSlot->SetVerticalAlignment(VAlign_Top);
        LeftSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    auto AddInfoRow = [this, Left](const FString& Label, UTextBlock*& OutText)
        {
            UHorizontalBox* InfoRow =
                WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

            if (UVerticalBoxSlot* RowSlot = Left->AddChildToVerticalBox(InfoRow))
            {
                RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
            }

            UTextBlock* LabelText = BuildLabelText(Label);
            OutText = BuildValueText(TEXT("-"));

            if (UHorizontalBoxSlot* LabelSlot = InfoRow->AddChildToHorizontalBox(LabelText))
            {
                LabelSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
                LabelSlot->SetHorizontalAlignment(HAlign_Left);
                LabelSlot->SetVerticalAlignment(VAlign_Center);
                LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            }

            if (UHorizontalBoxSlot* ValueSlot = InfoRow->AddChildToHorizontalBox(OutText))
            {
                ValueSlot->SetHorizontalAlignment(HAlign_Left);
                ValueSlot->SetVerticalAlignment(VAlign_Center);
                ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            }
        };

    AddInfoRow(TEXT("ELEMENT:"), ElementNameValueText);
    AddInfoRow(TEXT("TYPE:"), DesignNameValueText);
    AddInfoRow(TEXT("WEIGHT:"), WeightValueText);

    // ------------------------------------------------------------
    // RIGHT COLUMN
    // ------------------------------------------------------------

    UVerticalBox* Right =
        WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            TEXT("MissionWeaponRightColumn"));

    if (UHorizontalBoxSlot* RightSlot = MainRow->AddChildToHorizontalBox(Right))
    {
        RightSlot->SetPadding(FMargin(0.f, 12.f, 12.f, 12.f));
        RightSlot->SetHorizontalAlignment(HAlign_Fill);
        RightSlot->SetVerticalAlignment(VAlign_Fill);
        RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    // The right side has two mutually exclusive modes.  The fixed-weapon
    // panel is read-only and deliberately has no lower loadout/stations panel.
    FixedShipWeaponsPanel = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("FixedShipWeaponsPanel"));
    if (UVerticalBoxSlot* FixedSlot = Right->AddChildToVerticalBox(FixedShipWeaponsPanel))
        FixedSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    if (UBorder* ShipHeader = BuildHeader(TEXT("SHIP WEAPONS")))
        FixedShipWeaponsPanel->AddChildToVerticalBox(ShipHeader);

    FixedShipWeaponsScroll = WidgetTree->ConstructWidget<UScrollBox>(
        UScrollBox::StaticClass(), TEXT("FixedShipWeaponsScroll"));
    if (UVerticalBoxSlot* ScrollSlot = FixedShipWeaponsPanel->AddChildToVerticalBox(FixedShipWeaponsScroll))
    {
        ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ScrollSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
    }

    FighterLoadoutPanel = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("FighterLoadoutPanel"));
    if (UVerticalBoxSlot* FighterSlot = Right->AddChildToVerticalBox(FighterLoadoutPanel))
        FighterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    FixedShipWeaponsPanel->SetVisibility(ESlateVisibility::Collapsed);

    // Build the existing fighter widgets inside their own container.
    Right = FighterLoadoutPanel;

    // ------------------------------------------------------------
    // PRESET LOADOUTS HEADER
    // ------------------------------------------------------------

    if (UBorder* Header = BuildHeader(TEXT("STANDARD LOADOUTS")))
    {
        if (UVerticalBoxSlot* HeaderSlot = Right->AddChildToVerticalBox(Header))
        {
            HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
            HeaderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        }
    }

    // ------------------------------------------------------------
    // LOADOUT COLUMN HEADER ROW  
    // ------------------------------------------------------------

    if (UWidget* HeaderRow = BuildListHeaderRow(
        TEXT("LOADOUT"),
        TEXT("WEIGHT"),
        860.f,
        180.0f))
    {
        if (UVerticalBoxSlot* RowSlot = Right->AddChildToVerticalBox(HeaderRow))
        {
            RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
        }
    }

    // ------------------------------------------------------------
    // PRESET LOADOUTS LIST (UNCHANGED)
    // ------------------------------------------------------------

    UBorder* ListBorder =
        WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            TEXT("MissionWeaponListBorder"));
    ListBorder->SetBrushColor(MissionUIStyle::PanelBG);

    if (UVerticalBoxSlot* BorderSlot = Right->AddChildToVerticalBox(ListBorder))
    {
        BorderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        BorderSlot->SetPadding(FMargin(0.f));
    }

    USizeBox* ListHost =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("MissionWeaponListHost"));
    ListHost->SetWidthOverride(900.0f);
    ListHost->SetHeightOverride(250.0f);
    ListBorder->SetContent(ListHost);

    WeaponListView = WidgetTree->ConstructWidget<UMissionLoadoutListView>(
        UMissionLoadoutListView::StaticClass(),
        TEXT("RuntimeWeaponList"));

    if (EntryWidgetClass)
    {
        WeaponListView->SetEntryWidgetClassPublic(EntryWidgetClass);
    }

    ListHost->SetContent(WeaponListView);

    if (WeaponListView)
    {
        WeaponListView->OnItemSelectionChanged().AddUObject(
            this,
            &UMissionWeaponDlg::HandleLoadoutSelectionChanged);

        UE_LOG(LogTemp, Warning,
            TEXT("[MissionWeaponDlg] BuildRuntimeLayout: WeaponListView selection binding complete"));
    }

    // ------------------------------------------------------------
    // SELECTED LOADOUT HEADER
    // ------------------------------------------------------------

    if (UBorder* Header = BuildHeader(TEXT("SELECTED LOADOUT")))
    {
        if (UVerticalBoxSlot* HeaderSlot = Right->AddChildToVerticalBox(Header))
        {
            HeaderSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 6.f));
            HeaderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        }
    }

    // ------------------------------------------------------------
    // STATION COLUMN HEADER ROW
    // ------------------------------------------------------------

    if (UWidget* HeaderRow = BuildListHeaderRow(
        TEXT("STATION"),
        TEXT("WEAPON"),
        240.f,
        660.f))
    {
        if (UVerticalBoxSlot* RowSlot = Right->AddChildToVerticalBox(HeaderRow))
        {
            RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
            RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        }
    }

    // ------------------------------------------------------------
    // SELECTED LOADOUT STATION LIST
    // ------------------------------------------------------------

    UBorder* StationBorder =
        WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            TEXT("MissionWeaponStationBorder"));
    StationBorder->SetBrushColor(MissionUIStyle::PanelBG);

    if (UVerticalBoxSlot* BorderSlot = Right->AddChildToVerticalBox(StationBorder))
    {
        BorderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); //  THIS FIXES SCROLL
        BorderSlot->SetPadding(FMargin(0.f));
    }

    USizeBox* StationHost =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("MissionWeaponStationHost"));
    StationHost->SetWidthOverride(900.f);
    StationHost->SetHeightOverride(250.0f);
    StationBorder->SetContent(StationHost);

    StationListView = WidgetTree->ConstructWidget<UMissionLoadoutListView>(
        UMissionLoadoutListView::StaticClass(),
        TEXT("RuntimeStationList"));

    if (StationEntryWidgetClass)
    {
        StationListView->SetEntryWidgetClassPublic(StationEntryWidgetClass);
    }

    StationHost->SetContent(StationListView);

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionWeaponDlg] BuildRuntimeLayout: StationListView build complete"));
}

void UMissionWeaponDlg::PopulateFixedShipWeapons(const FShipDesign* Design)
{
    if (!FixedShipWeaponsScroll)
        return;

    FixedShipWeaponsScroll->ClearChildren();
    if (!Design)
        return;

    if (Design->Weapon.IsEmpty())
    {
        UTextBlock* Empty = BuildValueText(TEXT("NO INSTALLED WEAPONS"));
        FixedShipWeaponsScroll->AddChild(Empty);
        return;
    }

    // Unlike Hardpoint.AllowedWeaponTypes, Design->Weapon contains the actual
    // authored installations (the same source used by Tactical Reference).
    for (int32 Index = 0; Index < Design->Weapon.Num(); ++Index)
    {
        const FShipWeapon& Weapon = Design->Weapon[Index];
        const FString WeaponName = !Weapon.DesignName.IsEmpty()
            ? Weapon.DesignName
            : (!Weapon.WeaponType.IsEmpty() ? Weapon.WeaponType : TEXT("UNKNOWN WEAPON"));
        const FString Group = !Weapon.GroupName.IsEmpty() ? Weapon.GroupName.ToUpper() : TEXT("WEAPON");
        const FString Label = FString::Printf(TEXT("%02d   %s   [%s]"),
            Index + 1, *WeaponName, *Group);

        UTextBlock* Row = BuildValueText(Label);
        Row->SetAutoWrapText(true);
        FixedShipWeaponsScroll->AddChild(Row);
    }
}

void UMissionWeaponDlg::HandleLoadoutSelectionChanged(UObject* Item)
{
    UMissionWeaponLoadoutListObject* SelectedItem =
        Cast<UMissionWeaponLoadoutListObject>(Item);

    if (!SelectedItem)
    {
        return;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionWeaponDlg] Selection changed -> Index=%d"),
        SelectedItem->GetLoadoutIndex());

    OnLoadoutSelected(SelectedItem);
}

void UMissionWeaponDlg::OnLoadoutSelected(UMissionWeaponLoadoutListObject* SelectedItem)
{
    if (!SelectedItem)
    {
        return;
    }

    MissionElement* Elem = ResolvePlayerElement();
    const FShipDesign* Design = ResolvePlayerShipDesign();

    if (!Elem || !Design)
    {
        return;
    }

    const int32 Index = SelectedItem->GetLoadoutIndex();

    if (!Design->Loadout.IsValidIndex(Index))
    {
        return;
    }

    MissionLoad* Load = nullptr;

    if (Elem->GetLoadouts().size() > 0)
    {
        Load = Elem->GetLoadouts().at(0);
    }

    if (!Load)
    {
        return;
    }

    const FShipLoadout& Src = Design->Loadout[Index];

    UE_LOG(LogTemp, Warning,
        TEXT("APPLYING LOADOUT '%s'"),
        *Src.Name);

    ApplyShipLoadoutToMissionLoad(Load, Src);

    RefreshFromMission();
}

void UMissionWeaponDlg::RefreshWeaponList()
{
    Mission* MissionPtr = ResolveMission();
    if (!MissionPtr)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionWeaponDlg] RefreshWeaponList: MissionPtr is null"));
        return;
    }

    MissionElement* PlayerElem = ResolvePlayerElement();
    if (!PlayerElem)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionWeaponDlg] RefreshWeaponList: PlayerElem is null"));
        return;
    }

    const FShipDesign* ShipDesign = ResolvePlayerShipDesign();
    if (!ShipDesign)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionWeaponDlg] RefreshWeaponList: ShipDesign is null"));
        return;
    }

    MissionLoad* ActiveLoad = GetActivePlayerMissionLoad(MissionPtr);
    if (!ActiveLoad)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionWeaponDlg] RefreshWeaponList: ActiveLoad is null"));
        return;
    }

    const FMissionRuntimeLoadout RuntimeLoadout = ConvertMissionLoad(ActiveLoad);

    if (bLogRuntimeLoadout)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionWeaponDlg] Runtime Loadout='%s' ShipIndex=%d Stations=%d Hardpoints=%d"),
            *RuntimeLoadout.Name,
            RuntimeLoadout.ShipIndex,
            RuntimeLoadout.Stations.Num(),
            ShipDesign->Hardpoint.Num());

        for (int32 i = 0; i < RuntimeLoadout.Stations.Num(); ++i)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[MissionWeaponDlg]   Runtime Station[%d] = %d"),
                i,
                RuntimeLoadout.Stations[i]);
        }
    }

    if (bLogLoadoutsVerbose)
    {
        const int32 NumStations = ShipDesign->Hardpoint.Num();

        for (int32 StationIndex = 0; StationIndex < NumStations; ++StationIndex)
        {
            const int32 PointIndex =
                RuntimeLoadout.Stations.IsValidIndex(StationIndex)
                ? RuntimeLoadout.Stations[StationIndex]
                : INDEX_NONE;

            const FWeaponDesign* Weapon =
                ResolveWeaponDesignForStationSelection(
                    ShipDesign,
                    StationIndex,
                    PointIndex);

            const FString WeaponName = Weapon ? Weapon->Name : TEXT("Empty");

            UE_LOG(LogTemp, Warning,
                TEXT("[MissionWeaponDlg]   Display Station=%d Point=%d Weapon='%s'"),
                StationIndex,
                PointIndex,
                *WeaponName);
        }
    }
}

const FWeaponDesign* UMissionWeaponDlg::ResolveWeaponDesignForStationSelection(
    const FShipDesign* Design,
    int32 StationIndex,
    int32 PointIndex) const
{
    if (!Design)
    {
        return nullptr;
    }

    if (!Design->Hardpoint.IsValidIndex(StationIndex))
    {
        return nullptr;
    }

    if (PointIndex < 0)
    {
        return nullptr;
    }

    const FShipHardPoint& Hardpoint = Design->Hardpoint[StationIndex];

    if (!Hardpoint.AllowedWeaponTypes.IsValidIndex(PointIndex))
    {
        return nullptr;
    }

    const FString& WeaponKey = Hardpoint.AllowedWeaponTypes[PointIndex];
    if (WeaponKey.IsEmpty())
    {
        return nullptr;
    }

    if (const FWeaponDesign* WeaponRow = WeaponDesignRegistry::Find(WeaponKey))
    {
        return WeaponRow;
    }

    const TMap<FName, FWeaponDesign>& AllWeapons = WeaponDesignRegistry::GetAll();

    for (const TPair<FName, FWeaponDesign>& Pair : AllWeapons)
    {
        const FWeaponDesign& WeaponRow = Pair.Value;

        if (WeaponRow.Name.Equals(WeaponKey, ESearchCase::IgnoreCase) ||
            WeaponRow.Group.Equals(WeaponKey, ESearchCase::IgnoreCase))
        {
            return &WeaponRow;
        }
    }

    return nullptr;
}

double UMissionWeaponDlg::ComputeCurrentCustomMass(
    MissionElement* Element,
    const FShipDesign* Design) const
{
    if (!Element || !Design)
    {
        return 0.0;
    }

    double TotalMass = Design->Mass;

    if (Element->GetLoadouts().size() < 1)
    {
        return TotalMass;
    }

    MissionLoad* Load = Element->GetLoadouts().at(0);
    if (!Load)
    {
        return TotalMass;
    }

    if (Load->GetName().length() > 0)
    {
        const FString SelectedName = ANSI_TO_TCHAR(Load->GetName().data());

        for (const FShipLoadout& ShipLoadout : Design->Loadout)
        {
            if (ShipLoadout.Name.Equals(SelectedName, ESearchCase::IgnoreCase))
            {
                return ComputeLoadoutMass(Design, ShipLoadout);
            }
        }

        return TotalMass;
    }

    const int32* Stations = Load->GetStations();
    if (!Stations)
    {
        return TotalMass;
    }

    const int32 NumStations = Design->Hardpoint.Num();

    for (int32 StationIndex = 0; StationIndex < NumStations; ++StationIndex)
    {
        const int32 PointIndex = Stations[StationIndex];
        if (PointIndex < 0)
        {
            continue;
        }

        const FWeaponDesign* WeaponRow =
            ResolveWeaponDesignForStationSelection(Design, StationIndex, PointIndex);

        if (WeaponRow)
        {
            TotalMass += WeaponRow->CarryMass;
        }
    }

    return TotalMass;
}

double UMissionWeaponDlg::ComputeLoadoutMass(
    const FShipDesign* Design,
    const FShipLoadout& Loadout) const
{
    if (!Design)
    {
        return 0.0;
    }

    double TotalMass = Design->Mass;
    const int32 NumStations = FMath::Min(Design->Hardpoint.Num(), Loadout.Stations.Num());

    for (int32 StationIndex = 0; StationIndex < NumStations; ++StationIndex)
    {
        const int32 PointIndex = Loadout.Stations[StationIndex];
        if (PointIndex < 0)
        {
            continue;
        }

        const FWeaponDesign* WeaponRow =
            ResolveWeaponDesignForStationSelection(Design, StationIndex, PointIndex);

        if (!WeaponRow)
        {
            continue;
        }

        TotalMass += WeaponRow->CarryMass;
    }

    return TotalMass;
}

void UMissionWeaponDlg::RefreshSelectedLoadoutStations()
{
    StationItems.Empty();

    if (StationListView)
    {
        StationListView->ClearListItems();
    }

    Mission* MissionPtr = ResolveMission();
    MissionElement* Elem = ResolvePlayerElement();
    const FShipDesign* Design = ResolvePlayerShipDesign();

    if (!MissionPtr || !Elem || !Design)
    {
        return;
    }

    MissionLoad* Load = GetActivePlayerMissionLoad(MissionPtr);
    if (!Load)
    {
        return;
    }

    const int32 NumHardpoints = Design->Hardpoint.Num();

    for (int32 StationIndex = 0; StationIndex < NumHardpoints; ++StationIndex)
    {
        const int32 PointIndex = Load->GetStation(StationIndex);

        const FWeaponDesign* Weapon =
            ResolveWeaponDesignForStationSelection(
                Design,
                StationIndex,
                PointIndex);

        const FString WeaponName = Weapon ? Weapon->Name : TEXT("Empty");
        const FString StationLabel = FString::Printf(TEXT("STATION %d"), StationIndex + 1);

        UMissionWeaponStationRowObject* Row =
            NewObject<UMissionWeaponStationRowObject>(this);

        const FShipHardPoint& Hardpoint = Design->Hardpoint[StationIndex];

        Row->Init(
            StationIndex,
            StationLabel,
            WeaponName,
            Hardpoint.AllowedWeaponTypes,
            PointIndex
        );

        Row->SetOwningWeaponDlg(this);

        StationItems.Add(Row);

        if (StationListView)
        {
            StationListView->AddItem(Row);
        }
    }
}

void UMissionWeaponDlg::HandleStationChanged(int32 StationIndex, int32 NewSelection)
{
    Mission* MissionPtr = ResolveMission();
    if (!MissionPtr)
    {
        return;
    }

    MissionLoad* Load = GetActivePlayerMissionLoad(MissionPtr);
    if (!Load)
    {
        return;
    }

    Load->SetStation(StationIndex, NewSelection);

    // This is no longer an untouched preset loadout.
    Load->SetName("");

    RefreshSelectedLoadoutStations();

    MissionElement* Elem = ResolvePlayerElement();
    const FShipDesign* Design = ResolvePlayerShipDesign();

    if (WeightValueText && Elem && Design)
    {
        const double CurrentMass = ComputeCurrentCustomMass(Elem, Design);
        WeightValueText->SetText(FText::FromString(FormatWeight(CurrentMass)));
    }
}

UWidget* UMissionWeaponDlg::BuildListHeaderRow(
    const FString& LeftText,
    const FString& RightText,
    float LeftWidth,
    float RightWidth) const
{
    UBorder* HeaderBorder = WidgetTree->ConstructWidget<UBorder>();
    HeaderBorder->SetBrushColor(MissionUIStyle::SSWHeaderBG);

    UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    HeaderBorder->SetContent(HeaderRow);

    USizeBox* LeftBox = WidgetTree->ConstructWidget<USizeBox>();
    LeftBox->SetWidthOverride(LeftWidth);

    UTextBlock* LeftLabel = WidgetTree->ConstructWidget<UTextBlock>();
    LeftLabel->SetText(FText::FromString(LeftText));
    LeftLabel->SetColorAndOpacity(FLinearColor::White);
    LeftLabel->SetFont(MissionUIStyle::GetRowFont(15));
    LeftLabel->SetJustification(ETextJustify::Left);
    LeftBox->SetContent(LeftLabel);

    if (UHorizontalBoxSlot* LeftSlot = HeaderRow->AddChildToHorizontalBox(LeftBox))
    {
        LeftSlot->SetPadding(FMargin(8.f, 4.f, 12.f, 4.f));
        LeftSlot->SetHorizontalAlignment(HAlign_Left);
        LeftSlot->SetVerticalAlignment(VAlign_Center);
        LeftSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    USizeBox* RightBox = WidgetTree->ConstructWidget<USizeBox>();
    RightBox->SetWidthOverride(RightWidth);

    UTextBlock* RightLabel = WidgetTree->ConstructWidget<UTextBlock>();
    RightLabel->SetText(FText::FromString(RightText));
    RightLabel->SetColorAndOpacity(FLinearColor::White);
    RightLabel->SetFont(MissionUIStyle::GetRowFont(15));
    RightLabel->SetJustification(ETextJustify::Right);
    RightBox->SetContent(RightLabel);

    if (UHorizontalBoxSlot* RightSlot = HeaderRow->AddChildToHorizontalBox(RightBox))
    {
        RightSlot->SetPadding(FMargin(8.f, 4.f, 8.f, 4.f));
        RightSlot->SetHorizontalAlignment(HAlign_Left);
        RightSlot->SetVerticalAlignment(VAlign_Center);
        RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    return HeaderBorder;
}