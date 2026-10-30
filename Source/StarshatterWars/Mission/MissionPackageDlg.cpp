/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.
*/

#include "MissionPackageDlg.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "MissionUIStyle.h"


#include "MissionListLayout.h"

#include "Blueprint/WidgetTree.h"

#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ListView.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanelSlot.h"

#include "MissionBriefingDlg.h"
#include "MissionPlanner.h"

#include "Mission.h"
#include "Campaign.h"
#include "MissionElement.h"
#include "Instruction.h"
#include "GameStructs_System.h"

#include "MissionPackageListObject.h"
#include "MissionNavListObject.h"

// PKG uses the same eligibility test for list construction and selected-row
// resolution. Keep dynamic campaigns' filtering intact; scripted Campaign 1
// may omit team and per-element region in its .def mission data.
static bool IsVisiblePackageElement(const Mission* MissionPtr, const MissionElement* Elem,
    const FShipDesign* Design)
{
    if (!MissionPtr || !Elem || !Design)
        return false;

    const Campaign* ActiveCampaign = Campaign::GetCampaign();
    const bool bTrainingCampaign = ActiveCampaign && ActiveCampaign->GetCampaignId() == 1;

    int32 FriendlyIFF = MissionPtr->GetTeam();
    if (bTrainingCampaign && FriendlyIFF == 0)
    {
        // In T1.def the mission-level team is omitted (defaults to zero),
        // but the player flight and its allies have iff: 1.
        if (MissionElement* PlayerElem = const_cast<Mission*>(MissionPtr)->GetPlayer())
            FriendlyIFF = PlayerElem->GetIFF();
    }

    const FString MissionRegion = FString(ANSI_TO_TCHAR(MissionPtr->GetRegion())).TrimStartAndEnd();
    const FString ElementRegion = FString(ANSI_TO_TCHAR(Elem->GetRegion())).TrimStartAndEnd();
    const bool bRegionMatches = ElementRegion.Equals(MissionRegion, ESearchCase::IgnoreCase) ||
        (bTrainingCampaign && ElementRegion.IsEmpty() && !MissionRegion.IsEmpty());

    // IsSquadron() excludes carrier-based hangar inventory; preserve that
    // original rule for every campaign. Deployed flights such as Delta remain.
    return Elem->GetIFF() == FriendlyIFF &&
        !Elem->IsSquadron() &&
        bRegionMatches &&
        Design->ShipType < static_cast<int32>(CLASSIFICATION::STATION);
}

UMissionPackageDlg::UMissionPackageDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}


TSharedRef<SWidget> UMissionPackageDlg::RebuildWidget()
{
    BuildPackageLayout();
    return Super::RebuildWidget();
}

