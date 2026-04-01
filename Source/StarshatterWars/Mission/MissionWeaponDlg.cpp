#include "MissionWeaponDlg.h"

#include "MissionBriefingDlg.h"
#include "MissionPlanner.h"
#include "MissionWeaponLoadoutListObject.h"
#include "MissionLoadoutListView.h"

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

#include "Mission.h"
#include "MissionElement.h"

UMissionWeaponDlg::UMissionWeaponDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UMissionWeaponDlg::NativeConstruct()
{
    Super::NativeConstruct();

    BuildRuntimeLayout();
    RefreshFromMission();
}

void UMissionWeaponDlg::SetParentDlg(UMissionBriefingDlg* InParentDlg)
{
    ParentDlg = InParentDlg;
}

Mission* UMissionWeaponDlg::ResolveMission() const
{
    return ParentDlg ? ParentDlg->GetMissionPtr() : nullptr;
}

MissionElement* UMissionWeaponDlg::ResolvePlayerElement() const
{
    Mission* M = ResolveMission();
    if (!M) return nullptr;

    ListIter<MissionElement> It = M->GetElements();
    while (++It)
    {
        MissionElement* E = It.value();
        if (E && E->IsPlayer())
            return E;
    }

    return M->GetPlayer();
}

const FShipDesign* UMissionWeaponDlg::ResolvePlayerShipDesign() const
{
    MissionElement* E = ResolvePlayerElement();
    return E ? E->GetShipDesign() : nullptr;
}

void UMissionWeaponDlg::RefreshFromMission()
{
    ClearLoadouts();

    MissionElement* Elem = ResolvePlayerElement();
    const FShipDesign* Design = ResolvePlayerShipDesign();

    if (!Elem || !Design)
        return;

    if (ElementNameValueText)
        ElementNameValueText->SetText(FText::FromString(GetElementName(Elem)));

    if (DesignNameValueText)
        DesignNameValueText->SetText(FText::FromString(GetDesignName(Design)));

    if (WeightValueText)
    {
        const double CurrentMass = ComputeCurrentCustomMass(Elem, Design);
        WeightValueText->SetText(FText::FromString(FormatWeight(CurrentMass)));
    }

    BuildLoadouts(Elem, Design);
}

void UMissionWeaponDlg::ClearLoadouts()
{
    Items.Empty();

    if (WeaponListView)
    {
        WeaponListView->ClearListItems();
    }
}

bool UMissionWeaponDlg::GetSelectedLoadoutName(MissionElement* Element, FString& OutName) const
{
    if (!Element || Element->Loadouts().size() == 0)
        return false;

    MissionLoad* Load = Element->Loadouts().at(0);
    if (!Load) return false;

    if (Load->GetName().length() > 0)
    {
        OutName = ANSI_TO_TCHAR(Load->GetName().data());
        return true;
    }

    return false;
}

