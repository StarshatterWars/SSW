#include "FlightTacticalReferenceDlg.h"
#include "CreditsContent.h"
#include "MissionUIStyle.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ForcesDlg.h"
#include "ShipSystemIcon.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "FlightReferencePanel.h"
#include "ShipBriefingPreview.h"
#include "SystemMapPanel.h"
#include "Blueprint/WidgetTree.h"

TSharedRef<SWidget> UFlightTacticalReferenceDlg::RebuildWidget()
{
    SetIsFocusable(true);
    Preview = CreateWidget<UShipBriefingPreview>(GetOwningPlayer());
    if (!Preview) return SNew(SFlightReferencePanel);

    Preview->SetReferencePreview();
    GazetteerMap = WidgetTree->ConstructWidget<USystemMapPanel>();
    GazetteerMap->EnableMissionBodyMeshes();
    auto Reference=SNew(SFlightReferencePanel)
        .GameInstance(GetGameInstance())
        .GazetteerMap(GazetteerMap->TakeWidget())
        .UpdateGazetteerMap([Map = TWeakObjectPtr<USystemMapPanel>(GazetteerMap)](const FString& SystemName)
        {
            if (!SystemName.IsEmpty())
                if (auto* View = Map.Get()) View->SetViewedSystemName(SystemName);
        })
        .PreviewWidget(Preview->TakeWidget())
        .UpdatePreview([Widget = TWeakObjectPtr<UShipBriefingPreview>(Preview)](const FShipDesign* Design)
        {
            if (auto* View = Widget.Get())
            {
                if (Design) View->SetShipDesign(Design);
                View->SetPreviewActive(Design != nullptr);
            }
        });
    Forces=CreateWidget<UForcesDlg>(GetOwningPlayer());
    auto Credits=MissionUIStyle::MakeDataPane(SNew(SScrollBox)+SScrollBox::Slot()
        [SNew(STextBlock).Text(FText::FromString(CreditsContent::Load())).Font(MissionUIStyle::GetRowFont(13)).AutoWrapText(true)]);
    auto Pages=SNew(SWidgetSwitcher)+SWidgetSwitcher::Slot()[Reference]+SWidgetSwitcher::Slot()[Forces ? Forces->TakeWidget() : SNullWidget::NullWidget]+SWidgetSwitcher::Slot()[Credits];
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(8)
        [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,6,0)
            [SNew(SButton).ToolTipText(INVTEXT("Tactical Reference"))
                .OnClicked_Lambda([Weak=TWeakPtr<SWidgetSwitcher>(Pages),Ref=TWeakPtr<SFlightReferencePanel>(Reference)](){if(auto View=Weak.Pin())View->SetActiveWidgetIndex(0);if(auto Panel=Ref.Pin())Panel->SetReferenceActive(true);return FReply::Handled();})
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Reference)]]
            + SHorizontalBox::Slot().AutoWidth()
            [SNew(SButton).ToolTipText(INVTEXT("Forces — sector, system and neighboring-system reports"))
                .OnClicked_Lambda([Weak=TWeakPtr<SWidgetSwitcher>(Pages),Preview=TWeakObjectPtr<UShipBriefingPreview>(Preview)]()
                    {if(auto View=Weak.Pin())View->SetActiveWidgetIndex(1);if(Preview.IsValid())Preview->SetPreviewActive(false);return FReply::Handled();})
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Ship)]]
            + SHorizontalBox::Slot().AutoWidth().Padding(6,0,0,0)
            [SNew(SButton).ToolTipText(INVTEXT("Credits"))
                .OnClicked_Lambda([Weak=TWeakPtr<SWidgetSwitcher>(Pages),Preview=TWeakObjectPtr<UShipBriefingPreview>(Preview)]()
                    {if(auto View=Weak.Pin())View->SetActiveWidgetIndex(2);if(Preview.IsValid())Preview->SetPreviewActive(false);return FReply::Handled();})
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Agent)]]]
        + SVerticalBox::Slot().FillHeight(1)[Pages];
}
