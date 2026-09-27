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
#include "SSWRuntimeSubsystem.h"
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
    UGameInstance* GI = GetGameInstance();
    if (!GI)
        return;

    USSWRuntimeSubsystem* RuntimeSS =
        GI->GetSubsystem<USSWRuntimeSubsystem>();

    if (!RuntimeSS)
        return;

    const EGameMode Mode = RuntimeSS->GetGameMode();

    // Title:
    if (TitleText)
    {
        if (Mode == EGameMode::CLOD || Mode == EGameMode::CMPN)
        {
            SetTextBlock(TitleText, "Campaign");
        }
        else if (Mode == EGameMode::MENU)
        {
            SetTextBlock(TitleText, "Tactical Reference");
        }
        else
        {
            SetTextBlock(TitleText, "Mission");
        }
    }

    // Activity:
    if (ActivityText)
    {
        SetTextBlock(ActivityText, RuntimeSS->GetLoadActivity());
    }

    // Progress:
    if (ProgressBar)
    {
        const float P =
            FMath::Clamp((float)RuntimeSS->GetLoadProgress(), 0.0f, 1.0f);

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
