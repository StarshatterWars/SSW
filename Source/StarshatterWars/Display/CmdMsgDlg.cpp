#include "CmdMsgDlg.h"

#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

UCmdMsgDlg::UCmdMsgDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCmdMsgDlg::NativeConstruct()
{
    Super::NativeConstruct();

    bExitLatch = false;
    bWantsFocus = false;

    SetIsFocusable(true);
    HideMsgDlg();
}

void UCmdMsgDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (GetVisibility() == ESlateVisibility::Visible)
    {
        UpdateFocusIfVisible();
        HandleKeyboardShortcuts();
    }
}

FReply UCmdMsgDlg::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (GetVisibility() == ESlateVisibility::Visible &&
        InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        HideMsgDlg();
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UCmdMsgDlg::ShowMsgDlg()
{
    UE_LOG(LogTemp, Warning, TEXT("[CmdMsgDlg] ShowMsgDlg called"));
    UE_LOG(LogTemp, Warning, TEXT("[CmdMsgDlg] Title=%p MessageRich=%p Message=%p"),
        TitleText, MessageTextBlock, MessageText);

    SetVisibility(ESlateVisibility::Visible);
    bWantsFocus = true;
    bExitLatch = false;
}

void UCmdMsgDlg::HideMsgDlg()
{
    SetVisibility(ESlateVisibility::Hidden);
    bWantsFocus = false;
    bExitLatch = false;
}

void UCmdMsgDlg::SetTitleText(const FString& InTitle)
{
    if (TitleText)
    {
        TitleText->SetText(FText::FromString(InTitle));
    }
}

void UCmdMsgDlg::SetMessageText(const FString& InMessage)
{
    if (MessageTextBlock)
    {
        MessageTextBlock->SetText(FText::FromString(InMessage));
    }
    else if (MessageText)
    {
        MessageText->SetText(FText::FromString(InMessage));
    }
}

void UCmdMsgDlg::UpdateFocusIfVisible()
{
    if (!bWantsFocus)
    {
        return;
    }

    UWidgetBlueprintLibrary::SetFocusToGameViewport();
    SetKeyboardFocus();
    bWantsFocus = false;
}

void UCmdMsgDlg::HandleKeyboardShortcuts()
{
    const APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC)
    {
        return;
    }

    if (PC->WasInputKeyJustPressed(EKeys::Enter) ||
        PC->WasInputKeyJustPressed(EKeys::Virtual_Accept))
    {
        HideMsgDlg();
        return;
    }

    const bool bEscapeDown = PC->IsInputKeyDown(EKeys::Escape);
    if (bEscapeDown)
    {
        if (!bExitLatch)
        {
            HideMsgDlg();
        }

        bExitLatch = true;
    }
    else
    {
        bExitLatch = false;
    }
}