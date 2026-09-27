/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.
*/

#include "MissionWeaponLVElement.h"

#include "Components/TextBlock.h"
#include "Components/Border.h"

void UMissionWeaponLVElement::NativeConstruct()
{
    Super::NativeConstruct();
    ApplySelectionVisual();
}

void UMissionWeaponLVElement::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    WeaponItem = Cast<UMissionWeaponListObject>(ListItemObject);
    if (!WeaponItem)
    {
        if (StationText) StationText->SetText(FText::GetEmpty());
        if (AllowedWeaponsText) AllowedWeaponsText->SetText(FText::GetEmpty());
        if (SelectedWeaponText) SelectedWeaponText->SetText(FText::GetEmpty());
        if (AmmoText) AmmoText->SetText(FText::GetEmpty());

        bRowSelected = false;
        ApplySelectionVisual();
        return;
    }

    if (StationText)
    {
        StationText->SetText(FText::FromString(WeaponItem->GetStationText()));
        StationText->SetJustification(ETextJustify::Left);
    }

    if (AllowedWeaponsText)
    {
        AllowedWeaponsText->SetText(FText::FromString(WeaponItem->GetAllowedWeaponsText()));
        AllowedWeaponsText->SetJustification(ETextJustify::Left);
    }

    if (SelectedWeaponText)
    {
        SelectedWeaponText->SetText(FText::FromString(WeaponItem->GetSelectedWeaponText()));
        SelectedWeaponText->SetJustification(ETextJustify::Left);
    }

    if (AmmoText)
    {
        AmmoText->SetText(FText::FromString(WeaponItem->GetAmmoText()));
        AmmoText->SetJustification(ETextJustify::Left);
    }

    ApplySelectionVisual();
}

void UMissionWeaponLVElement::NativeOnItemSelectionChanged(bool bIsSelected)
{
    IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);
    bRowSelected = bIsSelected;
    ApplySelectionVisual();
}

void UMissionWeaponLVElement::NativeOnEntryReleased()
{
    IUserObjectListEntry::NativeOnEntryReleased();

    WeaponItem = nullptr;
    bRowSelected = false;

    if (StationText) StationText->SetText(FText::GetEmpty());
    if (AllowedWeaponsText) AllowedWeaponsText->SetText(FText::GetEmpty());
    if (SelectedWeaponText) SelectedWeaponText->SetText(FText::GetEmpty());
    if (AmmoText) AmmoText->SetText(FText::GetEmpty());

    ApplySelectionVisual();
}

void UMissionWeaponLVElement::ApplySelectionVisual()
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
        SelectionBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.f));
    }
}