void UMissionPackageDlg::BuildPackageLayout()
{
    // The Blueprint supplies list entry classes; retain its list instances.
    if (!WidgetTree || PresentationRoot || !PackageList || !NavList) return;
    if (!PackageHeaderRow) PackageHeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    if (!NavHeaderRow) NavHeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    for (UWidget* Widget : { static_cast<UWidget*>(PackageList), static_cast<UWidget*>(NavList),
        static_cast<UWidget*>(PackageHeaderRow), static_cast<UWidget*>(NavHeaderRow) })
        Widget->RemoveFromParent();
    PresentationRoot = WidgetTree->ConstructWidget<UBorder>();
    PresentationRoot->SetBrushColor(FLinearColor(0.045f, 0.055f, 0.065f, 0.96f));
    PresentationRoot->SetPadding(FMargin(16.f));
    WidgetTree->RootWidget = PresentationRoot;
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
    PresentationRoot->SetContent(Row);
    UVerticalBox* Tables = WidgetTree->ConstructWidget<UVerticalBox>();
    UHorizontalBoxSlot* TableSlot = Row->AddChildToHorizontalBox(Tables);
    FSlateChildSize TableSize(ESlateSizeRule::Fill); TableSize.Value = 3.f;
    TableSlot->SetSize(TableSize);
    TableSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
    auto Heading = [this](const FText& HeadingText)->UBorder*
    {
        UBorder* Bar = WidgetTree->ConstructWidget<UBorder>();
        Bar->SetBrushColor(FLinearColor(0.105f, 0.113f, 0.125f, 1.f));
        Bar->SetPadding(FMargin(12.f, 8.f));
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(HeadingText);
        Label->SetFont(MissionUIStyle::GetHeaderFont(22));
        Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.97f, 1.f)));
        Bar->SetContent(Label);
        return Bar;
    };
    Tables->AddChildToVerticalBox(Heading(NSLOCTEXT("MissionBriefing", "PackageRoster", "MISSION PACKAGE")));
    Tables->AddChildToVerticalBox(PackageHeaderRow);
    PackageTableSizebox = WidgetTree->ConstructWidget<USizeBox>();
    PackageTableSizebox->SetContent(PackageList);
    Tables->AddChildToVerticalBox(PackageTableSizebox)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Tables->AddChildToVerticalBox(Heading(NSLOCTEXT("MissionBriefing", "PackageRoute", "SELECTED FLIGHT ROUTE")))->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
    Tables->AddChildToVerticalBox(NavHeaderRow);
    Tables->AddChildToVerticalBox(NavList)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UVerticalBox* Summary = WidgetTree->ConstructWidget<UVerticalBox>();
    Row->AddChildToHorizontalBox(Summary)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Summary->AddChildToVerticalBox(Heading(NSLOCTEXT("MissionBriefing", "PackageThreats", "KNOWN THREATS")));
    UScrollBox* ThreatScroll = WidgetTree->ConstructWidget<UScrollBox>();
    ThreatScroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
    Summary->AddChildToVerticalBox(ThreatScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    for (UTextBlock** Threat : { &Threat0, &Threat1, &Threat2, &Threat3, &Threat4 })
    {
        if (!*Threat) *Threat = WidgetTree->ConstructWidget<UTextBlock>();
        (*Threat)->RemoveFromParent();
        (*Threat)->SetFont(MissionUIStyle::GetInfoValueFont(18));
        (*Threat)->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.93f, 0.98f)));
        (*Threat)->SetAutoWrapText(true);
        ThreatScroll->AddChild(*Threat);
        if (UScrollBoxSlot* Cell = Cast<UScrollBoxSlot>((*Threat)->Slot)) Cell->SetPadding(FMargin(12.f, 8.f));
    }
    if (MissionRosterLabel)
    {
        MissionRosterLabel->RemoveFromParent();
        MissionRosterLabel->SetFont(MissionUIStyle::GetInfoValueFont(16));
        MissionRosterLabel->SetAutoWrapText(true);
        Summary->AddChildToVerticalBox(MissionRosterLabel);
    }
}

void UMissionPackageDlg::NativeConstruct()
{
    Super::NativeConstruct();

    bHeadersBuiltFromGeometry = false;

    if (PackageList)
    {
        PackageList->OnItemSelectionChanged().Clear();
        PackageList->OnItemSelectionChanged().AddUObject(this, &UMissionPackageDlg::OnPackageSelectionChanged);
    }

    // First pass build. This may use fallback widths.
    BuildHeaders();
}

void UMissionPackageDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bHeadersBuiltFromGeometry)
    {
        return;
    }

    const float PackageWidth = GetPackageHeaderClampWidth();
    const float NavWidth = GetNavHeaderClampWidth();

    if (PackageWidth > 1.f && NavWidth > 1.f)
    {
        BuildHeaders();
        bHeadersBuiltFromGeometry = true;
    }
}

void UMissionPackageDlg::SetParentDlg(UMissionBriefingDlg* InParentDlg)
{
    ParentDlg = InParentDlg;
}

Mission* UMissionPackageDlg::ResolveMission() const
{
    return ParentDlg ? ParentDlg->GetMissionPtr() : nullptr;
}

