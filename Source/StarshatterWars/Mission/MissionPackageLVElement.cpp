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
#include "Components/Image.h"
#include "Components/SizeBox.h"

UMissionPackageLVElement::UMissionPackageLVElement(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    static ConstructorHelpers::FObjectFinder<UTexture2D> ArrowTex(
        TEXT("/Game/ProIconPack/Textures/Generic_Icons/64x64/T_Arrow_R_64x64.T_Arrow_R_64x64"));

    if (ArrowTex.Succeeded())
    {
        PlayerArrowTexture = ArrowTex.Object;
    }
}

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
        if (MarkerImage)
        {
            MarkerImage->SetVisibility(ESlateVisibility::Collapsed);
        }

        if (MarkerText)
        {
            MarkerText->SetVisibility(ESlateVisibility::Collapsed);
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

        return;
    }

    // ---------------------------------------
    //  REPLACE TEXT MARKER WITH TEXTURE
    // ---------------------------------------

    if (MarkerText)
    {
        MarkerText->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (MarkerImage)
    {
        const bool bIsPlayer = !PackageItem->GetMarker().IsEmpty();

        if (bIsPlayer && PlayerArrowTexture)
        {
            MarkerImage->SetBrushFromTexture(PlayerArrowTexture);
            MarkerImage->SetVisibility(ESlateVisibility::Visible);
            MarkerImage->SetColorAndOpacity(FLinearColor(0.2f, 0.6f, 1.0f));
        }
        else
        {
            MarkerImage->SetVisibility(ESlateVisibility::Hidden);
        }
    }

    // ---------------------------------------
    // NORMAL TEXT BINDING
    // ---------------------------------------

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

    ApplyTextRules();
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
    if (MarkerImage)
    {
        MarkerImage->SetVisibility(ESlateVisibility::Collapsed);
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