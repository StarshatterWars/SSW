/*=============================================================================
    Project:        Starshatter Wars (Unreal Port)
    Studio:         Fractal Dev Studios
    FILE:           ExitDlg.cpp
    AUTHOR:         Carlos Bott
=============================================================================*/

#include "ExitDlg.h"

// UMG
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/RichTextBlock.h"

// UE
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Kismet/KismetSystemLibrary.h"

// Router
#include "MenuScreen.h"

DEFINE_LOG_CATEGORY_STATIC(LogExitDlg, Log, All);

/* --------------------------------------------------------------------
   Construction
   -------------------------------------------------------------------- */

UExitDlg::UExitDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(true);
}

/* --------------------------------------------------------------------
   NativeConstruct
   -------------------------------------------------------------------- */

void UExitDlg::NativeConstruct()
{
    Super::NativeConstruct();

    // Avoid duplicate delegate binding if widget rebuilds:
    if (ExitBtn)
    {
        ExitBtn->OnClicked.RemoveAll(this);
        ExitBtn->OnClicked.AddDynamic(this, &UExitDlg::OnExitClicked);
    }
    else
    {
        UE_LOG(LogExitDlg, Warning, TEXT("ExitBtn is null (BindWidgetOptional mismatch or not IsVariable)."));
    }

    if (CancelBtn)
    {
        CancelBtn->OnClicked.RemoveAll(this);
        CancelBtn->OnClicked.AddDynamic(this, &UExitDlg::OnCancelClicked);
    }
    else
    {
        UE_LOG(LogExitDlg, Warning, TEXT("CancelBtn is null (BindWidgetOptional mismatch or not IsVariable)."));
    }

    // Credits
    bExitLatch = true;
    ScrollOffset = 0.0f;

    FString Credits;
    if (LoadCreditsFile(Credits))
    {
        ApplyCredits(Credits);
    }
}

/* --------------------------------------------------------------------
   NativeTick
   -------------------------------------------------------------------- */

void UExitDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    TickCredits(InDeltaTime);
}

/* --------------------------------------------------------------------
   Input handling
   -------------------------------------------------------------------- */

void UExitDlg::HandleAccept()
{
    Super::HandleAccept();
    OnExitClicked();
}

void UExitDlg::HandleCancel()
{
    // Legacy latch: swallow the very first cancel after opening
    if (bExitLatch)
    {
        bExitLatch = false;
        return;
    }

    Super::HandleCancel();
    OnCancelClicked();
}

/* --------------------------------------------------------------------
   Button handlers
   -------------------------------------------------------------------- */

void UExitDlg::OnExitClicked()
{
    UE_LOG(LogExitDlg, Log, TEXT("Exit confirmed."));

    // MenuScreen owns modal state + Z-order; we just request exit:
    UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, true);
}

void UExitDlg::OnCancelClicked()
{
    UE_LOG(LogExitDlg, Log, TEXT("Exit canceled. Returning to menu."));

    // Hide ourselves so we don't eat input:
    SetVisibility(ESlateVisibility::Collapsed);
    SetDialogInputEnabled(false);

    if (MenuManager)
    {
        MenuManager->ShowMenuDlg();
    }
    else
    {
        UE_LOG(LogExitDlg, Warning, TEXT("MenuManager is null (MenuScreen didn't assign it)."));
    }
}

/* --------------------------------------------------------------------
   Credits helpers
   -------------------------------------------------------------------- */

bool UExitDlg::LoadCreditsFile(FString& OutText) const
{
    const TArray<FString> Paths =
    {
        FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/credits.txt")),
        FPaths::Combine(FPaths::ProjectDir(),        TEXT("Data/credits.txt")),
    };

    for (const FString& Path : Paths)
    {
        if (FPaths::FileExists(Path) && FFileHelper::LoadFileToString(OutText, *Path))
        {
            return true;
        }
    }

    return false;
}

void UExitDlg::ApplyCredits(const FString& Text)
{
    if (CreditsText)
    {
        CreditsText->SetText(FText::FromString(Text));
    }

    if (CreditsScroll)
    {
        CreditsScroll->SetScrollOffset(0.0f);
    }
}

void UExitDlg::TickCredits(float DeltaTime)
{
    if (!CreditsScroll || !IsVisible())
        return;

    ScrollOffset += ScrollPixelsPerSecond * DeltaTime;
    CreditsScroll->SetScrollOffset(ScrollOffset);

#if ENGINE_MAJOR_VERSION >= 5
    const float End = CreditsScroll->GetScrollOffsetOfEnd();
    if (End > 0.0f && ScrollOffset >= End)
    {
        ScrollOffset = 0.0f;
        CreditsScroll->SetScrollOffset(0.0f);
    }
#endif
}
