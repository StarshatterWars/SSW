#include "ObjectivesDlg.h"
#include "ObjectivesPopup.h"
#include "Sim.h"
#include "FlightPanelStyle.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

TSharedRef<SWidget> UObjectivesDlg::CreatePanelContent()
{
    if (PanelTitle) PanelTitle->SetVisibility(ESlateVisibility::Collapsed);
    const TWeakObjectPtr<UObjectivesDlg> Owner(this);
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(0, 16, 0, 16)
        [
            SNew(SBox).HeightOverride(1)
            [
                SNew(SBorder).Padding(0)
                .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FlightPanelStyle::DividerColor())
            ]
        ]
        + SVerticalBox::Slot().FillHeight(1)
        [
            SNew(SObjectivesPopup).Embedded(true)
            .IsMissionActive([Owner]() {
                auto* Simulation = Sim::GetSim();
                return Owner.IsValid() && Simulation && Simulation->GetMission();
            })
        ];
}

FText UObjectivesDlg::GetPanelCaption() const
{
    return FText::FromString(TEXT("MISSION OBJECTIVES"));
}
