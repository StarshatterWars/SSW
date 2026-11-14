#pragma once

#include "ShipDesignRegistry.h"
#include "WeaponDesignRegistry.h"
#include "MissionUIStyle.h"
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
            Entries.Add(MakeShared<FEntry>(
                TEXT("Ship · ") + Name,
                FString::Printf(TEXT("%s\nSpeed limit  %.0f m/s\nRepair teams  %d\nCargo capacity  %.2f t"),
                    *Design.ShipClass, Design.Vlimit, Design.RepairTeams, Design.CargoCapacity.Tonnes),
                Design.Description));
        }
        for (const auto& [Key, Design] : WeaponDesignRegistry::GetAll())
        {
            if (Design.bSecret) continue;
            const FString Name = Design.Name.IsEmpty() ? Key.ToString() : Design.Name;
            Entries.Add(MakeShared<FEntry>(
                TEXT("Weapon · ") + Name,
                FString::Printf(TEXT("Range  %.0f m\nProjectile speed  %.0f m/s"), Design.MaxRange, Design.Speed),
                Design.Description));
        }
        Entries.Sort([](const auto& A, const auto& B) { return A->Name < B->Name; });
        if (!Entries.IsEmpty()) Selected = Entries[0];

        ChildSlot.Padding(20, 16)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
            [
                SNew(SComboBox<TSharedPtr<FEntry>>).OptionsSource(&Entries).InitiallySelectedItem(Selected)
                .OnGenerateWidget_Lambda([](TSharedPtr<FEntry> Entry) -> TSharedRef<SWidget>
                {
                    return SNew(STextBlock).Text(Entry ? FText::FromString(Entry->Name) : FText::GetEmpty())
                        .ColorAndOpacity(FLinearColor::White).Font(MissionUIStyle::GetRowFont(16));
                })
                .OnSelectionChanged_Lambda([this](TSharedPtr<FEntry> Entry, ESelectInfo::Type) { Selected = Entry; })
                [
                    SNew(STextBlock).Font(MissionUIStyle::GetRowFont(17)).ColorAndOpacity(FLinearColor::White)
                    .Text_Lambda([this]() { return Selected ? FText::FromString(Selected->Name) : INVTEXT("No reference entries"); })
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
        FString Name;
        FString Stats;
        FString Description;
    };
    TArray<TSharedPtr<FEntry>> Entries;
    TSharedPtr<FEntry> Selected;
};
