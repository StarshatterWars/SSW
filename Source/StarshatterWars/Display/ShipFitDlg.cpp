#include "ShipFitDlg.h"
#include "FittingMetrics.h"
#include "Sim.h"
#include "Ship.h"
#include "ShipInsuranceSubsystem.h"
#include "StarshatterSystemDesignSubsystem.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "Engine/GameInstance.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "ShipBriefingPreview.h"
#include "ShipDesignRegistry.h"
#include "Widgets/SNullWidget.h"

TSharedRef<SWidget> UShipFitDlg::RebuildWidget()
{
    // UMG can rebuild this panel in the editor without a live game instance.
    if (IsDesignTime())
        return SNew(STextBlock).Font(MissionUIStyle::GetHeaderFont(18)).Text(INVTEXT("SHIP FITTING"));
    auto* Instance = GetGameInstance();
    auto* Service = Instance ? Instance->GetSubsystem<UShipInsuranceSubsystem>() : nullptr;
    if (!Service)
        return SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13))
            .Text(INVTEXT("Hangar services are available during a game session."));
    Service->RefreshAssignment();
    const auto* HullEntry = Service->FindShip(Hull);
    if (!HullEntry) return SNew(STextBlock).Text(INVTEXT("Select a hull in Hangar."));
    const auto Selected = MakeShared<int32>(INDEX_NONE);
    const auto Grade = MakeShared<int32>(3);
    const auto Feedback = MakeShared<FText>();
    const auto Slots = SNew(SVerticalBox);
    const auto Weak = TWeakObjectPtr<UShipInsuranceSubsystem>(Service);
    for (int32 Index = 0; Index < HullEntry->Components.Num(); ++Index)
    {
        Slots->AddSlot().AutoHeight().Padding(0, 2)
        [
            SNew(SButton).HAlign(HAlign_Fill)
            .ButtonColorAndOpacity_Lambda([Selected, Index]()
            { return *Selected == Index ? MissionUIStyle::RowSelectedBG : MissionUIStyle::RowBG; })
            .OnClicked_Lambda([Selected, Index]() { *Selected = Index; return FReply::Handled(); })
            [
                SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12))
                .Text_Lambda([Weak, Id = Hull, Index]()
                {
                    const auto* Entry = Weak.IsValid() ? Weak->FindShip(Id) : nullptr;
                    if (!Entry || !Entry->Components.IsValidIndex(Index)) return FText::GetEmpty();
                    const auto& Item = Entry->Components[Index];
                    static const TCHAR* Levels[] = {TEXT("I"), TEXT("II"), TEXT("III"), TEXT("IV"), TEXT("V")};
                    return FText::FromString(FString::Printf(TEXT("%s  /  %s  [%s]"), *Item.SystemName,
                        *Item.Component.ToString(), Levels[FMath::Clamp(Item.Level, 1, 5) - 1])).ToUpper();
                })
            ]
        ];
    }
    const auto Grades = SNew(SHorizontalBox);
    const TCHAR* Names[] = {TEXT("I"), TEXT("II"), TEXT("III"), TEXT("IV"), TEXT("V")};
    for (int32 Level = 1; Level <= 5; ++Level)
        Grades->AddSlot().FillWidth(1).Padding(2)
        [
            SNew(SButton).Text(FText::FromString(Names[Level - 1]))
            .ButtonColorAndOpacity_Lambda([Grade, Level]()
            { return *Grade == Level ? MissionUIStyle::RowSelectedBG : MissionUIStyle::RowBG; })
            .OnClicked_Lambda([Grade, Level]() { *Grade = Level; return FReply::Handled(); })
        ];
    Preview = CreateWidget<UShipBriefingPreview>(GetOwningPlayer());
    TSharedPtr<SWidget> PreviewSlate;
    if (Preview)
    {
        Preview->SetReferencePreview();
        PreviewSlate = Preview->TakeWidget();
        if (const auto* Design = ShipDesignRegistry::Find(HullEntry->Design)) Preview->SetShipDesign(Design);
    }
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(10)
        [SNew(STextBlock).Text(FText::FromString(HullEntry->Name).ToUpper()).Font(MissionUIStyle::GetHeaderFont(18))]
        + SVerticalBox::Slot().AutoHeight().Padding(8)
        [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true)
            .ToolTipText(INVTEXT("Nominal unmitigated damage at minimum range, before ammunition depletion. Sustained output assumes rated power requests. Actual output depends on aim, range, capacitor charge and power allocation. Values use SSW firing and reactor data."))
            .Text_Lambda([]() { auto* Simulation = Sim::GetSim(); auto* Craft = Simulation ? Simulation->GetPlayerShip() : nullptr;
                return Craft ? FFittingMetrics::Describe(*Craft) : FText::GetEmpty(); })]
        + SVerticalBox::Slot().FillHeight(1)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 8, 0)
            [MissionUIStyle::MakeDataPane(SNew(SScrollBox) + SScrollBox::Slot()[Slots])]
            + SHorizontalBox::Slot().FillWidth(1)
            [PreviewSlate.IsValid() ? PreviewSlate.ToSharedRef() : SNullWidget::NullWidget]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(8)[Grades]
        + SVerticalBox::Slot().AutoHeight().Padding(8)
        [
            SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13)).AutoWrapText(true)
            .Text_Lambda([Weak, Id = Hull, Selected, Grade, Instance = TWeakObjectPtr<UGameInstance>(GetGameInstance())]()
            {
                const auto* Entry = Weak.IsValid() ? Weak->FindShip(Id) : nullptr;
                if (!Entry || !Entry->Components.IsValidIndex(*Selected) || !Instance.IsValid())
                    return INVTEXT("Select an installed component to compare levels I–V.");
                const auto& Slot = Entry->Components[*Selected];
                const auto* Catalog = Instance->GetSubsystem<UStarshatterSystemDesignSubsystem>();
                const auto Variant = Catalog->FindComponentGrade(Slot.SystemDesign, Slot.Component, *Grade);
                const auto* Design = Catalog->FindDesign(Slot.SystemDesign);
                const auto* Base = Design ? Design->Components.FindByPredicate([&](const auto& Item)
                    { return FName(*Item.Name) == Slot.Component; }) : nullptr;
                if (!Variant || !Base) return INVTEXT("No compatible catalog component.");
                return FText::Format(INVTEXT("Repair: {0} s   Replacement: {1} s   Refit: {2} CR   Balance: {3} CR\nPerformance: {4}%   Power demand: {5}%"),
                    FText::AsNumber(Variant->RepairTime), FText::AsNumber(Variant->ReplaceTime),
                    FText::AsNumber(Variant->BaseCost),
                    FText::AsNumber(Weak->GetBalance()), FText::AsNumber(Variant->PerformanceMultiplier * 100), FText::AsNumber(Variant->PowerDemandMultiplier * 100));
            })
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(8)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Text_Lambda([Feedback]() { return *Feedback; })]
            + SHorizontalBox::Slot().AutoWidth()
            [
                SNew(SButton).ToolTipText(INVTEXT("Refit selected component — docked service"))
                .IsEnabled_Lambda([Weak, Selected, Id = Hull]() { return Weak.IsValid() && Weak->CanModifyHull(Id) && *Selected != INDEX_NONE; })
                .OnClicked_Lambda([Weak, Id = Hull, Selected, Grade, Feedback]()
                {
                    if (Weak.IsValid()) Weak->FitComponent(Id, *Selected, *Grade, *Feedback);
                    return FReply::Handled();
                })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Repair)]
            ]
        ];
}
