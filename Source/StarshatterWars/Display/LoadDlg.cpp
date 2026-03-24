/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         LoadDlg.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Loading progress dialog (legacy LoadDlg) implementation for Unreal UMG.
*/

#include "LoadDlg.h"

// Unreal
#include "Logging/LogMacros.h"

// UMG
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

// Starshatter
#include "Starshatter.h"
#include "Game.h"

DEFINE_LOG_CATEGORY_STATIC(LogLoadDlg, Log, All);

// --------------------------------------------------------------------

ULoadDlg::ULoadDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

// --------------------------------------------------------------------

void ULoadDlg::NativeConstruct()
{
    Super::NativeConstruct();

    // Mirror legacy behavior: controls are discovered via BindWidget.
    RegisterControls();

    // First paint:
    ExecFrame();
}

// --------------------------------------------------------------------

void ULoadDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ExecFrame();
}

// --------------------------------------------------------------------
// Legacy parity stubs (UMG uses BindWidget, so this is largely semantic)
// --------------------------------------------------------------------

void ULoadDlg::RegisterControls()
{
    // No-op in UMG (TitleText/ActivityText/ProgressBar are bound by name),
    // but kept for parity with the original class.
}

// --------------------------------------------------------------------

void ULoadDlg::ExecFrame()
{
    Starshatter* stars = Starshatter::GetInstance();
    if (!stars)
        return;

    // Title:
    if (TitleText)
    {
        if (stars->GetGameMode() == EGameMode::CLOD ||
            stars->GetGameMode() == EGameMode::CMPN)
        {
            SetTextBlock(TitleText, Game::GetText("LoadDlg.campaign"));
        }
        else if (stars->GetGameMode() == EGameMode::MENU)
        {
            SetTextBlock(TitleText, Game::GetText("LoadDlg.tac-ref"));
        }
        else
        {
            SetTextBlock(TitleText, Game::GetText("LoadDlg.mission"));
        }
    }

    // Activity:
    if (ActivityText)
    {
        SetTextBlock(ActivityText, stars->GetLoadActivity());
    }

    // Progress:
    if (ProgressBar)
    {
        // Legacy slider likely expects 0..1; keep it clamped either way:
        const float P = FMath::Clamp((float)stars->GetLoadProgress(), 0.0f, 1.0f);
        ProgressBar->SetPercent(P);
    }
}

// --------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------

void ULoadDlg::SetTextBlock(UTextBlock* Block, const char* AnsiText)
{
    if (!Block)
        return;

    if (!AnsiText)
        AnsiText = "";

    Block->SetText(FText::FromString(UTF8_TO_TCHAR(AnsiText)));
}
