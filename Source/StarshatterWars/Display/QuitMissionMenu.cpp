#include "QuitMissionMenu.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanelSlot.h"
#include "Styling/CoreStyle.h"

UQuitMissionMenu::UQuitMissionMenu(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    SetIsFocusable(true);
}

void UQuitMissionMenu::BuildGeneratedContent()
{
    if (!WidgetTree) return;
    UVerticalBox* Container = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("VerticalBox_0")));
    if (!Container)
    {
        UE_LOG(LogTemp, Error, TEXT("[QuitMissionMenu] WBP needs VerticalBox_0 to host generated actions"));
        return;
    }
    if (!TitleText) TitleText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ExitTitle")));
    // Five readable actions, status feedback, and a slightly larger modern command frame.
    if (USizeBox* Frame = Cast<USizeBox>(WidgetTree->FindWidget(TEXT("DlgSizeBox"))))
    {
        Frame->SetHeightOverride(408.0f);
        if (UCanvasPanelSlot* FrameCanvasSlot = Cast<UCanvasPanelSlot>(Frame->Slot))
            FrameCanvasSlot->SetSize(FVector2D(580.0f, 408.0f));
    }
    if (USizeBox* Content = Cast<USizeBox>(WidgetTree->FindWidget(TEXT("SizeBox_5"))))
        Content->ClearWidthOverride();

    if (TitleText)
    {
        TitleText->SetText(FText::FromString(TEXT("MISSION CONTROL  /  PAUSED")));
        TitleText->SetJustification(ETextJustify::Left);
        TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.53f, 0.84f, 0.96f, 1.0f)));
    }

    FSlateFontInfo RowFont = TitleText ? TitleText->GetFont() : FCoreStyle::GetDefaultFontStyle("Regular", 16);
    RowFont.Size = 16;
    auto MakeButton = [&](TObjectPtr<UButton>& Button, const TCHAR* Name, const TCHAR* Label)
    {
        if (!Button) Button = Cast<UButton>(WidgetTree->FindWidget(FName(Name)));
        if (!Button)
        {
            Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name));
            if (UVerticalBoxSlot* RowSlot = Container->AddChildToVerticalBox(Button))
            {
                RowSlot->SetPadding(FMargin(12.0f, 5.0f));
                RowSlot->SetHorizontalAlignment(HAlign_Fill);
                RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            }
        }

        UTextBlock* LabelWidget = Button->GetContent() ? Cast<UTextBlock>(Button->GetContent()) : nullptr;
        if (!LabelWidget)
        {
            if (Button->GetContent()) Button->ClearChildren();
            LabelWidget = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
            Button->AddChild(LabelWidget);
        }
        LabelWidget->SetText(FText::FromString(Label));
        LabelWidget->SetFont(RowFont);
        LabelWidget->SetJustification(ETextJustify::Left);
        LabelWidget->SetColorAndOpacity(FSlateColor(FLinearColor(0.79f, 0.93f, 1.0f, 1.0f)));
        LabelWidget->SetMargin(FMargin(10.0f, 7.0f));
        LabelWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
        if (UButtonSlot* ButtonContentSlot = Cast<UButtonSlot>(LabelWidget->Slot))
        {
            ButtonContentSlot->SetHorizontalAlignment(HAlign_Fill);
            ButtonContentSlot->SetVerticalAlignment(VAlign_Center);
        }
        // Solid charcoal-blue background with a brighter blue hover state.
        FButtonStyle RowStyle = Button->GetStyle();
        RowStyle.Normal.DrawAs = ESlateBrushDrawType::Box;
        RowStyle.Hovered.DrawAs = ESlateBrushDrawType::Box;
        RowStyle.Pressed.DrawAs = ESlateBrushDrawType::Box;
        RowStyle.Normal.TintColor = FSlateColor(FLinearColor(0.055f, 0.105f, 0.16f, 0.98f));
        RowStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.09f, 0.24f, 0.36f, 1.0f));
        RowStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.035f, 0.32f, 0.49f, 1.0f));
        Button->SetStyle(RowStyle);
    };
    MakeButton(AcceptBtn, TEXT("AcceptBtn"), TEXT("1.   END MISSION  /  ACCEPT RESULTS"));
    MakeButton(AbortBtn, TEXT("AbortBtn"), TEXT("2.   ABORT MISSION  /  DISCARD RESULTS"));
    MakeButton(ResumeBtn, TEXT("ResumeBtn"), TEXT("3.   RESUME MISSION"));
    MakeButton(ControlsBtn, TEXT("ControlsBtn"), TEXT("4.   CONTROL SETUP"));
    MakeButton(ExitWindowsBtn, TEXT("ExitWindowsBtn"), TEXT("5.   QUIT AND EXIT TO WINDOWS"));
    if (!StatusText) StatusText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("StatusText")));
    if (!StatusText)
    {
        StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
        FSlateFontInfo StatusFont = RowFont;
        StatusFont.Size = 12;
        StatusText->SetFont(StatusFont);
        StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.65f, 0.25f)));
        StatusText->SetAutoWrapText(true);
        Container->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(12.0f, 6.0f));
    }
}

