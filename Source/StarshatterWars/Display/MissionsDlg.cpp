#include "MissionsDlg.h"

#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void UMissionsDlg::SetDetails(FText InTitle, FText InBrief, FSimpleDelegate InAccept, FSimpleDelegate InDecline, FSimpleDelegate InClose)
{
    Title = MoveTemp(InTitle);
    Brief = MoveTemp(InBrief);
    Accept = MoveTemp(InAccept);
    Decline = MoveTemp(InDecline);
    Close = MoveTemp(InClose);
}

TSharedRef<SWidget> UMissionsDlg::RebuildWidget()
{
    return MissionUIStyle::MakeDataPane(
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1)
            [SNew(STextBlock).Text(Title).Font(MissionUIStyle::GetHeaderFont(17)).AutoWrapText(true)]
            + SHorizontalBox::Slot().AutoWidth()
            [SNew(SButton).Text(INVTEXT("×")).ButtonColorAndOpacity(FLinearColor(.55f, .09f, .07f))
                .OnClicked_Lambda([Action = Close]() { Action.ExecuteIfBound(); return FReply::Handled(); })]
        ]
        + SVerticalBox::Slot().FillHeight(1)
        [SNew(SScrollBox) + SScrollBox::Slot()
            [SNew(STextBlock).Text(Brief).Font(MissionUIStyle::GetRowFont(14)).AutoWrapText(true)]]
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0, 10, 0, 0)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
            [SNew(SButton).ToolTipText(INVTEXT("Decline mission"))
                .Visibility(Decline.IsBound() ? EVisibility::Visible : EVisibility::Collapsed)
                .OnClicked_Lambda([Action = Decline]() { Action.ExecuteIfBound(); return FReply::Handled(); })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Abort)]]
            + SHorizontalBox::Slot().AutoWidth()
            [SNew(SButton).ToolTipText(INVTEXT("Accept mission and add it to the queue"))
                .Visibility(Accept.IsBound() ? EVisibility::Visible : EVisibility::Collapsed)
                .OnClicked_Lambda([Action = Accept]() { Action.ExecuteIfBound(); return FReply::Handled(); })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Accept)]]
        ]);
}
