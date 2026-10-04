#include "QuitMissionMenu.h"
#include "Components/Button.h"
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
    // Make room for four rows and a wrapped validation message inside the frame.
    if (USizeBox* Frame = Cast<USizeBox>(WidgetTree->FindWidget(TEXT("DlgSizeBox"))))
    {
        Frame->SetHeightOverride(320.0f);
        if (UCanvasPanelSlot* CSlot = Cast<UCanvasPanelSlot>(Frame->Slot))
            CSlot->SetSize(FVector2D(512.0f, 320.0f));
    }
    if (USizeBox* Content = Cast<USizeBox>(WidgetTree->FindWidget(TEXT("SizeBox_5"))))
        Content->ClearWidthOverride();

    FSlateFontInfo RowFont = TitleText ? TitleText->GetFont() : FCoreStyle::GetDefaultFontStyle("Regular", 16);
    RowFont.Size = 16;
    auto MakeButton = [&](TObjectPtr<UButton>& Button, const TCHAR* Name, const TCHAR* Label)
    {
        // Reuse generated widgets when NativeConstruct runs again.
        if (!Button) Button = Cast<UButton>(WidgetTree->FindWidget(FName(Name)));
        if (Button) return;
        Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name));

        UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Text->SetText(FText::FromString(Label));
        Text->SetFont(RowFont);
        Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.04f, 0.04f, 0.04f, 1.0f)));
        Text->SetMargin(FMargin(8.0f, 4.0f));
        Text->SetVisibility(ESlateVisibility::HitTestInvisible);
        Button->AddChild(Text);
        UVerticalBoxSlot* Slot = Container->AddChildToVerticalBox(Button);
        Slot->SetPadding(FMargin(4.0f, 3.0f));
        Slot->SetHorizontalAlignment(HAlign_Fill);
        Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    };
    MakeButton(AcceptBtn, TEXT("AcceptBtn"), TEXT("1. End Mission and Accept Results"));
    MakeButton(AbortBtn, TEXT("AbortBtn"), TEXT("2. Abort and Discard Mission"));
    MakeButton(ResumeBtn, TEXT("ResumeBtn"), TEXT("3. Resume Current Mission"));
    MakeButton(ControlsBtn, TEXT("ControlsBtn"), TEXT("4. Control Setup"));
    if (!StatusText) StatusText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("StatusText")));
    if (!StatusText)
    {
        StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
        FSlateFontInfo StatusFont = RowFont;
        StatusFont.Size = 12;
        StatusText->SetFont(StatusFont);
        StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.65f, 0.25f)));
        StatusText->SetAutoWrapText(true);
        Container->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(8.0f, 4.0f));
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
    // Keep the title authored in WBP_QuitMissionMenu (ExitTitle or TitleText).
    SetStatusMessage(StatusMessage);
    SetDialogInputEnabled(true);
}
void UQuitMissionMenu::NativeDestruct()
{
    if (AcceptBtn) AcceptBtn->OnClicked.RemoveAll(this);
    if (AbortBtn) AbortBtn->OnClicked.RemoveAll(this);
    if (ResumeBtn) ResumeBtn->OnClicked.RemoveAll(this);
    if (ControlsBtn) ControlsBtn->OnClicked.RemoveAll(this);
    OnActionRequested.Unbind();
    Super::NativeDestruct();
}
void UQuitMissionMenu::AcceptMission() { OnActionRequested.ExecuteIfBound(EMissionQuitAction::Accept); }
void UQuitMissionMenu::AbortMission() { OnActionRequested.ExecuteIfBound(EMissionQuitAction::Abort); }
void UQuitMissionMenu::ResumeMission() { OnActionRequested.ExecuteIfBound(EMissionQuitAction::Resume); }
void UQuitMissionMenu::OpenControls() { OnActionRequested.ExecuteIfBound(EMissionQuitAction::Controls); }
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
    const bool bMenuKey = Key == EKeys::Escape || Key == EKeys::One || Key == EKeys::Two || Key == EKeys::Three || Key == EKeys::Four;
    if (!bMenuKey) return Super::NativeOnPreviewKeyDown(Geometry, Event);
    if (!Event.IsRepeat())
    {
        if (Key == EKeys::One) AcceptMission();
        else if (Key == EKeys::Two) AbortMission();
        else if (Key == EKeys::Four) OpenControls();
        else ResumeMission();
    }
    return FReply::Handled();
}
