/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         LoadScreen.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    ULoadScreen
    - UMG load screen widget inheriting from UBaseScreen.
    - Controls which loading dialog is shown based on current game mode.
*/

#include "LoadScreen.h"
#include "GameStructs.h"

// Child widgets:
#include "LoadDlg.h"
#include "CmpLoadDlg.h"

// Starshatter:
#include "Starshatter.h"
#include "Game.h"
#include "DataLoader.h"

// Unreal:
#include "CoreMinimal.h"

// +--------------------------------------------------------------------+

ULoadScreen::ULoadScreen(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

// +--------------------------------------------------------------------+

void ULoadScreen::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // Optional: if LoadDlg / CmpLoadDlg are not designer-bound, log once.
    if (!LoadDlg && !CmpLoadDlg)
    {
        UE_LOG(LogTemp, Verbose, TEXT("ULoadScreen::NativeOnInitialized: No child dialogs bound (LoadDlg/CmpLoadDlg)."));
    }
}

// +--------------------------------------------------------------------+

void ULoadScreen::Setup(Screen* InScreen)
{
    if (!InScreen)
        return;

    ScreenPtr = InScreen;

    DataLoader* Loader = DataLoader::GetLoader();
    if (!Loader)
    {
        UE_LOG(LogTemp, Warning, TEXT("ULoadScreen::Setup: DataLoader::GetLoader returned null."));
        return;
    }

    // Match original Starshatter behavior:
    Loader->UseFileSystem(true);

    // In the UMG version, LoadDlg / CmpLoadDlg are expected to be created
    // via widget tree (BindWidgetOptional) or by the screen that owns this widget.
    // Therefore, we do NOT "new" them here.

    Loader->UseFileSystem(Starshatter::UseFileSystem());
    ShowLoadDlg();
}

// +--------------------------------------------------------------------+

void ULoadScreen::TearDown()
{
    // In UMG, widgets are owned by the widget tree / GC.
    // We only clear references and ensure they are hidden.

    HideLoadDlg();

    LoadDlg = nullptr;
    CmpLoadDlg = nullptr;
    ScreenPtr = nullptr;

    bIsShown = false;
}

// +--------------------------------------------------------------------+

void ULoadScreen::ExecFrame()
{
    // Starshatter core used Color::Black; Unreal port uses FColor::Black.
    Game::SetScreenColor(FColor::Black);

    // If your dialog widgets implement their own per-frame logic, call it.
    if (LoadDlg && LoadDlg->IsVisible())
    {
        // Optional: only if your ULoadDlg class has ExecFrame()
        LoadDlg->ExecFrame();
    }

    if (CmpLoadDlg && CmpLoadDlg->IsVisible())
    {
        // Optional: only if your UCmpLoadDlg class has ExecFrame()
        CmpLoadDlg->ExecFrame();
    }
}

// +--------------------------------------------------------------------+

bool ULoadScreen::CloseTopmost()
{
    // Original always returned false (no "close topmost" on load screens).
    return false;
}

// +--------------------------------------------------------------------+

void ULoadScreen::Show()
{
    if (!bIsShown)
    {
        ShowLoadDlg();
        bIsShown = true;
    }
}

void ULoadScreen::Hide()
{
    if (bIsShown)
    {
        HideLoadDlg();
        bIsShown = false;
    }
}

// +--------------------------------------------------------------------+

void ULoadScreen::ShowLoadDlg()
{
    // Hide both first (matches original logic).
    if (LoadDlg)    LoadDlg->SetVisibility(ESlateVisibility::Hidden);
    if (CmpLoadDlg) CmpLoadDlg->SetVisibility(ESlateVisibility::Hidden);

    Starshatter* Stars = Starshatter::GetInstance();

    // Show campaign load dialog if available and loading campaign:
    if (Stars && CmpLoadDlg)
    {
        if (Stars->GetGameMode() == EGameMode::CLOD ||
            Stars->GetGameMode() == EGameMode::CMPN)
        {
            CmpLoadDlg->SetVisibility(ESlateVisibility::Visible);

            // Hide mouse cursor (legacy behavior):
            // If you use a PlayerController cursor, hide it here.
            if (UWorld* World = GetWorld())
            {
                if (APlayerController* PC = World->GetFirstPlayerController())
                {
                    PC->bShowMouseCursor = false;
                }
            }

            return;
        }
    }

    // Otherwise, show regular load dialog:
    if (LoadDlg)
    {
        LoadDlg->SetVisibility(ESlateVisibility::Visible);

        if (UWorld* World = GetWorld())
        {
            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                PC->bShowMouseCursor = false;
            }
        }
    }
}

// +--------------------------------------------------------------------+

void ULoadScreen::HideLoadDlg()
{
    if (LoadDlg && LoadDlg->IsVisible())
        LoadDlg->SetVisibility(ESlateVisibility::Hidden);

    if (CmpLoadDlg && CmpLoadDlg->IsVisible())
        CmpLoadDlg->SetVisibility(ESlateVisibility::Hidden);
}
