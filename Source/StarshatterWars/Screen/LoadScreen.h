/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         LoadScreen.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    LoadScreen
    - UMG load screen widget.
    - Inherits from UBaseScreen to use common dialog behavior + legacy FORM binding.
    - Hosts/controls loading dialog widgets (LoadDlg / CmpLoadDlg) and screen show/hide state.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "GameStructs.h"

#include "LoadScreen.generated.h"

// +--------------------------------------------------------------------+
// Forward Declarations
// +--------------------------------------------------------------------+

class ULoadDlg;
class UCmpLoadDlg;

// If you are still using legacy Starshatter screen/window types elsewhere:
class Screen;

// +--------------------------------------------------------------------+

UCLASS()
class STARSHATTERWARS_API ULoadScreen : public UBaseScreen
{
    GENERATED_BODY()

public:
    ULoadScreen(const FObjectInitializer& ObjectInitializer);

    // ----------------------------------------------------------------
    // Legacy Screen-style lifecycle (kept to match Starshatter call sites)
    // ----------------------------------------------------------------
    virtual void Setup(Screen* InScreen);
    virtual void TearDown();
    virtual bool CloseTopmost();

    // ----------------------------------------------------------------
    // Visibility control
    // ----------------------------------------------------------------
    virtual bool IsShown() const { return bIsShown; }
    virtual void Show();
    virtual void Hide();

    // ----------------------------------------------------------------
    // Dialog control
    // ----------------------------------------------------------------
    virtual void ShowLoadDlg();
    virtual void HideLoadDlg();

    ULoadDlg* GetLoadDlg() const { return LoadDlg; }
    UCmpLoadDlg* GetCmpLoadDlg() const { return CmpLoadDlg; }

    // ----------------------------------------------------------------
    // Per-frame hook (called by external UI driver if needed)
    // ----------------------------------------------------------------
    virtual void ExecFrame();

protected:
    // ----------------------------------------------------------------
    // UBaseScreen overrides (optional)
    // ----------------------------------------------------------------
    virtual void NativeOnInitialized() override;

private:
    // Non-owning pointer to legacy screen wrapper (if still used).
    Screen* ScreenPtr = nullptr;

    // Child dialog widgets (UMG)
    ULoadDlg* LoadDlg = nullptr;

    UCmpLoadDlg* CmpLoadDlg = nullptr;

    bool bIsShown = false;
};
