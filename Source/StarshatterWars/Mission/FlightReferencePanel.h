#pragma once

#include "ShipDesignRegistry.h"
#include "WeaponDesignRegistry.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"

class SFlightReferencePanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SFlightReferencePanel) {}
    SLATE_END_ARGS()

    void Construct(const FArguments&)
    {
        for (const auto& [Key, Design] : ShipDesignRegistry::GetAll())
        {
            if (Design.Secret || Design.Hidden) continue;
            const FString Name = Design.DisplayName.IsEmpty() ? Key.ToString() : Design.DisplayName;
            const TCHAR* Type;
            EShipSystemIcon Icon;
            switch (Design.Category)
            {
            case EShipCategory::CapitalShip: Type = TEXT("Ship"); Icon = EShipSystemIcon::Ship; break;
            case EShipCategory::Transport: Type = TEXT("Transport"); Icon = EShipSystemIcon::Transport; break;
            case EShipCategory::Shuttle: Type = TEXT("Shuttle"); Icon = EShipSystemIcon::Transport; break;
            case EShipCategory::Station: Type = TEXT("Station"); Icon = EShipSystemIcon::Station; break;
            case EShipCategory::Platform: Type = TEXT("Platform"); Icon = EShipSystemIcon::Station; break;
            case EShipCategory::Building: Type = TEXT("Building"); Icon = EShipSystemIcon::Building; break;
            case EShipCategory::Fighter: Type = TEXT("Fighter"); Icon = EShipSystemIcon::Fighter; break;
            default: continue;
            }
            Catalog.Add(MakeShared<FEntry>(
                Icon, FString::Printf(TEXT("%s: %s"), Type, *Name),
                FString::Printf(TEXT("%s\nSpeed limit  %.0f m/s\nRepair teams  %d\nCargo capacity  %.2f t"),
                    *Design.ShipClass, Design.Vlimit, Design.RepairTeams, Design.CargoCapacity.Tonnes),
                Design.Description));
        }
        for (const auto& [Key, Design] : WeaponDesignRegistry::GetAll())
        {
            if (Design.bSecret) continue;
            const FString Name = Design.Name.IsEmpty() ? Key.ToString() : Design.Name;
            Catalog.Add(MakeShared<FEntry>(
                EShipSystemIcon::Weapon, TEXT("Weapon · ") + Name,
                FString::Printf(TEXT("Range  %.0f m\nProjectile speed  %.0f m/s"), Design.MaxRange, Design.Speed),
                Design.Description));
        }
        Catalog.Sort([](const auto& A, const auto& B) { return A->Name < B->Name; });
        SelectCategory(EShipSystemIcon::Ship);

        auto Filters = SNew(SHorizontalBox);
        const auto AddFilter = [&](EShipSystemIcon Icon, const FText& Label)
        {
            Filters->AddSlot().FillWidth(1).Padding(0, 0, 6, 0)
            [
                SNew(SButton).ContentPadding(FMargin(8, 5)).HAlign(HAlign_Center).ToolTipText(Label)
                .ButtonColorAndOpacity_Lambda([this, Icon]()
                {
                    return ActiveCategory == Icon ? MissionUIStyle::RowSelectedBG : MissionUIStyle::RowBG;
                })
                .OnClicked_Lambda([this, Icon]()
                {
                    if (ActiveCategory != Icon) SelectCategory(Icon);
                    return FReply::Handled();
                })
                [ SNew(SShipSystemIcon).Icon(Icon) ]
            ];
        };
        AddFilter(EShipSystemIcon::Ship, INVTEXT("Ships"));
        AddFilter(EShipSystemIcon::Transport, INVTEXT("Transports and shuttles"));
        AddFilter(EShipSystemIcon::Fighter, INVTEXT("Fighters"));
        AddFilter(EShipSystemIcon::Station, INVTEXT("Stations and platforms"));
        AddFilter(EShipSystemIcon::Building, INVTEXT("Buildings"));
        AddFilter(EShipSystemIcon::Weapon, INVTEXT("Weapons"));

        ChildSlot.Padding(20, 16)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
            [ Filters ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
            [
                SAssignNew(Picker, SComboBox<TSharedPtr<FEntry>>).OptionsSource(&Entries).InitiallySelectedItem(Selected)
                .OnGenerateWidget_Lambda([](TSharedPtr<FEntry> Entry) -> TSharedRef<SWidget>
                {
                    return SNew(STextBlock).Text(Entry ? FText::FromString(Entry->Name) : FText::GetEmpty())
                        .ColorAndOpacity(FLinearColor::White).Font(MissionUIStyle::GetRowFont(16));
                })
                .OnSelectionChanged_Lambda([this](TSharedPtr<FEntry> Entry, ESelectInfo::Type) { Selected = Entry; })
                [
                    SNew(STextBlock).Font(MissionUIStyle::GetRowFont(17)).ColorAndOpacity(FLinearColor::White)
                    .Text_Lambda([this]() { return Selected ? FText::FromString(Selected->Name) : INVTEXT("No entries in this category"); })
                ]
            ]
            + SVerticalBox::Slot().FillHeight(1)
            [
                SNew(SScrollBox)
                + SScrollBox::Slot().Padding(0, 0, 12, 16)
                [
                    SNew(STextBlock).Font(MissionUIStyle::GetRowFont(17)).ColorAndOpacity(FLinearColor(.48f, .8f, .94f))
                    .AutoWrapText(true).Text_Lambda([this]() { return Selected ? FText::FromString(Selected->Stats) : FText::GetEmpty(); })
                ]
                + SScrollBox::Slot().Padding(0, 0, 12, 0)
                [
                    SNew(STextBlock).Font(MissionUIStyle::GetRowFont(17)).AutoWrapText(true)
                    .Text_Lambda([this]() { return Selected ? FText::FromString(Selected->Description) : FText::GetEmpty(); })
                ]
            ]
        ];
    }

private:
    struct FEntry
    {
        EShipSystemIcon Category;
        FString Name;
        FString Stats;
        FString Description;
    };
    void SelectCategory(EShipSystemIcon Category)
    {
        ActiveCategory = Category;
        Entries.Reset();
        for (const auto& Entry : Catalog)
        {
            if (Entry->Category == Category) Entries.Add(Entry);
        }
        const TSharedPtr<FEntry> First = Entries.IsEmpty() ? nullptr : Entries[0];
        if (Picker)
        {
            Picker->ClearSelection();
            Picker->RefreshOptions();
            if (First) Picker->SetSelectedItem(First);
        }
        Selected = First;
    }

    EShipSystemIcon ActiveCategory = EShipSystemIcon::Ship;
    TArray<TSharedPtr<FEntry>> Catalog;
    TArray<TSharedPtr<FEntry>> Entries;
    TSharedPtr<SComboBox<TSharedPtr<FEntry>>> Picker;
    TSharedPtr<FEntry> Selected;
};
