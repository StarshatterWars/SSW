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
#include "Components/TextBlock.h"
#include "Components/Border.h"

void UMissionNavLVElement::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    NavItem = Cast<UMissionNavListObject>(ListItemObject);
    if (!NavItem)
    {
        bRowSelected = false;
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