/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.
*/

#include "MissionPackageDlg.h"

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
#include "MissionElement.h"
#include "Instruction.h"
#include "GameStructs_System.h"

#include "MissionPackageListObject.h"
#include "MissionNavListObject.h"

UMissionPackageDlg::UMissionPackageDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UMissionPackageDlg::NativeConstruct()
{
    Super::NativeConstruct();

    bHeadersBuiltFromGeometry = false;

    if (PanelSizeBox)
    {
        PanelSizeBox->SetWidthOverride(1490.f);
        PanelSizeBox->SetHeightOverride(685.f);

        if (UCanvasPanelSlot* PanelCanvasSlot = Cast<UCanvasPanelSlot>(PanelSizeBox->Slot))
        {
            PanelCanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
            PanelCanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
            PanelCanvasSlot->SetPosition(FVector2D(0.f, 0.f));
            PanelCanvasSlot->SetSize(FVector2D(1490.f, 685.f));
        }
    }

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
    PackageIndex = INDEX_NONE;

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

        if (Elem->GetIFF() == MissionPtr->GetTeam() &&
            !Elem->IsSquadron() &&
            Elem->GetRegion() == MissionPtr->GetRegion() &&
            Design->ShipType < (int32)CLASSIFICATION::STATION)
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

        if (Elem->GetIFF() == MissionPtr->GetTeam() &&
            !Elem->IsSquadron() &&
            Elem->GetRegion() == MissionPtr->GetRegion() &&
            Design->ShipType < (int32)CLASSIFICATION::STATION)
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

    if (PackageIndex == INDEX_NONE && PackageItems.Num() > 0)
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

        const double Dist = FVector::Dist(Loc, Nav->Location());

        UMissionNavListObject* Item = NewObject<UMissionNavListObject>(this);
        if (Item)
        {
            Item->InitFromInstruction(Nav, NavIndex, Dist);
            NavItems.Add(Item);
            NavList->AddItem(Item);
        }

        Loc = Nav->Location();
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
            Elem->IntelLevel() <= Intel::SECRET)
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