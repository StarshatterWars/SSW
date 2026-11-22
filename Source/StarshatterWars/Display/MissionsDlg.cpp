#include "MissionsDlg.h"
#include "PanelCloseIcon.h"

#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
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
            [SNew(STextBlock).Text(Title).Font(MissionUIStyle::GetHeaderFont(16)).AutoWrapText(true)]
            + SHorizontalBox::Slot().AutoWidth()
            [SNew(SButton).ContentPadding(FMargin(8)).HAlign(HAlign_Center).VAlign(VAlign_Center).ToolTipText(INVTEXT("Close panel")).ButtonColorAndOpacity(FLinearColor(.55f, .09f, .07f))
                .OnClicked_Lambda([Action = Close]() { Action.ExecuteIfBound(); return FReply::Handled(); })
                [SNew(SPanelCloseIcon)]]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)
        [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
            [SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(MetadataAgent).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true)]
                + SVerticalBox::Slot().AutoHeight().Padding(0,4)[SNew(STextBlock).Text(MetadataStatus).Font(MissionUIStyle::GetRowFont(11)).ColorAndOpacity(FLinearColor(.4f,.8f,.9f)).AutoWrapText(true)]]
            + SHorizontalBox::Slot().AutoWidth().Padding(16,0,0,0).VAlign(VAlign_Center)
            [SNew(STextBlock).Text(MetadataExpiry).Justification(ETextJustify::Right).Font(MissionUIStyle::GetRowFont(11)).ColorAndOpacity(FLinearColor(.65f,.75f,.8f))]]
        + SVerticalBox::Slot().FillHeight(1)
        [SNew(SVerticalBox)
            + SVerticalBox::Slot().FillHeight(Supplement.IsValid() ? .35f : 1.f)
                .Padding(0, 0, 0, Supplement.IsValid() ? 12 : 0)
            [SNew(SScrollBox) + SScrollBox::Slot()
                [SNew(STextBlock).Text(Brief).Font(MissionUIStyle::GetRowFont(13)).AutoWrapText(true)]]
            + SVerticalBox::Slot().FillHeight(Supplement.IsValid() ? .65f : 0.f)
            [Supplement.IsValid() ? Supplement.ToSharedRef() : SNullWidget::NullWidget]]
        + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
        [SNew(STextBlock)
            .Text_Lambda([this]() { return LaunchFeedback; })
            .Visibility_Lambda([this]() { return LaunchFeedback.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
            .Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true)
            .ColorAndOpacity(FLinearColor(.4f, .85f, .95f))]
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
            + SHorizontalBox::Slot().AutoWidth().Padding(12, 0, 0, 0)
            [SNew(SButton).ContentPadding(FMargin(16,8)).ButtonColorAndOpacity(FLinearColor(.05f,.28f,.38f))
                .Visibility(StartMission.IsBound() ? EVisibility::Visible : EVisibility::Collapsed)
                .IsEnabled_Lambda([this](){ return CanStartMission.IsBound() && CanStartMission.Execute(); })
                .ToolTipText(INVTEXT("Start this mission"))
                .OnClicked_Lambda([Action=StartMission](){ Action.ExecuteIfBound(); return FReply::Handled(); })
                [SNew(SHorizontalBox) + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Play)] + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6,0)[SNew(STextBlock).Text(INVTEXT("MISSION")).Font(MissionUIStyle::GetRowFont(14))]]]
        ]);
}

void UMissionsDlg::NativeDestruct()
{
    // Drop the nested UUserWidget's SObjectWidget while its UObject is alive.
    Supplement.Reset();
    Super::NativeDestruct();
}
void UMissionsDlg::ReleaseSlateResources(bool bReleaseChildren)
{
    Supplement.Reset();
    Super::ReleaseSlateResources(bReleaseChildren);
}