void UMissionPackageDlg::RefreshFromMission()
{
    Mission* CurrentMission = ResolveMission();
    const int32 CurrentId = CurrentMission ? CurrentMission->GetIdentity() : INDEX_NONE;
    if (CurrentMission != LastPackageMission || CurrentId != LastPackageMissionId)
        PackageIndex = INDEX_NONE;
    LastPackageMission = CurrentMission;
    LastPackageMissionId = CurrentId;

    DrawPackages();
    DrawNavPlan();
    DrawThreats();

    // Layout may change after content refresh, so allow one rebuild.
    bHeadersBuiltFromGeometry = false;
}

MissionElement* UMissionPackageDlg::ResolveSelectedPackageElement() const
{
    Mission* MissionPtr = ResolveMission();
    if (!MissionPtr)
    {
        return nullptr;
    }

    int32 VisibleIndex = 0;

    ListIter<MissionElement> Iter = MissionPtr->GetElements();
    while (++Iter)
    {
        MissionElement* Elem = Iter.value();
        if (!Elem)
        {
            continue;
        }

        const FShipDesign* Design = Elem->GetShipDesign();
        if (!Design)
        {
            continue;
        }

        if (IsVisiblePackageElement(MissionPtr, Elem, Design))
        {
            if (VisibleIndex == PackageIndex)
            {
                return Elem;
            }

            ++VisibleIndex;
        }
    }

    return nullptr;
}