void UQuitMissionMenu::NativeConstruct()
{
    Super::NativeConstruct();
    BuildGeneratedContent();
    if (AcceptBtn) { AcceptBtn->OnClicked.RemoveAll(this); AcceptBtn->OnClicked.AddDynamic(this, &UQuitMissionMenu::AcceptMission); }
    if (AbortBtn) { AbortBtn->OnClicked.RemoveAll(this); AbortBtn->OnClicked.AddDynamic(this, &UQuitMissionMenu::AbortMission); }
    if (ResumeBtn) { ResumeBtn->OnClicked.RemoveAll(this); ResumeBtn->OnClicked.AddDynamic(this, &UQuitMissionMenu::ResumeMission); }
    if (ControlsBtn) { ControlsBtn->OnClicked.RemoveAll(this); ControlsBtn->OnClicked.AddDynamic(this, &UQuitMissionMenu::OpenControls); }
    if (ExitWindowsBtn) { ExitWindowsBtn->OnClicked.RemoveAll(this); ExitWindowsBtn->OnClicked.AddDynamic(this, &UQuitMissionMenu::ExitToWindows); }
    SetStatusMessage(StatusMessage);
    SetDialogInputEnabled(true);
}
void UQuitMissionMenu::NativeDestruct()
{
    if (AcceptBtn) AcceptBtn->OnClicked.RemoveAll(this);
    if (AbortBtn) AbortBtn->OnClicked.RemoveAll(this);
    if (ResumeBtn) ResumeBtn->OnClicked.RemoveAll(this);
    if (ControlsBtn) ControlsBtn->OnClicked.RemoveAll(this);
    if (ExitWindowsBtn) ExitWindowsBtn->OnClicked.RemoveAll(this);
    OnActionRequested.Unbind();
    Super::NativeDestruct();
}
void UQuitMissionMenu::AcceptMission() { OnActionRequested.ExecuteIfBound(EMissionQuitAction::Accept); }
void UQuitMissionMenu::AbortMission() { OnActionRequested.ExecuteIfBound(EMissionQuitAction::Abort); }
void UQuitMissionMenu::ResumeMission() { OnActionRequested.ExecuteIfBound(EMissionQuitAction::Resume); }
void UQuitMissionMenu::OpenControls() { OnActionRequested.ExecuteIfBound(EMissionQuitAction::Controls); }
void UQuitMissionMenu::ExitToWindows() { OnActionRequested.ExecuteIfBound(EMissionQuitAction::ExitWindows); }
void UQuitMissionMenu::HandleAccept() { ResumeMission(); }
void UQuitMissionMenu::HandleCancel() { ResumeMission(); }
void UQuitMissionMenu::SetStatusMessage(const FText& Message)
{
    StatusMessage = Message;
    if (StatusText)
    {
        StatusText->SetText(Message);
        StatusText->SetVisibility(Message.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    }
    OnStatusMessageChanged(Message);
}
FReply UQuitMissionMenu::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    const FKey Key = Event.GetKey();
    const bool bMenuKey = Key == EKeys::Escape || Key == EKeys::One || Key == EKeys::Two ||
        Key == EKeys::Three || Key == EKeys::Four || Key == EKeys::Five ||
        Key == EKeys::NumPadOne || Key == EKeys::NumPadTwo || Key == EKeys::NumPadThree ||
        Key == EKeys::NumPadFour || Key == EKeys::NumPadFive;
    if (!bMenuKey) return Super::NativeOnPreviewKeyDown(Geometry, Event);
    if (!Event.IsRepeat())
    {
        if (Key == EKeys::One || Key == EKeys::NumPadOne) AcceptMission();
        else if (Key == EKeys::Two || Key == EKeys::NumPadTwo) AbortMission();
        else if (Key == EKeys::Four || Key == EKeys::NumPadFour) OpenControls();
        else if (Key == EKeys::Five || Key == EKeys::NumPadFive) ExitToWindows();
        else ResumeMission();
    }
    return FReply::Handled();
}