void UMissionWeaponDlg::BuildLoadouts(MissionElement* Element, const FShipDesign* Design)
{
    FString SelectedName;
    const bool bHasSelected = GetSelectedLoadoutName(Element, SelectedName);

    for (int32 i = 0; i < Design->Loadout.Num(); i++)
    {
        const FShipLoadout& L = Design->Loadout[i];

        bool bSelected =
            bHasSelected &&
            L.Name.Equals(SelectedName, ESearchCase::IgnoreCase);

        const double LoadoutMass = ComputeLoadoutMass(Design, L);
        FString WeightStr = FormatWeight(LoadoutMass);

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
}

FString UMissionWeaponDlg::GetElementName(MissionElement* E) const
{
    return E ? ANSI_TO_TCHAR(E->GetName().data()) : TEXT("UNKNOWN");
}

FString UMissionWeaponDlg::GetDesignName(const FShipDesign* D) const
{
    if (!D) return TEXT("UNKNOWN");

    if (!D->DisplayName.IsEmpty())
        return D->DisplayName;

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

    if (WeaponListView)
    {
        return;
    }

    RuntimeHost->SetContent(nullptr);

    USizeBox* Root =
        WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MissionWeaponRoot"));
    Root->SetWidthOverride(1400.f);
    Root->SetHeightOverride(500.f);
    RuntimeHost->SetContent(Root);

    UHorizontalBox* MainRow =
        WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MissionWeaponRootRow"));
    Root->SetContent(MainRow);

    // LEFT COLUMN
    UVerticalBox* Left =
        WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MissionWeaponLeftColumn"));

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

    // RIGHT COLUMN
    UVerticalBox* Right =
        WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MissionWeaponRightColumn"));

    if (UHorizontalBoxSlot* RightSlot = MainRow->AddChildToHorizontalBox(Right))
    {
        RightSlot->SetPadding(FMargin(0.f, 12.f, 12.f, 12.f));
        RightSlot->SetHorizontalAlignment(HAlign_Fill);
        RightSlot->SetVerticalAlignment(VAlign_Fill);
        RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    if (UBorder* Header = BuildHeader(TEXT("STANDARD LOADOUTS")))
    {
        if (UVerticalBoxSlot* HeaderSlot = Right->AddChildToVerticalBox(Header))
        {
            HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
            HeaderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        }
    }

    UBorder* ListBorder =
        WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MissionWeaponListBorder"));
    ListBorder->SetBrushColor(MissionUIStyle::PanelBG);

    if (UVerticalBoxSlot* BorderSlot = Right->AddChildToVerticalBox(ListBorder))
    {
        BorderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        BorderSlot->SetPadding(FMargin(0.f));
    }

    USizeBox* ListHost =
        WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MissionWeaponListHost"));
    ListHost->SetWidthOverride(900.f);
    ListHost->SetHeightOverride(320.f);
    ListBorder->SetContent(ListHost);

    WeaponListView = WidgetTree->ConstructWidget<UMissionLoadoutListView>(
        UMissionLoadoutListView::StaticClass(),
        TEXT("RuntimeWeaponList"));

    if (EntryWidgetClass)
    {
        WeaponListView->SetEntryWidgetClassPublic(EntryWidgetClass);
    }

    ListHost->SetContent(WeaponListView);
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

    // 1. Direct row-name lookup
    if (const FWeaponDesign* WeaponRow = WeaponDesignRegistry::Find(WeaponKey))
    {
        return WeaponRow;
    }

    // 2. Fallback by Name / Group
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

    if (Element->Loadouts().size() < 1)
    {
        return TotalMass;
    }

    MissionLoad* Load = Element->Loadouts().at(0);
    if (!Load)
    {
        return TotalMass;
    }

    // Named loadout path:
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

    // Custom station selection path:
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

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionWeaponDlg] Runtime '%s' hardpoints=%d"),
        *Design->ShipName,
        Design->Hardpoint.Num());

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionWeaponDlg] Design hardpoints=%d loadout stations=%d using=%d for '%s'"),
        Design->Hardpoint.Num(),
        Loadout.Stations.Num(),
        NumStations,
        *Loadout.Name);

    for (int32 i = 0; i < Loadout.Stations.Num(); ++i)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MissionWeaponDlg]   Loadout '%s' raw stations[%d]=%d"),
            *Loadout.Name,
            i,
            Loadout.Stations[i]);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionWeaponDlg] Loadout '%s' base mass = %f"),
        *Loadout.Name,
        Design->Mass);

    for (int32 StationIndex = 0; StationIndex < NumStations; ++StationIndex)
    {
        const int32 PointIndex = Loadout.Stations[StationIndex];
        if (PointIndex < 0)
        {
            continue;
        }

        const FString WeaponKey =
            Design->Hardpoint.IsValidIndex(StationIndex) &&
            Design->Hardpoint[StationIndex].AllowedWeaponTypes.IsValidIndex(PointIndex)
            ? Design->Hardpoint[StationIndex].AllowedWeaponTypes[PointIndex]
            : TEXT("INVALID");

        const FWeaponDesign* WeaponRow =
            ResolveWeaponDesignForStationSelection(Design, StationIndex, PointIndex);

        if (!WeaponRow)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[MissionWeaponDlg] station=%d point=%d key='%s' -> NO WEAPON ROW"),
                StationIndex,
                PointIndex,
                *WeaponKey);
            continue;
        }

        UE_LOG(LogTemp, Warning,
            TEXT("[MissionWeaponDlg] station=%d point=%d key='%s' weapon='%s' carry=%f"),
            StationIndex,
            PointIndex,
            *WeaponKey,
            *WeaponRow->Name,
            WeaponRow->CarryMass);

        TotalMass += WeaponRow->CarryMass;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[MissionWeaponDlg] Loadout '%s' total mass = %f"),
        *Loadout.Name,
        TotalMass);

    return TotalMass;
}