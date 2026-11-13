#include "QuitMissionMenu.h"
#include "MissionUIStyle.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
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
        {
            FrameCanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
            FrameCanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
            FrameCanvasSlot->SetPosition(FVector2D::ZeroVector);
            FrameCanvasSlot->SetSize(FVector2D(580.0f, 408.0f));
        }
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
    RowFont.Size = 15;
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
        LabelWidget->SetMargin(FMargin(4.0f, 7.0f));
        LabelWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
        if (UButtonSlot* ButtonContentSlot = Cast<UButtonSlot>(LabelWidget->Slot))
        {
            ButtonContentSlot->SetHorizontalAlignment(HAlign_Fill);
            ButtonContentSlot->SetVerticalAlignment(VAlign_Center);
        }
        MissionUIStyle::ApplyMenuButtonStyle(Button, GetGameInstance());
        const bool bDestructive = FCString::Strcmp(Name, TEXT("AbortBtn")) == 0 || FCString::Strcmp(Name, TEXT("ExitWindowsBtn")) == 0;
        const bool bControls = FCString::Strcmp(Name, TEXT("ControlsBtn")) == 0;
        MissionUIStyle::ApplyActionButtonStyle(Button, bDestructive ? TEXT("Red") : (bControls ? TEXT("Blue") : TEXT("Green")));
    };
    MakeButton(AcceptBtn, TEXT("AcceptBtn"), TEXT("1.   ACCEPT MISSION RESULTS"));
    MakeButton(AbortBtn, TEXT("AbortBtn"), TEXT("2.   ABORT MISSION"));
    MakeButton(ResumeBtn, TEXT("ResumeBtn"), TEXT("3.   RESUME MISSION"));
    MakeButton(ControlsBtn, TEXT("ControlsBtn"), TEXT("4.   CONTROL SETUP"));
    UHorizontalBox* Footer = Cast<UHorizontalBox>(WidgetTree->FindWidget(TEXT("QuitFooter")));
    if (!Footer) Footer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("QuitFooter"));

    const auto MakeIconButton = [&](TObjectPtr<UButton>& Button, const TCHAR* Name, const TCHAR* TexturePath, const FText& Tooltip)
    {
        if (!Button) Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name));
        Button->RemoveFromParent();
        auto* IconSlot = Footer->AddChildToHorizontalBox(Button);
        IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        IconSlot->SetPadding(FMargin(2.0f, 0.0f));
        Button->ClearChildren();
        auto* Icon = WidgetTree->ConstructWidget<UImage>();
        Icon->SetBrushFromTexture(LoadObject<UTexture2D>(nullptr, TexturePath));
        auto* IconFrame = WidgetTree->ConstructWidget<USizeBox>();
        IconFrame->SetWidthOverride(22.0f);
        IconFrame->SetHeightOverride(22.0f);
        IconFrame->AddChild(Icon);
        auto* ContentSlot = CastChecked<UButtonSlot>(Button->AddChild(IconFrame));
        ContentSlot->SetHorizontalAlignment(HAlign_Center);
        ContentSlot->SetVerticalAlignment(VAlign_Center);
        Button->SetToolTipText(Tooltip);
        MissionUIStyle::ApplyActionButtonStyle(Button, TEXT("Red"));
    };
    MakeIconButton(MainMenuBtn, TEXT("MainMenuBtn"),
        TEXT("/Game/SciFiUIComponents/Textures/Icons4/T_ICON_Menu.T_ICON_Menu"),
        NSLOCTEXT("QuitMissionMenu", "MainMenuTooltip", "Return to Main Menu and discard mission results"));
    MakeIconButton(ExitWindowsBtn, TEXT("ExitWindowsBtn"),
        TEXT("/Game/SciFiUIComponents/Textures/Icons4/T_Icon_Exit1.T_Icon_Exit1"),
        NSLOCTEXT("QuitMissionMenu", "ExitTooltip", "Exit to Windows"));
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
    Footer->RemoveFromParent();
    auto* FooterSlot = Container->AddChildToVerticalBox(Footer);
    FooterSlot->SetPadding(FMargin(10.0f, 5.0f));
    FooterSlot->SetHorizontalAlignment(HAlign_Fill);
    FooterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    FooterSlot->SetVerticalAlignment(VAlign_Bottom);
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
    if (MainMenuBtn) MainMenuBtn->OnClicked.AddUniqueDynamic(this, &UQuitMissionMenu::ExitToMainMenu);
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
void UQuitMissionMenu::ExitToMainMenu() { OnActionRequested.ExecuteIfBound(EMissionQuitAction::MainMenu); }
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
FReply UQuitMissionMenu::HandleLocalMenuKey(const FKeyEvent& Event)
{
    if (!IsDialogInputEnabled() || (GetVisibility() == ESlateVisibility::Hidden || GetVisibility() == ESlateVisibility::Collapsed) || !GetIsEnabled()) return FReply::Unhandled();
    const FKey Key = Event.GetKey();
    const bool bMenuKey = Key == EKeys::Escape || Key == EKeys::One || Key == EKeys::Two ||
        Key == EKeys::Three || Key == EKeys::Four || Key == EKeys::Five ||
        Key == EKeys::NumPadOne || Key == EKeys::NumPadTwo || Key == EKeys::NumPadThree ||
        Key == EKeys::NumPadFour || Key == EKeys::NumPadFive;
    if (!bMenuKey) return FReply::Unhandled();
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

FReply UQuitMissionMenu::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    const FReply Reply=HandleLocalMenuKey(Event);
    return Reply.IsEventHandled()?Reply:Super::NativeOnPreviewKeyDown(Geometry,Event);
}
FReply UQuitMissionMenu::NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    // The focused dialog itself receives the bubble event. Preview is needed
    // separately when focus moves into one of its buttons.
    const FReply Reply=HandleLocalMenuKey(Event);
    return Reply.IsEventHandled()?Reply:Super::NativeOnKeyDown(Geometry,Event);
}
