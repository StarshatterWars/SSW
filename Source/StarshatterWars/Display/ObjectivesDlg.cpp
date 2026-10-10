#include "ObjectivesDlg.h"
#include "ObjectivesPopup.h"
#include "Sim.h"

TSharedRef<SWidget> UObjectivesDlg::CreatePanelContent()
{
    const TWeakObjectPtr<UObjectivesDlg> Owner(this);
    return SNew(SObjectivesPopup).Embedded(true)
        .IsMissionActive([Owner]() {
            auto* Simulation = Sim::GetSim();
            return Owner.IsValid() && Simulation && Simulation->GetMission();
        })
        .OnClose(FSimpleDelegate::CreateWeakLambda(this, [this]() { RequestPanelClose(); }));
}

FText UObjectivesDlg::GetPanelCaption() const
{
    return FText::FromString(TEXT("MISSION OBJECTIVES"));
}
