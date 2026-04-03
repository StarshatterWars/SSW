/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    UI / Mission Briefing
    FILE:         MissionNavObjectLVElement.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Runtime ListView row widget for right-side MissionNav object entries.

    This widget builds its row layout fully in code.
*/

#include "MissionNavObjectLVElement.h"
#include "MissionUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

namespace MissionNavObjectLayout
{
    static constexpr float RowHeight = 28.0f;
    static constexpr float PrimaryCol = 190.0f;
    static constexpr float SecondaryCol = 96.0f;
    static constexpr float LeftPadding = 6.0f;
}

void UMissionNavObjectLVElement::NativeConstruct()
{
    Super::NativeConstruct();

    BuildRuntimeWidget();
    ApplySlotRules();
    ApplyColumnLayout();
    ApplyTextRules();

    if (SelectionBorder)
    {
        SelectionBorder->SetBrushColor(MissionUIStyle::RowBG);
    }

    ApplySelectionVisual();
}

void UMissionNavObjectLVElement::BuildRuntimeWidget()
{
    if (!WidgetTree)
    {
        return;
    }

    if (SelectionBorder)
    {
        return;
    }

    SelectionBorder =
        WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            TEXT("SelectionBorder"));
    SelectionBorder->SetPadding(FMargin(0.f));
    SelectionBorder->SetBrushColor(MissionUIStyle::RowBG);

    RowSizeBox =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("RowSizeBox"));
    RowSizeBox->SetHeightOverride(MissionNavObjectLayout::RowHeight);

    RowHorizontalBox =
        WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(),
            TEXT("RowHorizontalBox"));

    PrimarySizeBox =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("PrimarySizeBox"));
    PrimarySizeBox->SetWidthOverride(MissionNavObjectLayout::PrimaryCol);

    SecondarySizeBox =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("SecondarySizeBox"));
    SecondarySizeBox->SetWidthOverride(MissionNavObjectLayout::SecondaryCol);

    PrimaryText =
        WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),
            TEXT("PrimaryText"));
    PrimaryText->SetText(FText::GetEmpty());

    SecondaryText =
        WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),
            TEXT("SecondaryText"));
    SecondaryText->SetText(FText::GetEmpty());

    PrimarySizeBox->AddChild(PrimaryText);
    SecondarySizeBox->AddChild(SecondaryText);

    if (UHorizontalBoxSlot* PrimarySlot = RowHorizontalBox->AddChildToHorizontalBox(PrimarySizeBox))
    {
        PrimarySlot->SetPadding(FMargin(MissionNavObjectLayout::LeftPadding, 0.f, 0.f, 0.f));
        PrimarySlot->SetHorizontalAlignment(HAlign_Left);
        PrimarySlot->SetVerticalAlignment(VAlign_Center);
        PrimarySlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    if (UHorizontalBoxSlot* SecondarySlot = RowHorizontalBox->AddChildToHorizontalBox(SecondarySizeBox))
    {
        SecondarySlot->SetPadding(FMargin(MissionNavObjectLayout::LeftPadding, 0.f, 0.f, 0.f));
        SecondarySlot->SetHorizontalAlignment(HAlign_Left);
        SecondarySlot->SetVerticalAlignment(VAlign_Center);
        SecondarySlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    RowSizeBox->AddChild(RowHorizontalBox);
    SelectionBorder->SetContent(RowSizeBox);

    WidgetTree->RootWidget = SelectionBorder;
}

void UMissionNavObjectLVElement::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    ObjectItem = Cast<UMissionNavObjectListObject>(ListItemObject);
    if (!ObjectItem)
    {
        bRowSelected = false;

        if (PrimaryText)
        {
            PrimaryText->SetText(FText::GetEmpty());
        }

        if (SecondaryText)
        {
            SecondaryText->SetText(FText::GetEmpty());
        }

        ApplySelectionVisual();
        return;
    }

    if (PrimaryText)
    {
        PrimaryText->SetText(FText::FromString(ObjectItem->GetPrimaryText()));
    }

    if (SecondaryText)
    {
        SecondaryText->SetText(FText::FromString(ObjectItem->GetSecondaryText()));
    }

    ApplySelectionVisual();
}

void UMissionNavObjectLVElement::NativeOnItemSelectionChanged(bool bIsSelected)
{
    IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);

    bRowSelected = bIsSelected;
    ApplySelectionVisual();
}

void UMissionNavObjectLVElement::NativeOnEntryReleased()
{
    IUserObjectListEntry::NativeOnEntryReleased();

    ObjectItem = nullptr;
    bRowSelected = false;

    if (PrimaryText)
    {
        PrimaryText->SetText(FText::GetEmpty());
    }

    if (SecondaryText)
    {
        SecondaryText->SetText(FText::GetEmpty());
    }

    ApplySelectionVisual();
}

void UMissionNavObjectLVElement::ApplySelectionVisual()
{
    if (!SelectionBorder)
    {
        return;
    }

    if (bRowSelected)
    {
        SelectionBorder->SetBrushColor(MissionUIStyle::RowSelectedBG);
    }
    else
    {
        SelectionBorder->SetBrushColor(MissionUIStyle::RowBG);
    }
}

void UMissionNavObjectLVElement::ApplySlotRules()
{
    auto FixSlot = [](UWidget* Widget)
        {
            if (!Widget)
            {
                return;
            }

            if (UHorizontalBoxSlot* HBSlot = Cast<UHorizontalBoxSlot>(Widget->Slot))
            {
                HBSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
                HBSlot->SetHorizontalAlignment(HAlign_Left);
                HBSlot->SetVerticalAlignment(VAlign_Center);
            }
        };

    FixSlot(PrimarySizeBox);
    FixSlot(SecondarySizeBox);
}

void UMissionNavObjectLVElement::ApplyColumnLayout()
{
    if (RowSizeBox)
    {
        RowSizeBox->SetHeightOverride(MissionNavObjectLayout::RowHeight);
    }

    if (PrimarySizeBox)
    {
        PrimarySizeBox->SetWidthOverride(MissionNavObjectLayout::PrimaryCol);
    }

    if (SecondarySizeBox)
    {
        SecondarySizeBox->SetWidthOverride(MissionNavObjectLayout::SecondaryCol);
    }
}

void UMissionNavObjectLVElement::ApplyTextRules()
{
    auto FixText = [](UTextBlock* Text)
        {
            if (!Text)
            {
                return;
            }

            Text->SetAutoWrapText(false);
            Text->SetJustification(ETextJustify::Left);
            Text->SetColorAndOpacity(MissionUIStyle::RowText);
            Text->SetFont(MissionUIStyle::GetTableRowFont());
        };

    FixText(PrimaryText);
    FixText(SecondaryText);
}