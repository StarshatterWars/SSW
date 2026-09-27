/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         LoadDlg.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Loading progress dialog (legacy LoadDlg) adapted for Unreal UMG.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "LoadDlg.generated.h"

// Forward declarations (UMG)
class UTextBlock;
class UProgressBar;

class Starshatter;

// --------------------------------------------------------------------

UCLASS()
class STARSHATTERWARS_API ULoadDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    ULoadDlg(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // Legacy parity:
    void RegisterControls();
    void ExecFrame();

protected:
    // ----------------------------------------------------------------
    // Bound UMG widgets (legacy ids: 100, 101, 102)
    // ----------------------------------------------------------------
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* TitleText = nullptr;        // id 100

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* ActivityText = nullptr;     // id 101

    UPROPERTY(meta = (BindWidgetOptional))
    UProgressBar* ProgressBar = nullptr;    // id 102 (Slider -> ProgressBar)

private:
    void SetTextBlock(UTextBlock* Block, const char* AnsiText);
};
