#include "FlightLogbookDlg.h"
#include "PlayerDlg.h"
#include "ShipSystemIcon.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"

TSharedRef<SWidget> UFlightLogbookDlg::RebuildWidget()
{
    return BuildPanelContent(GetGameInstance());
}

TSharedRef<SWidget> UFlightLogbookDlg::BuildPanelContent(UGameInstance* Instance) const
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(8)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,6,0)
            [SNew(SButton).ToolTipText(INVTEXT("Skills"))
                .OnClicked_Lambda([Open = OnOpenSkills]() { Open.ExecuteIfBound(); return FReply::Handled(); })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Skills)]]
            + SHorizontalBox::Slot().AutoWidth()
            [SNew(SButton).ToolTipText(INVTEXT("Hangar"))
            .OnClicked_Lambda([Open = OnOpenHangar]() { Open.ExecuteIfBound(); return FReply::Handled(); })
            [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Hangar)]]
        ]
        + SVerticalBox::Slot().FillHeight(1)[UPlayerDlg::BuildFlightStatistics(Instance)];
}