void UMissionPackageDlg::DrawPackages()
{
    Mission* MissionPtr = ResolveMission();

    PackageItems.Empty();

    if (PackageList)
    {
        PackageList->ClearListItems();
    }

    if (!MissionPtr || !PackageList)
    {
        return;
    }

    int32 VisibleIndex = 0;

    ListIter<MissionElement> Iter = MissionPtr->GetElements();
    while (++Iter)
    {
        MissionElement* Elem = Iter.value();
        if (!Elem)
        {
            continue;
        }

        const FShipDesign* Design = Elem->GetShipDesign();
        if (!Design)
        {
            continue;
        }

        if (IsVisiblePackageElement(MissionPtr, Elem, Design))
        {
            UMissionPackageListObject* Item = NewObject<UMissionPackageListObject>(this);
            if (!Item)
            {
                continue;
            }

            Item->InitFromMissionElement(Elem, VisibleIndex, Elem->IsPlayer());

            PackageItems.Add(Item);
            PackageList->AddItem(Item);

            if (Elem->IsPlayer() && PackageIndex == INDEX_NONE)
            {
                PackageIndex = VisibleIndex;
            }

            ++VisibleIndex;
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("[MissionPackageDlg] Mission=%d PackageRows=%d Team=%d Region='%hs'"),
        MissionPtr->GetIdentity(), PackageItems.Num(), MissionPtr->GetTeam(), MissionPtr->GetRegion());

    if (!PackageItems.IsValidIndex(PackageIndex) && PackageItems.Num() > 0)
    {
        PackageIndex = 0;
    }

    if (PackageItems.IsValidIndex(PackageIndex))
    {
        PackageList->SetSelectedItem(PackageItems[PackageIndex]);
    }
}

void UMissionPackageDlg::DrawNavPlan()
{
    Mission* MissionPtr = ResolveMission();

    NavItems.Empty();

    if (NavList)
    {
        NavList->ClearListItems();
    }

    if (!MissionPtr || !NavList)
    {
        return;
    }

    MissionElement* Element = ResolveSelectedPackageElement();
    if (!Element)
    {
        return;
    }

    FVector Loc = Element->GetLocation();
    int32 NavIndex = 0;

    ListIter<Instruction> NavPt = Element->NavList();
    while (++NavPt)
    {
        Instruction* Nav = NavPt.value();
        if (!Nav)
        {
            continue;
        }

        const double Dist = FVector::Dist(Loc, Nav->GetLocation());

        UMissionNavListObject* Item = NewObject<UMissionNavListObject>(this);
        if (Item)
        {
            Item->InitFromInstruction(Nav, NavIndex, Dist);
            NavItems.Add(Item);
            NavList->AddItem(Item);
        }

        Loc = Nav->GetLocation();
        ++NavIndex;
    }
}

void UMissionPackageDlg::DrawThreats()
{
    auto SetThreat = [](UTextBlock* Block, const FString& Value)
        {
            if (Block)
            {
                Block->SetText(FText::FromString(Value));
            }
        };

    SetThreat(Threat0, TEXT(""));
    SetThreat(Threat1, TEXT(""));
    SetThreat(Threat2, TEXT(""));
    SetThreat(Threat3, TEXT(""));
    SetThreat(Threat4, TEXT(""));

    Mission* MissionPtr = ResolveMission();
    if (!MissionPtr)
    {
        return;
    }

    MissionElement* Player = MissionPtr->GetPlayer();
    if (!Player)
    {
        return;
    }

    FVector BaseLoc = Player->GetLocation();
    int32 ThreatIndex = 0;

    ListIter<MissionElement> Iter = MissionPtr->GetElements();
    while (++Iter)
    {
        MissionElement* Elem = Iter.value();
        if (!Elem)
        {
            continue;
        }

        if (Elem->GetIFF() == 0 ||
            Elem->GetIFF() == Player->GetIFF() ||
            Elem->GetIntelLevel() <= EIntel::SECRET)
        {
            continue;
        }

        const FShipDesign* Design = Elem->GetShipDesign();
        if (!Design)
        {
            continue;
        }

        const double Dist = FVector::Dist(BaseLoc, Elem->GetLocation());

        FString Text = FString::Printf(
            TEXT("%d %s - %.0f"),
            Elem->Count(),
            Design->Abrv.IsEmpty() ? TEXT("UNK") : *Design->Abrv,
            Dist);

        if (ThreatIndex == 0) SetThreat(Threat0, Text);
        if (ThreatIndex == 1) SetThreat(Threat1, Text);
        if (ThreatIndex == 2) SetThreat(Threat2, Text);
        if (ThreatIndex == 3) SetThreat(Threat3, Text);
        if (ThreatIndex == 4) SetThreat(Threat4, Text);

        ++ThreatIndex;
        if (ThreatIndex >= 5)
        {
            break;
        }
    }
}

void UMissionPackageDlg::BuildHeaders()
{
    BuildPackageHeaderRow();
    BuildNavHeaderRow();
}

float UMissionPackageDlg::GetPackageHeaderClampWidth() const
{
    // Best source: the package table size box if it has a width override.
    if (PackageTableSizebox)
    {
        const float WidthOverride = PackageTableSizebox->GetWidthOverride();
        if (WidthOverride > 1.f)
        {
            return WidthOverride;
        }

        const float CachedWidth = PackageTableSizebox->GetCachedGeometry().GetLocalSize().X;
        if (CachedWidth > 1.f)
        {
            return CachedWidth;
        }
    }

    // Next best source: list geometry.
    if (PackageList)
    {
        const float CachedWidth = PackageList->GetCachedGeometry().GetLocalSize().X;
        if (CachedWidth > 1.f)
        {
            return CachedWidth;
        }
    }

    // Fallback: column sum.
    return
        MissionListLayout::PackageMarkerCol +
        MissionListLayout::PackageNameCol +
        MissionListLayout::PackageRoleCol +
        MissionListLayout::PackageTextCol;
}

float UMissionPackageDlg::GetNavHeaderClampWidth() const
{
    if (NavList)
    {
        const float CachedWidth = NavList->GetCachedGeometry().GetLocalSize().X;
        if (CachedWidth > 1.f)
        {
            return CachedWidth;
        }
    }

    return
        MissionListLayout::NavCol1 +
        MissionListLayout::NavCol2 +
        MissionListLayout::NavCol3 +
        MissionListLayout::NavCol4 +
        MissionListLayout::NavCol5;
}

void UMissionPackageDlg::BuildPackageHeaderRow()
{
    if (!PackageHeaderRow || !WidgetTree)
    {
        return;
    }

    PackageHeaderRow->ClearChildren();

    const float ClampWidth = GetPackageHeaderClampWidth();

    USizeBox* ClampedRowBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    ClampedRowBox->SetWidthOverride(ClampWidth);
    ClampedRowBox->SetMinDesiredHeight(MissionListLayout::RowHeight);

    UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    RowBorder->SetBrushColor(FLinearColor(0.20f, 0.30f, 0.45f, 1.0f));
    RowBorder->SetPadding(FMargin(0.f));

    UHorizontalBox* RowContent = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    if (UHorizontalBoxSlot* HeaderCellSlot = RowContent->AddChildToHorizontalBox(
        MakeHeaderCell(TEXT("PKG"), MissionListLayout::PackageMarkerCol)))
    {
        HeaderCellSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HeaderCellSlot->SetHorizontalAlignment(HAlign_Left);
        HeaderCellSlot->SetVerticalAlignment(VAlign_Center);
    }

    if (UHorizontalBoxSlot* HeaderCellSlot = RowContent->AddChildToHorizontalBox(
        MakeHeaderCell(TEXT("CALLSIGN"), MissionListLayout::PackageNameCol)))
    {
        HeaderCellSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HeaderCellSlot->SetHorizontalAlignment(HAlign_Left);
        HeaderCellSlot->SetVerticalAlignment(VAlign_Center);
    }

    if (UHorizontalBoxSlot* HeaderCellSlot = RowContent->AddChildToHorizontalBox(
        MakeHeaderCell(TEXT("ROLE"), MissionListLayout::PackageRoleCol)))
    {
        HeaderCellSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HeaderCellSlot->SetHorizontalAlignment(HAlign_Left);
        HeaderCellSlot->SetVerticalAlignment(VAlign_Center);
    }

    if (UHorizontalBoxSlot* HeaderCellSlot = RowContent->AddChildToHorizontalBox(
        MakeHeaderCell(TEXT("TYPE"), MissionListLayout::PackageTextCol)))
    {
        HeaderCellSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HeaderCellSlot->SetHorizontalAlignment(HAlign_Left);
        HeaderCellSlot->SetVerticalAlignment(VAlign_Center);
    }

    RowBorder->SetContent(RowContent);
    ClampedRowBox->AddChild(RowBorder);

    if (UHorizontalBoxSlot* HeaderRowSlot = PackageHeaderRow->AddChildToHorizontalBox(ClampedRowBox))
    {
        HeaderRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HeaderRowSlot->SetHorizontalAlignment(HAlign_Left);
        HeaderRowSlot->SetVerticalAlignment(VAlign_Center);
        HeaderRowSlot->SetPadding(FMargin(0.f));
    }
}

void UMissionPackageDlg::BuildNavHeaderRow()
{
    if (!NavHeaderRow || !WidgetTree)
    {
        return;
    }

    NavHeaderRow->ClearChildren();

    const float ClampWidth = GetNavHeaderClampWidth();

    USizeBox* ClampedRowBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    ClampedRowBox->SetWidthOverride(ClampWidth);
    ClampedRowBox->SetMinDesiredHeight(MissionListLayout::RowHeight);

    UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    RowBorder->SetBrushColor(FLinearColor(0.20f, 0.30f, 0.45f, 1.0f));
    RowBorder->SetPadding(FMargin(0.f));

    UHorizontalBox* RowContent = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    if (UHorizontalBoxSlot* HeaderCellSlot = RowContent->AddChildToHorizontalBox(
        MakeHeaderCell(TEXT("NO."), MissionListLayout::NavCol1)))
    {
        HeaderCellSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HeaderCellSlot->SetHorizontalAlignment(HAlign_Left);
        HeaderCellSlot->SetVerticalAlignment(VAlign_Center);
    }

    if (UHorizontalBoxSlot* HeaderCellSlot = RowContent->AddChildToHorizontalBox(
        MakeHeaderCell(TEXT("ACTION"), MissionListLayout::NavCol2)))
    {
        HeaderCellSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HeaderCellSlot->SetHorizontalAlignment(HAlign_Left);
        HeaderCellSlot->SetVerticalAlignment(VAlign_Center);
    }

    if (UHorizontalBoxSlot* HeaderCellSlot = RowContent->AddChildToHorizontalBox(
        MakeHeaderCell(TEXT("SECTOR"), MissionListLayout::NavCol3)))
    {
        HeaderCellSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HeaderCellSlot->SetHorizontalAlignment(HAlign_Left);
        HeaderCellSlot->SetVerticalAlignment(VAlign_Center);
    }

    if (UHorizontalBoxSlot* HeaderCellSlot = RowContent->AddChildToHorizontalBox(
        MakeHeaderCell(TEXT("DIST"), MissionListLayout::NavCol4)))
    {
        HeaderCellSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HeaderCellSlot->SetHorizontalAlignment(HAlign_Left);
        HeaderCellSlot->SetVerticalAlignment(VAlign_Center);
    }

    if (UHorizontalBoxSlot* HeaderCellSlot = RowContent->AddChildToHorizontalBox(
        MakeHeaderCell(TEXT("SPEED"), MissionListLayout::NavCol5)))
    {
        HeaderCellSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HeaderCellSlot->SetHorizontalAlignment(HAlign_Left);
        HeaderCellSlot->SetVerticalAlignment(VAlign_Center);
    }

    RowBorder->SetContent(RowContent);
    ClampedRowBox->AddChild(RowBorder);

    if (UHorizontalBoxSlot* HeaderRowSlot = NavHeaderRow->AddChildToHorizontalBox(ClampedRowBox))
    {
        HeaderRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HeaderRowSlot->SetHorizontalAlignment(HAlign_Left);
        HeaderRowSlot->SetVerticalAlignment(VAlign_Center);
        HeaderRowSlot->SetPadding(FMargin(0.f));
    }
}

UWidget* UMissionPackageDlg::MakeHeaderCell(const FString& Text, float Width) const
{
    if (!WidgetTree)
    {
        return nullptr;
    }

    USizeBox* CellBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    CellBox->SetWidthOverride(Width);
    CellBox->SetMinDesiredHeight(MissionListLayout::RowHeight);

    UBorder* CellBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    CellBorder->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));
    CellBorder->SetHorizontalAlignment(HAlign_Left);
    CellBorder->SetVerticalAlignment(VAlign_Center);
    CellBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.f));

    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Label->SetText(FText::FromString(Text));
    Label->SetJustification(ETextJustify::Left);
    Label->SetAutoWrapText(false);

    if (MissionRosterLabel)
    {
        FSlateFontInfo FontInfo = MissionRosterLabel->GetFont();
        FontInfo.Size = 16;
        Label->SetFont(FontInfo);
        Label->SetColorAndOpacity(MissionRosterLabel->GetColorAndOpacity());
        Label->SetShadowOffset(MissionRosterLabel->GetShadowOffset());
        Label->SetShadowColorAndOpacity(MissionRosterLabel->GetShadowColorAndOpacity());
    }

    CellBorder->AddChild(Label);
    CellBox->AddChild(CellBorder);

    return CellBox;
}

void UMissionPackageDlg::OnPackageSelectionChanged(UObject* Item)
{
    UMissionPackageListObject* PackageItem = Cast<UMissionPackageListObject>(Item);
    if (!PackageItem)
    {
        return;
    }

    PackageIndex = PackageItem->GetIndex();
    DrawNavPlan();

    // Nav rows may affect visible nav width after refresh.
    bHeadersBuiltFromGeometry = false;
}