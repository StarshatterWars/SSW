/*=============================================================================
    Project:        Starshatter Wars (Unreal Port)
    Studio:         Fractal Dev Studios
    Copyright:      (C) 2025–2026.

    DIALOG:         ExitDlg
    FILE:           ExitDlg.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UExitDlg
    - Unreal UMG replacement for legacy ExitDlg.frm
    - Pure UMG (no FormDef bridge)
    - MenuScreen owns Z-order + modal policy
    - ExitDlg owns only: bind buttons, scroll credits, quit/cancel actions
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "ExitDlg.generated.h"

class UButton;
class UScrollBox;
class URichTextBlock;
class UMenuScreen;

UCLASS(BlueprintType, Blueprintable)
class STARSHATTERWARS_API UExitDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UExitDlg(const FObjectInitializer& ObjectInitializer);

    /* --------------------------------------------------------------------
       Manager wiring
       -------------------------------------------------------------------- */
    void SetMenuManager(UMenuScreen* InManager) { MenuManager = InManager; }

protected:
    /* --------------------------------------------------------------------
       UUserWidget
       -------------------------------------------------------------------- */
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    /* --------------------------------------------------------------------
       Input (keyboard/controller)
       -------------------------------------------------------------------- */
    virtual void HandleAccept() override;
    virtual void HandleCancel() override;

protected:
    /* --------------------------------------------------------------------
       UMG Events
       -------------------------------------------------------------------- */
    UFUNCTION()
    void OnExitClicked();

    UFUNCTION()
    void OnCancelClicked();

protected:
    /* --------------------------------------------------------------------
       Credits
       -------------------------------------------------------------------- */
    bool LoadCreditsFile(FString& OutText) const;
    void ApplyCredits(const FString& Text);
    void TickCredits(float DeltaTime);

protected:
    /* --------------------------------------------------------------------
       Widgets (BindWidgetOptional)
       Names must match WBP_ExitDlg variables
       -------------------------------------------------------------------- */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> ExitBtn;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> CancelBtn;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UScrollBox> CreditsScroll;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URichTextBlock> CreditsText;

protected:

    bool  bExitLatch = false;
    float ScrollOffset = 0.0f;

    UPROPERTY(EditAnywhere, Category = "ExitDlg")
    float ScrollPixelsPerSecond = 22.0f;
};
