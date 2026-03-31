/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    UI / Mission Briefing
    FILE:         MissionPackageLVElement.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UMissionPackageLVElement

    Runtime ListView row widget for briefing package entries.
*/

#include "MissionPackageLVElement.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"

void UMissionPackageLVElement::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    PackageItem = Cast<UMissionPackageListObject>(ListItemObject);
    if (!PackageItem)
    {
        bRowSelected = false;
        ApplySelectionVisual();
        return;
    }

    if (MarkerText)
    {
        MarkerText->SetText(FText::FromString(PackageItem->GetMarker()));
    }

    if (ElementNameText)
    {
        ElementNameText->SetText(FText::FromString(PackageItem->GetElementName()));
    }

    if (RoleText)
    {
        RoleText->SetText(FText::FromString(PackageItem->GetRoleText()));
    }

    if (PackageText)
    {
        PackageText->SetText(FText::FromString(PackageItem->GetPackageText()));
    }

    ApplySelectionVisual();
}

void UMissionPackageLVElement::NativeOnItemSelectionChanged(bool bIsSelected)
{
    IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);

    bRowSelected = bIsSelected;
    ApplySelectionVisual();
}

void UMissionPackageLVElement::NativeOnEntryReleased()
{
    IUserObjectListEntry::NativeOnEntryReleased();

    PackageItem = nullptr;
    bRowSelected = false;
    ApplySelectionVisual();
}

void UMissionPackageLVElement::ApplySelectionVisual()
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