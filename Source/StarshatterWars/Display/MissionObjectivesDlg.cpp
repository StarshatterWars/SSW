#include "MissionObjectivesDlg.h"
#include "ShipBriefingPreview.h"
#include "MissionUIStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "FlightPanelStyle.h"
#include "Mission.h"
#include "MissionBriefingContent.h"
void UMissionObjectivesDlg::SetMissionBriefing(const Mission& Mission,FText InTitle,FText Agent,FText Status,
    FSimpleDelegate OnAccept,FSimpleDelegate OnDecline,FSimpleDelegate OnClose)
{
    const FText BriefingText = FText::Format(INVTEXT("AGENT: {0}\nSTATUS: {1}\n\n{2}"),
        Agent,Status,FText::FromString(MissionBriefingContent::Build(Mission)));
    if(auto* PC=GetOwningPlayer())
    {
        Rehearsal=CreateWidget<UShipBriefingPreview>(PC);
        if(Rehearsal)
        {
            Rehearsal->UseExternalPlaybackControls();auto Scene=Rehearsal->TakeWidget();
            Rehearsal->SetMissionScene(const_cast<class Mission*>(&Mission));Rehearsal->SetPreviewActive(true);
            const TWeakObjectPtr<UShipBriefingPreview> Preview(Rehearsal);
            auto Controls=SNew(SHorizontalBox);
            auto Add=[&](const TCHAR* Icon,const TCHAR* Tip,void(UShipBriefingPreview::*Action)())
            {
                Controls->AddSlot().AutoWidth().Padding(3)[SNew(SButton).Text(FText::FromString(Icon)).ToolTipText(FText::FromString(Tip))
                    .OnClicked_Lambda([Preview,Action](){if(Preview.IsValid())(Preview.Get()->*Action)();return FReply::Handled();})];
            };
            Add(TEXT("|\u25C0"),TEXT("Restart rehearsal"),&UShipBriefingPreview::RewindRehearsal);
            Add(TEXT("\u25B6"),TEXT("Play rehearsal"),&UShipBriefingPreview::PlayRehearsal);
            Add(TEXT("\u2161"),TEXT("Pause rehearsal"),&UShipBriefingPreview::PauseRehearsal);
            Add(TEXT("\u25A0"),TEXT("Stop rehearsal"),&UShipBriefingPreview::StopRehearsal);
            SetSupplement(SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(INVTEXT("MISSION REHEARSAL")).Font(MissionUIStyle::GetHeaderFont(14))]
                + SVerticalBox::Slot().FillHeight(1)[SNew(SBorder).BorderImage(&FlightPanelStyle::Window()).Padding(0)[SNew(SBox).MinAspectRatio(16.f/9.f).MaxAspectRatio(16.f/9.f)[Scene]]]
                + SVerticalBox::Slot().AutoHeight()[Controls]);
        }
    }
    SetDetails(MoveTemp(InTitle),BriefingText,MoveTemp(OnAccept),MoveTemp(OnDecline),MoveTemp(OnClose));
}

void UMissionObjectivesDlg::NativeDestruct(){if(Rehearsal)Rehearsal->SetPreviewActive(false);Super::NativeDestruct();}
