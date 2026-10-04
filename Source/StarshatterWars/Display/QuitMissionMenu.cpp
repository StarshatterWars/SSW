#include "QuitMissionMenu.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"

UQuitMissionMenu::UQuitMissionMenu(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    SetIsFocusable(true);
}
void UQuitMissionMenu::NativeConstruct()
{
    Super::NativeConstruct();
    if (AcceptBtn) { AcceptBtn->OnClicked.RemoveAll(this); AcceptBtn->OnClicked.AddDynamic(this, &UQuitMissionMenu::AcceptMission); }
    if (AbortBtn) { AbortBtn->OnClicked.RemoveAll(this); AbortBtn->OnClicked.AddDynamic(this, &UQuitMissionMenu::AbortMission); }
    if (ResumeBtn) { ResumeBtn->OnClicked.RemoveAll(this); ResumeBtn->OnClicked.AddDynamic(this, &UQuitMissionMenu::ResumeMission); }
    if (ControlsBtn) { ControlsBtn->OnClicked.RemoveAll(this); ControlsBtn->OnClicked.AddDynamic(this, &UQuitMissionMenu::OpenControls); }
    if (TitleText) TitleText->SetText(FText::FromString(TEXT("SIMULATION MENU")));
    if (StatusText) StatusText->SetText(StatusMessage);
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
    if (StatusText) StatusText->SetText(Message);
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
