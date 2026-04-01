/*  Project Starshatter Wars
    Fractal Dev Studios

    SUBSYSTEM:    Stars.exe
    FILE:         MissionWeaponStationLVElement.cpp

    OVERVIEW
    ========
    ListView entry widget for displaying a single
    station and its selected weapon.

    Mirrors behavior of MissionWeaponLoadoutLVElement:
    - consistent text styling
    - selection highlighting
    - entry lifecycle handling
*/

#include "MissionWeaponStationLVElement.h"

#include "MissionWeaponStationRowObject.h"
#include "MissionUIStyle.h"

#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

// ------------------------------------------------------------

void UMissionWeaponStationLVElement::NativeConstruct()
{
    Super::NativeConstruct();

    ApplyTextRules();
    ApplySelectionVisual();

    if (StationSizeBox)
    {
        StationSizeBox->SetWidthOverride(240.f);
    }

    if (WeaponSizeBox)
    {
        WeaponSizeBox->SetWidthOverride(600.f);
    }

    if (StationText)
    {
        StationText->SetJustification(ETextJustify::Left);
        StationText->SetColorAndOpacity(MissionUIStyle::RowText);
        StationText->SetFont(MissionUIStyle::GetRowFont(16));
    }

    if (WeaponText)
    {
        WeaponText->SetJustification(ETextJustify::Left);
        WeaponText->SetColorAndOpacity(MissionUIStyle::RowText);
        WeaponText->SetFont(MissionUIStyle::GetRowFont(16));
    }
}

// ------------------------------------------------------------

void UMissionWeaponStationLVElement::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    StationItem = Cast<UMissionWeaponStationRowObject>(ListItemObject);

    if (!StationItem)
    {
        bRowSelected = false;

        if (StationText)
        {
            StationText->SetText(FText::GetEmpty());
        }

        if (WeaponText)
        {
            WeaponText->SetText(FText::GetEmpty());
        }

        ApplySelectionVisual();
        return;
    }

    if (StationText)
    {
        StationText->SetText(FText::FromString(StationItem->GetStationLabel()));
        StationText->SetJustification(ETextJustify::Left);
        StationText->SetColorAndOpacity(MissionUIStyle::RowText);
        StationText->SetFont(MissionUIStyle::GetRowFont(16));
    }

    if (WeaponText)
    {
        WeaponText->SetText(FText::FromString(StationItem->GetWeaponName()));
        WeaponText->SetJustification(ETextJustify::Left);
        WeaponText->SetColorAndOpacity(MissionUIStyle::RowText);
        WeaponText->SetFont(MissionUIStyle::GetRowFont(16));
    }

    bRowSelected = false;
    ApplySelectionVisual();
}

// ------------------------------------------------------------

void UMissionWeaponStationLVElement::NativeOnItemSelectionChanged(bool bIsSelected)
{
    IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);

    bRowSelected = bIsSelected;
    ApplySelectionVisual();
}

// ------------------------------------------------------------

void UMissionWeaponStationLVElement::NativeOnEntryReleased()
{
    IUserObjectListEntry::NativeOnEntryReleased();

    StationItem = nullptr;
    bRowSelected = false;

    if (StationText)
    {
        StationText->SetText(FText::GetEmpty());
    }

    if (WeaponText)
    {
        WeaponText->SetText(FText::GetEmpty());
    }

    ApplySelectionVisual();
}

// ------------------------------------------------------------

void UMissionWeaponStationLVElement::ApplySelectionVisual()
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

// ------------------------------------------------------------

void UMissionWeaponStationLVElement::ApplyTextRules()
{
    if (StationText)
    {
        StationText->SetAutoWrapText(false);
        StationText->SetJustification(ETextJustify::Left);
        StationText->SetColorAndOpacity(MissionUIStyle::RowText);
        StationText->SetFont(MissionUIStyle::GetRowFont(16));
    }

    if (WeaponText)
    {
        WeaponText->SetAutoWrapText(false);
        WeaponText->SetJustification(ETextJustify::Left);
        WeaponText->SetColorAndOpacity(MissionUIStyle::RowText);
        WeaponText->SetFont(MissionUIStyle::GetRowFont(16));
    }
}