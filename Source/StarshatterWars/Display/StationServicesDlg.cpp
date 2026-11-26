#include "StationServicesDlg.h"
#include "StationServicesAI.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
EShipSystemIcon ServiceIcon(ESSWStationService Service)
{
    switch(Service)
    {
    case ESSWStationService::Cargo:return EShipSystemIcon::Cargo;
    case ESSWStationService::Market:return EShipSystemIcon::Marketplace;
    case ESSWStationService::Insurance:return EShipSystemIcon::Insurance;
    case ESSWStationService::Repair:return EShipSystemIcon::Repair;
    case ESSWStationService::Loadout:return EShipSystemIcon::Loadout;
    case ESSWStationService::HullSelection:return EShipSystemIcon::Hangar;
    case ESSWStationService::Refit:return EShipSystemIcon::SettingsGear;
    case ESSWStationService::Manufacturing:return EShipSystemIcon::Production;
    case ESSWStationService::Blueprints:return EShipSystemIcon::Reference;
    default:return EShipSystemIcon::Skills;
    }
}
}
TSharedRef<SWidget> UStationServicesDlg::RebuildWidget()
{
    const TWeakObjectPtr<UStationServicesDlg> Weak(this);
    const auto Rows=SNew(SVerticalBox);
    const auto Feedback=MakeShared<FText>();
    for(const auto& Descriptor:StationServicesAI::Descriptors())
    {
        const auto Service=Descriptor.Service;
        Rows->AddSlot().AutoHeight().Padding(0,3)
        [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1)
            [SNew(SButton).HAlign(HAlign_Fill).ContentPadding(FMargin(10,8))
                .ButtonColorAndOpacity(MissionUIStyle::RowBG)
                .IsEnabled_Lambda([Weak,Service]()
                {FText Reason;return Weak.IsValid() && (Service==ESSWStationService::Repair ? StationServicesAI::CanRepair(Weak->GetGameInstance(),Reason,Weak->GetStation()) : Weak->OnOpenService.IsBound() && StationServicesAI::CanUse(StationServicesAI::Context(Weak.IsValid()?Weak->GetStation():NAME_None),Service,Reason));})
                .ToolTipText_Lambda([Weak,Service,Explanation=FString(Descriptor.Explanation)]()
                {FText Reason;if(Service==ESSWStationService::Repair && Weak.IsValid()){StationServicesAI::CanRepair(Weak->GetGameInstance(),Reason,Weak->GetStation());return Reason;}return StationServicesAI::CanUse(StationServicesAI::Context(Weak.IsValid()?Weak->GetStation():NAME_None),Service,Reason)?FText::FromString(Explanation):Reason;})
                .OnClicked_Lambda([Weak,Service,Feedback]()
                {FText Reason;if(Weak.IsValid()){if(Service==ESSWStationService::Repair)StationServicesAI::ExecuteRepair(Weak->GetGameInstance(),*Feedback,Weak->GetStation(),[Feedback](bool,const FText& Why,FGuid){*Feedback=Why;});else if(StationServicesAI::CanUse(StationServicesAI::Context(Weak.IsValid()?Weak->GetStation():NAME_None),Service,Reason))Weak->OnOpenService.ExecuteIfBound(Service);}return FReply::Handled();})
                [SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,12,0)[SNew(SShipSystemIcon).Icon(ServiceIcon(Service))]
                    + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                    [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Text(FText::FromString(Descriptor.Label))]]]
            + SHorizontalBox::Slot().FillWidth(1.7f).VAlign(VAlign_Center).Padding(12,0)
            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(11)).AutoWrapText(true)
                .Text_Lambda([Weak,Service]()
                {const auto Context=StationServicesAI::Context(Weak.IsValid()?Weak->GetStation():NAME_None);
                    if(!FStationServiceCatalog::Get().HasService(Context.Design,Context.Facility,Service))return INVTEXT("NOT PROVIDED HERE");
                    FText Reason;if(Service==ESSWStationService::Repair && Weak.IsValid()){StationServicesAI::CanRepair(Weak->GetGameInstance(),Reason,Weak->GetStation());return Reason.ToUpper();}
                    return StationServicesAI::CanUse(Context,Service,Reason)?INVTEXT("AVAILABLE"):Reason.ToUpper();})]];
    }
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(10,10,10,12)
        [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true)
            .Text_Lambda([Weak]()
            {
                const auto Context=StationServicesAI::Context(Weak.IsValid()?Weak->GetStation():NAME_None);
                if(Context.Name.IsEmpty())return INVTEXT("SELECT AN NPC STATION OR CARRIER TO VIEW ITS SERVICES");
                return FText::FromString(FString::Printf(TEXT("%s  |  %s  |  %s\n%s"),*Context.Name,*Context.System,*Context.Region,
                    Context.bDocked?TEXT("DOCKED - AVAILABLE SERVICES BELOW"):TEXT("READ ONLY - DOCK HERE TO USE SERVICES"))).ToUpper();
            })]
        + SVerticalBox::Slot().FillHeight(1)
        [MissionUIStyle::MakeDataPane(SNew(SScrollBox)+SScrollBox::Slot()[Rows])]
        + SVerticalBox::Slot().AutoHeight().Padding(10,8)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(11)).AutoWrapText(true).Text_Lambda([Feedback](){return Feedback->ToUpper();})];
}
