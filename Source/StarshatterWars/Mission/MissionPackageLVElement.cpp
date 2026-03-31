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
#include "MissionListLayout.h"

#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"

void UMissionPackageLVElement::NativeConstruct()
{
    Super::NativeConstruct();

    ApplyColumnLayout();
    ApplyTextRules();
    ApplySelectionVisual();
}

void UMissionPackageLVElement::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    PackageItem = Cast<UMissionPackageListObject>(ListItemObject);
    if (!PackageItem)
    {
        bRowSelected = false;

        if (MarkerText)
        {
            MarkerText->SetText(FText::GetEmpty());
        }

        if (ElementNameText)
        {
            ElementNameText->SetText(FText::GetEmpty());
        }

        if (RoleText)
        {
            RoleText->SetText(FText::GetEmpty());
        }

        if (PackageText)
        {
            PackageText->SetText(FText::GetEmpty());
        }

        ApplySelectionVisual();
        return;
    }

    if (MarkerText)
    {
        MarkerText->SetText(FText::FromString(PackageItem->GetMarker()));
        MarkerText->SetJustification(ETextJustify::Left);
    }

    if (ElementNameText)
    {
        ElementNameText->SetText(FText::FromString(PackageItem->GetElementName()));
        ElementNameText->SetJustification(ETextJustify::Left);
    }

    if (RoleText)
    {
        RoleText->SetText(FText::FromString(PackageItem->GetRoleText()));
        RoleText->SetJustification(ETextJustify::Left);
    }

    if (PackageText)
    {
        PackageText->SetText(FText::FromString(PackageItem->GetPackageText()));
        PackageText->SetJustification(ETextJustify::Left);
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

    if (MarkerText)
    {
        MarkerText->SetText(FText::GetEmpty());
    }

    if (ElementNameText)
    {
        ElementNameText->SetText(FText::GetEmpty());
    }

    if (RoleText)
    {
        RoleText->SetText(FText::GetEmpty());
    }

    if (PackageText)
    {
        PackageText->SetText(FText::GetEmpty());
    }

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

void UMissionPackageLVElement::ApplyColumnLayout()
{
    if (RowSizeBox)
    {
        RowSizeBox->SetHeightOverride(MissionListLayout::RowHeight);
    }

    if (MarkerSizeBox)
    {
        MarkerSizeBox->SetWidthOverride(MissionListLayout::PackageMarkerCol);
    }

    if (ElementNameSizeBox)
    {
        ElementNameSizeBox->SetWidthOverride(MissionListLayout::PackageNameCol);
    }

    if (RoleSizeBox)
    {
        RoleSizeBox->SetWidthOverride(MissionListLayout::PackageRoleCol);
    }

    if (PackageSizeBox)
    {
        PackageSizeBox->SetWidthOverride(MissionListLayout::PackageTextCol);
    }
}

void UMissionPackageLVElement::ApplyTextRules()
{
    if (MarkerText)
    {
        MarkerText->SetAutoWrapText(false);
    }

    if (ElementNameText)
    {
        ElementNameText->SetAutoWrapText(false);
    }

    if (RoleText)
    {
        RoleText->SetAutoWrapText(false);
    }

    if (PackageText)
    {
        PackageText->SetAutoWrapText(false);
    }
}