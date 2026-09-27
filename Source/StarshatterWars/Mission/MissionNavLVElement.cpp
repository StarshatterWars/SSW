/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    UI / Mission Briefing
    FILE:         MissionNavLVElement.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UMissionNavLVElement

    Runtime ListView row widget for briefing navigation entries.
*/

#include "MissionNavLVElement.h"
#include "MissionListLayout.h"

#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBoxSlot.h"

void UMissionNavLVElement::NativeConstruct()
{
    Super::NativeConstruct();

    ApplySlotRules();
    ApplyColumnLayout();
    ApplyTextRules();
    ApplySelectionVisual();
}

void UMissionNavLVElement::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    NavItem = Cast<UMissionNavListObject>(ListItemObject);
    if (!NavItem)
    {
        bRowSelected = false;

        if (StepText)
        {
            StepText->SetText(FText::GetEmpty());
        }

        if (ActionText)
        {
            ActionText->SetText(FText::GetEmpty());
        }

        if (RegionText)
        {
            RegionText->SetText(FText::GetEmpty());
        }

        if (DistanceText)
        {
            DistanceText->SetText(FText::GetEmpty());
        }

        if (SpeedText)
        {
            SpeedText->SetText(FText::GetEmpty());
        }

        ApplySelectionVisual();
        return;
    }

    if (StepText)
    {
        StepText->SetText(FText::FromString(NavItem->GetStepText()));
    }

    if (ActionText)
    {
        ActionText->SetText(FText::FromString(NavItem->GetActionText()));
    }

    if (RegionText)
    {
        RegionText->SetText(FText::FromString(NavItem->GetRegionText()));
    }

    if (DistanceText)
    {
        DistanceText->SetText(FText::FromString(NavItem->GetDistanceText()));
    }

    if (SpeedText)
    {
        SpeedText->SetText(FText::FromString(NavItem->GetSpeedText()));
    }

    ApplySelectionVisual();
}

void UMissionNavLVElement::NativeOnItemSelectionChanged(bool bIsSelected)
{
    IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);

    bRowSelected = bIsSelected;
    ApplySelectionVisual();
}

void UMissionNavLVElement::NativeOnEntryReleased()
{
    IUserObjectListEntry::NativeOnEntryReleased();

    NavItem = nullptr;
    bRowSelected = false;

    if (StepText)
    {
        StepText->SetText(FText::GetEmpty());
    }

    if (ActionText)
    {
        ActionText->SetText(FText::GetEmpty());
    }

    if (RegionText)
    {
        RegionText->SetText(FText::GetEmpty());
    }

    if (DistanceText)
    {
        DistanceText->SetText(FText::GetEmpty());
    }

    if (SpeedText)
    {
        SpeedText->SetText(FText::GetEmpty());
    }

    ApplySelectionVisual();
}

void UMissionNavLVElement::ApplySelectionVisual()
{
    if (!SelectionBorder)
    {
        return;
    }

    if (bRowSelected)
    {
        SelectionBorder->SetBrushColor(FLinearColor(0.0f, 0.7f, 1.0f, 0.35f));
    }
    else
    {
        SelectionBorder->SetBrushColor(FLinearColor(0, 0, 0, 0));
    }
}

void UMissionNavLVElement::ApplySlotRules()
{
    auto FixSlot = [](UWidget* Widget)
        {
            if (!Widget)
            {
                return;
            }

            if (UHorizontalBoxSlot* Slot = Cast<UHorizontalBoxSlot>(Widget->Slot))
            {
                Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
                Slot->SetHorizontalAlignment(HAlign_Left);
                Slot->SetVerticalAlignment(VAlign_Center);
            }
        };

    FixSlot(StepSizeBox);
    FixSlot(ActionSizeBox);
    FixSlot(RegionSizeBox);
    FixSlot(DistanceSizeBox);
    FixSlot(SpeedSizeBox);
}

void UMissionNavLVElement::ApplyColumnLayout()
{
    if (RowSizeBox)
    {
        RowSizeBox->SetHeightOverride(MissionListLayout::RowHeight);
    }

    if (StepSizeBox)
    {
        StepSizeBox->SetWidthOverride(MissionListLayout::NavCol1);
    }

    if (ActionSizeBox)
    {
        ActionSizeBox->SetWidthOverride(MissionListLayout::NavCol2);
    }

    if (RegionSizeBox)
    {
        RegionSizeBox->SetWidthOverride(MissionListLayout::NavCol3);
    }

    if (DistanceSizeBox)
    {
        DistanceSizeBox->SetWidthOverride(MissionListLayout::NavCol4);
    }

    if (SpeedSizeBox)
    {
        SpeedSizeBox->SetWidthOverride(MissionListLayout::NavCol5);
    }
}

void UMissionNavLVElement::ApplyTextRules()
{
    auto FixText = [](UTextBlock* Text)
        {
            if (!Text)
            {
                return;
            }

            Text->SetAutoWrapText(false);
            Text->SetJustification(ETextJustify::Left);
        };

    FixText(StepText);
    FixText(ActionText);
    FixText(RegionText);
    FixText(DistanceText);
    FixText(SpeedText);
}