/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.
*/

#include "MissionWeaponLoadoutLVElement.h"

#include "MissionWeaponLoadoutListObject.h"
#include "MissionUIStyle.h"

#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

void UMissionWeaponLoadoutLVElement::NativeConstruct()
{
    Super::NativeConstruct();

    ApplyTextRules();
    ApplySelectionVisual();

    if (NameSizeBox)
    {
        NameSizeBox->SetWidthOverride(660.f);
    }

    if (WeightSizeBox)
    {
        WeightSizeBox->SetWidthOverride(180.f);
    }

    if (LoadoutNameText)
    {
        LoadoutNameText->SetJustification(ETextJustify::Left);
        LoadoutNameText->SetColorAndOpacity(MissionUIStyle::RowText);
        LoadoutNameText->SetFont(MissionUIStyle::GetRowFont(16));
    }

    if (WeightText)
    {
        WeightText->SetJustification(ETextJustify::Right);
        WeightText->SetColorAndOpacity(MissionUIStyle::RowText);
        WeightText->SetFont(MissionUIStyle::GetRowFont(16));
    }
}

void UMissionWeaponLoadoutLVElement::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    LoadoutItem = Cast<UMissionWeaponLoadoutListObject>(ListItemObject);

    if (!LoadoutItem)
    {
        bRowSelected = false;

        if (LoadoutNameText)
        {
            LoadoutNameText->SetText(FText::GetEmpty());
        }

        if (WeightText)
        {
            WeightText->SetText(FText::GetEmpty());
        }

        ApplySelectionVisual();
        return;
    }

    if (LoadoutNameText)
    {
        LoadoutNameText->SetText(FText::FromString(LoadoutItem->GetLoadoutName()));
        LoadoutNameText->SetJustification(ETextJustify::Left);
        LoadoutNameText->SetColorAndOpacity(MissionUIStyle::RowText);
        LoadoutNameText->SetFont(MissionUIStyle::GetRowFont(16));
    }

    if (WeightText)
    {
        WeightText->SetText(FText::FromString(LoadoutItem->GetWeightText()));
        WeightText->SetJustification(ETextJustify::Right);
        WeightText->SetColorAndOpacity(MissionUIStyle::RowText);
        WeightText->SetFont(MissionUIStyle::GetRowFont(16));
    }

    bRowSelected = LoadoutItem->IsSelected();
    ApplySelectionVisual();
}

void UMissionWeaponLoadoutLVElement::NativeOnItemSelectionChanged(bool bIsSelected)
{
    IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);

    bRowSelected = bIsSelected;

    if (LoadoutItem)
    {
        LoadoutItem->SetSelected(bIsSelected);
    }

    ApplySelectionVisual();
}

void UMissionWeaponLoadoutLVElement::NativeOnEntryReleased()
{
    IUserObjectListEntry::NativeOnEntryReleased();

    LoadoutItem = nullptr;
    bRowSelected = false;

    if (LoadoutNameText)
    {
        LoadoutNameText->SetText(FText::GetEmpty());
    }

    if (WeightText)
    {
        WeightText->SetText(FText::GetEmpty());
    }

    ApplySelectionVisual();
}

void UMissionWeaponLoadoutLVElement::ApplySelectionVisual()
{
    if (!SelectionBorder)
    {
        return;
    }

    if (bRowSelected)
    {
        SelectionBorder->SetBrushColor(MissionUIStyle::RowSelected);
    }
    else
    {
        SelectionBorder->SetBrushColor(MissionUIStyle::RowBG);
        SelectionBorder->SetPadding(FMargin(6.f, 4.f));
    }
}

void UMissionWeaponLoadoutLVElement::ApplyTextRules()
{
    if (LoadoutNameText)
    {
        LoadoutNameText->SetAutoWrapText(false);
        LoadoutNameText->SetJustification(ETextJustify::Left);
        LoadoutNameText->SetColorAndOpacity(MissionUIStyle::RowText);
        LoadoutNameText->SetFont(MissionUIStyle::GetRowFont(16));
    }

    if (WeightText)
    {
        WeightText->SetAutoWrapText(false);
        WeightText->SetJustification(ETextJustify::Right);
        WeightText->SetColorAndOpacity(MissionUIStyle::RowText);
        WeightText->SetFont(MissionUIStyle::GetRowFont(16));
    }
}