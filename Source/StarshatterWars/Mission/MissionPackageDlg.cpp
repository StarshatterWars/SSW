/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.
*/

#include "MissionPackageDlg.h"

#include "MissionListLayout.h"

#include "Blueprint/WidgetTree.h"
#include "Fonts/SlateFontInfo.h"

#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"

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

    if (PanelSizeBox)
    {
        PanelSizeBox->SetWidthOverride(1490.f);
        PanelSizeBox->SetHeightOverride(685.f);

        if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(PanelSizeBox->Slot))
        {
            PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
            PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
            PanelSlot->SetPosition(FVector2D(0.f, 0.f));
            PanelSlot->SetSize(FVector2D(1490.0f, 685.0f));
        }
    }

    BuildHeaders();

    if (PackageList)
    {
        PackageList->OnItemSelectionChanged().Clear();
        PackageList->OnItemSelectionChanged().AddUObject(this, &UMissionPackageDlg::OnPackageSelectionChanged);
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

void UMissionPackageDlg::BuildPackageHeaderRow()
{
    if (!PackageHeaderRow || !WidgetTree)
    {
        return;
    }

    PackageHeaderRow->ClearChildren();

    if (UHorizontalBoxSlot* HeaderSlot = PackageHeaderRow->AddChildToHorizontalBox(
        MakeHeaderCell(TEXT("PKG"), MissionListLayout::PackageMarkerCol)))
    {
        HeaderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HeaderSlot->SetHorizontalAlignment(HAlign_Left);
        HeaderSlot->SetVerticalAlignment(VAlign_Center);
    }

    if (UHorizontalBoxSlot* HeaderSlot = PackageHeaderRow->AddChildToHorizontalBox(
        MakeHeaderCell(TEXT("CALLSIGN"), MissionListLayout::PackageNameCol)))
    {
        HeaderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HeaderSlot->SetHorizontalAlignment(HAlign_Left);
        HeaderSlot->SetVerticalAlignment(VAlign_Center);
    }

    if (UHorizontalBoxSlot* HeaderSlot = PackageHeaderRow->AddChildToHorizontalBox(
        MakeHeaderCell(TEXT("ROLE"), MissionListLayout::PackageRoleCol)))
    {
        HeaderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HeaderSlot->SetHorizontalAlignment(HAlign_Left);
        HeaderSlot->SetVerticalAlignment(VAlign_Center);
    }

    if (UHorizontalBoxSlot* HeaderSlot = PackageHeaderRow->AddChildToHorizontalBox(
        MakeHeaderCell(TEXT("TYPE"), MissionListLayout::PackageTextCol)))
    {
        HeaderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HeaderSlot->SetHorizontalAlignment(HAlign_Left);
        HeaderSlot->SetVerticalAlignment(VAlign_Center);
    }
}

void UMissionPackageDlg::BuildNavHeaderRow()
{
    if (!NavHeaderRow || !WidgetTree)
    {
        return;
    }

    NavHeaderRow->ClearChildren();

    if (UHorizontalBoxSlot* HeaderSlot = NavHeaderRow->AddChildToHorizontalBox(
        MakeHeaderCell(TEXT("NO."), MissionListLayout::NavCol1)))
    {
        HeaderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HeaderSlot->SetHorizontalAlignment(HAlign_Left);
        HeaderSlot->SetVerticalAlignment(VAlign_Center);
    }

    if (UHorizontalBoxSlot* HeaderSlot = NavHeaderRow->AddChildToHorizontalBox(
        MakeHeaderCell(TEXT("ACTION"), MissionListLayout::NavCol2)))
    {
        HeaderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HeaderSlot->SetHorizontalAlignment(HAlign_Left);
        HeaderSlot->SetVerticalAlignment(VAlign_Center);
    }

    if (UHorizontalBoxSlot* HeaderSlot = NavHeaderRow->AddChildToHorizontalBox(
        MakeHeaderCell(TEXT("SECTOR"), MissionListLayout::NavCol3)))
    {
        HeaderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HeaderSlot->SetHorizontalAlignment(HAlign_Left);
        HeaderSlot->SetVerticalAlignment(VAlign_Center);
    }

    if (UHorizontalBoxSlot* HeaderSlot = NavHeaderRow->AddChildToHorizontalBox(
        MakeHeaderCell(TEXT("DIST"), MissionListLayout::NavCol4)))
    {
        HeaderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HeaderSlot->SetHorizontalAlignment(HAlign_Left);
        HeaderSlot->SetVerticalAlignment(VAlign_Center);
    }

    if (UHorizontalBoxSlot* HeaderSlot = NavHeaderRow->AddChildToHorizontalBox(
        MakeHeaderCell(TEXT("SPEED"), MissionListLayout::NavCol5)))
    {
        HeaderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HeaderSlot->SetHorizontalAlignment(HAlign_Left);
        HeaderSlot->SetVerticalAlignment(VAlign_Center);
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
    CellBox->SetHeightOverride(MissionListLayout::RowHeight);

    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Label->SetText(FText::FromString(Text));
    Label->SetJustification(ETextJustify::Left);
    Label->SetAutoWrapText(false);

    // FONT SET HERE
    FSlateFontInfo FontInfo;
    FontInfo.Size = 16; // adjust as needed
    FontInfo.TypefaceFontName = FName("Bold"); // or "Regular"

    // Optional: set a specific font asset
    static ConstructorHelpers::FObjectFinder<UFont> FontObj(TEXT("/Game/Font/SERPNTB"));
    if (FontObj.Succeeded())
    {
        FontInfo.FontObject = FontObj.Object;
    }

    Label->SetFont(FontInfo);

    CellBox->AddChild(Label);

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
}