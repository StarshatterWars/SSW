/*  Project Starshatter Wars
    Fractal Dev Studios

    SUBSYSTEM:    Stars.exe
    FILE:         MissionWeaponStationLVElement.cpp

    OVERVIEW
    ========
    Interactive station row widget for runtime MissionLoad editing.
*/

#include "MissionWeaponStationLVElement.h"

#include "MissionWeaponDlg.h"
#include "MissionWeaponStationRowObject.h"
#include "MissionUIStyle.h"

#include "Components/Border.h"
#include "Components/ComboBoxString.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

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

    if (WeaponCombo)
    {
        WeaponCombo->OnSelectionChanged.RemoveAll(this);
        WeaponCombo->OnSelectionChanged.AddDynamic(
            this,
            &UMissionWeaponStationLVElement::HandleWeaponSelectionChanged);

        // Closed combo button colors:
        WeaponCombo->WidgetStyle.ComboButtonStyle.ButtonStyle.Normal.TintColor =
            FSlateColor(MissionUIStyle::ComboBG);

        WeaponCombo->WidgetStyle.ComboButtonStyle.ButtonStyle.Hovered.TintColor =
            FSlateColor(MissionUIStyle::ComboHoverBG);

        WeaponCombo->WidgetStyle.ComboButtonStyle.ButtonStyle.Pressed.TintColor =
            FSlateColor(MissionUIStyle::ComboPressedBG);

        // Text/foreground colors:
        WeaponCombo->WidgetStyle.ComboButtonStyle.ButtonStyle.NormalForeground =
            FSlateColor(MissionUIStyle::ComboText);

        WeaponCombo->WidgetStyle.ComboButtonStyle.ButtonStyle.HoveredForeground =
            FSlateColor(MissionUIStyle::ComboText);

        WeaponCombo->WidgetStyle.ComboButtonStyle.ButtonStyle.PressedForeground =
            FSlateColor(MissionUIStyle::ComboText);

        // Popup/menu border tint:
        WeaponCombo->WidgetStyle.ComboButtonStyle.MenuBorderBrush.TintColor =
            FSlateColor(MissionUIStyle::ComboMenuBG);

        // Dropdown row text:
        WeaponCombo->ItemStyle.TextColor =
            FSlateColor(MissionUIStyle::ComboText);

        WeaponCombo->ItemStyle.SelectedTextColor =
            FSlateColor(MissionUIStyle::ComboText);

        // Dropdown row background states:
        WeaponCombo->ItemStyle.ActiveBrush.TintColor =
            FSlateColor(MissionUIStyle::ComboItemSelectedBG);

        WeaponCombo->ItemStyle.ActiveHoveredBrush.TintColor =
            FSlateColor(MissionUIStyle::ComboItemSelectedBG);

        WeaponCombo->ItemStyle.InactiveBrush.TintColor =
            FSlateColor(MissionUIStyle::ComboItemBG);

        WeaponCombo->ItemStyle.InactiveHoveredBrush.TintColor =
            FSlateColor(MissionUIStyle::ComboItemHoverBG);
    }
}

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

        if (WeaponCombo)
        {
            bUpdatingCombo = true;

            WeaponCombo->OnSelectionChanged.RemoveAll(this);
            WeaponCombo->ClearOptions();
            WeaponCombo->ClearSelection();

            WeaponCombo->OnSelectionChanged.AddDynamic(
                this,
                &UMissionWeaponStationLVElement::HandleWeaponSelectionChanged);

            bUpdatingCombo = false;
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

    if (WeaponCombo)
    {
        bUpdatingCombo = true;

        WeaponCombo->OnSelectionChanged.RemoveAll(this);
        WeaponCombo->ClearOptions();

        const TArray<FString>& AllowedWeapons = StationItem->GetAllowedWeapons();

        for (const FString& WeaponName : AllowedWeapons)
        {
            WeaponCombo->AddOption(WeaponName);
        }

        const int32 CurrentSelection = StationItem->GetCurrentSelection();

        if (AllowedWeapons.IsValidIndex(CurrentSelection))
        {
            WeaponCombo->SetSelectedOption(AllowedWeapons[CurrentSelection]);
        }
        else
        {
            WeaponCombo->ClearSelection();
        }

        WeaponCombo->OnSelectionChanged.AddDynamic(
            this,
            &UMissionWeaponStationLVElement::HandleWeaponSelectionChanged);

        bUpdatingCombo = false;
    }

    bRowSelected = false;
    ApplySelectionVisual();
}

void UMissionWeaponStationLVElement::HandleWeaponSelectionChanged(
    FString SelectedItem,
    ESelectInfo::Type SelectionType)
{
    if (bUpdatingCombo)
    {
        return;
    }

    if (!StationItem)
    {
        return;
    }

    const int32 NewSelection =
        StationItem->GetAllowedWeapons().IndexOfByKey(SelectedItem);

    if (NewSelection == INDEX_NONE)
    {
        return;
    }

    StationItem->SetCurrentSelection(NewSelection);
    StationItem->SetWeaponName(SelectedItem);

    if (WeaponText)
    {
        WeaponText->SetText(FText::FromString(SelectedItem));
    }

    if (OwningWeaponDlg)
    {
        OwningWeaponDlg->HandleStationChanged(
            StationItem->GetStationIndex(),
            NewSelection);
    }
}

void UMissionWeaponStationLVElement::ApplyTextRules()
{
    if (StationText)
    {
        StationText->SetAutoWrapText(false);
        StationText->SetMinDesiredWidth(1.f);
    }

    if (WeaponText)
    {
        WeaponText->SetAutoWrapText(false);
        WeaponText->SetMinDesiredWidth(1.f);
    }
}

void UMissionWeaponStationLVElement::ApplySelectionVisual()
{
    if (!RowBorder)
    {
        return;
    }

    RowBorder->SetBrushColor(
        bRowSelected
        ? MissionUIStyle::RowSelected
        : MissionUIStyle::RowBG);
}