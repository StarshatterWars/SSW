#include "ObjectivesDlg.h"
#include "ObjectivesPopup.h"
#include "Sim.h"
#include "MissionPackageDlg.h"
#include "MissionBriefingDlg.h"
#include "MissionUIStyle.h"
#include "ShipBriefingPreview.h"
#include "Widgets/Layout/SBorder.h"
#include "FlightPanelStyle.h"
#include "ShipSystemIcon.h"
#include "Components/TextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"

void UObjectivesDlg::SetMissionContext(UMissionBriefingDlg* InBriefing, bool bShowPackage)
{
    Briefing=InBriefing; bIncludePackage=bShowPackage; ContentInsets=FMargin(12.f);
}
TSharedRef<SWidget> UObjectivesDlg::CreatePanelContent()
{
    if(PanelTitle) PanelTitle->SetVisibility(ESlateVisibility::Collapsed);
    const TWeakObjectPtr<UObjectivesDlg> Owner(this);
    auto Objectives=SNew(SObjectivesPopup).Embedded(true).IsMissionActive([Owner](){auto* S=Sim::GetSim();return Owner.IsValid() && S && S->GetMission();}).OnClose(OnExitMenuRequested);
    TSharedRef<SWidget> BriefingView=Objectives;
    if(auto* PC=GetOwningPlayer())
    {
        auto* Simulation=Sim::GetSim();
        Mission* Model=Briefing.IsValid()?Briefing->GetMissionPtr():Simulation?Simulation->GetMission():nullptr;
        if(Model)
        {
            Rehearsal=CreateWidget<UShipBriefingPreview>(PC);
            if(Rehearsal)
            {
                Rehearsal->UseExternalPlaybackControls();
                auto Scene=Rehearsal->TakeWidget(); Rehearsal->SetMissionScene(Model); Rehearsal->SetPreviewActive(true);
                auto Controls=SNew(SHorizontalBox);
                auto AddControl=[&](const TCHAR* Symbol,const TCHAR* Tip,void(UShipBriefingPreview::*Action)())
                {
                    Controls->AddSlot().AutoWidth().Padding(3,0)[SNew(SButton).ToolTipText(FText::FromString(Tip))
                        .Text(FText::FromString(Symbol)).OnClicked_Lambda([Owner,Action](){if(Owner.IsValid() && Owner->Rehearsal)(Owner->Rehearsal.Get()->*Action)();return FReply::Handled();})];
                };
                AddControl(TEXT("|\u25C0"),TEXT("Rewind rehearsal"),&UShipBriefingPreview::RewindRehearsal);
                AddControl(TEXT("\u25B6"),TEXT("Play rehearsal"),&UShipBriefingPreview::PlayRehearsal);
                AddControl(TEXT("\u2161"),TEXT("Pause rehearsal"),&UShipBriefingPreview::PauseRehearsal);
                AddControl(TEXT("\u25A0"),TEXT("Stop rehearsal"),&UShipBriefingPreview::StopRehearsal);
                BriefingView=SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(.45f).Padding(0,0,12,0)[Objectives]
                    + SHorizontalBox::Slot().FillWidth(.55f)[SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,6)[SNew(STextBlock).Text(INVTEXT("MISSION REHEARSAL")).Font(MissionUIStyle::GetHeaderFont(14))]
                        + SVerticalBox::Slot().FillHeight(1)[SNew(SBorder).BorderImage(&FlightPanelStyle::Window()).Padding(4)[Scene]]
                        + SVerticalBox::Slot().AutoHeight().Padding(0,6,0,0)[Controls]];
            }
        }
    }
    if(!bIncludePackage)return BriefingView;
    TSharedRef<SWidget> PackageContent=SNew(STextBlock).Text(INVTEXT("No mission package is available.")).Font(MissionUIStyle::GetRowFont(13));
    if(auto* PC=GetOwningPlayer();PC && Briefing.IsValid())
    {
        UClass* PackageClass=LoadClass<UMissionPackageDlg>(nullptr,TEXT("/Game/Screens/Mission/MissionPackagePanel.MissionPackagePanel_C"));
        if(PackageClass)
        {
            PackagePanel=CreateWidget<UMissionPackageDlg>(PC,PackageClass);
            if(PackagePanel){PackagePanel->SetParentDlg(Briefing.Get());PackageContent=PackagePanel->TakeWidget();PackagePanel->RefreshFromMission();}
        }
    }
    auto Pages=SNew(SWidgetSwitcher)+SWidgetSwitcher::Slot()[BriefingView]+SWidgetSwitcher::Slot()[PackageContent];
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)[SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)[SNew(SButton).ToolTipText(INVTEXT("Mission briefing, situation, objectives and current instructions"))
                .OnClicked_Lambda([Owner,Pages](){Pages->SetActiveWidgetIndex(0);if(Owner.IsValid() && Owner->Rehearsal)Owner->Rehearsal->SetPreviewActive(true);return FReply::Handled();})[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Computer)]]
            + SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ToolTipText(INVTEXT("Friendly mission package, selected flight route and known threats"))
                .OnClicked_Lambda([Owner,Pages](){if(Owner.IsValid() && Owner->PackagePanel)Owner->PackagePanel->RefreshFromMission();if(Owner.IsValid() && Owner->Rehearsal)Owner->Rehearsal->SetPreviewActive(false);Pages->SetActiveWidgetIndex(1);return FReply::Handled();})[SNew(SShipSystemIcon).Icon(EShipSystemIcon::MissionPackage)]]]
        + SVerticalBox::Slot().FillHeight(1)[Pages];
}
FText UObjectivesDlg::GetPanelCaption() const{return bIncludePackage?INVTEXT("MISSION"):INVTEXT("OBJECTIVES");}
void UObjectivesDlg::NativeDestruct(){if(Rehearsal)Rehearsal->SetPreviewActive(false);Super::NativeDestruct();}
