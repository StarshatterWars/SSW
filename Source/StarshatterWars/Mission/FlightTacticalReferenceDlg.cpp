#include "FlightTacticalReferenceDlg.h"
#include "FlightReferencePanel.h"
#include "ShipBriefingPreview.h"
#include "SystemMapPanel.h"
#include "Blueprint/WidgetTree.h"

TSharedRef<SWidget> UFlightTacticalReferenceDlg::RebuildWidget()
{
    SetIsFocusable(true);
    Preview = CreateWidget<UShipBriefingPreview>(GetOwningPlayer());
    if (!Preview) return SNew(SFlightReferencePanel);

    GazetteerMap = WidgetTree->ConstructWidget<USystemMapPanel>();
    return SNew(SFlightReferencePanel)
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
                View->SetPreviewActive(Design != nullptr);
                if (Design) View->SetShipDesign(Design);
            }
        });
}
