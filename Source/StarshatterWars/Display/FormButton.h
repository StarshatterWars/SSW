/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe (UE)
    FILE:         FormButton.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    UE port of legacy FormButton control.
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateColor.h"
#include "FormButton.generated.h"

class SButton;
class STextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFormButtonClicked);

UCLASS(BlueprintType, Blueprintable)
class STARSHATTERWARS_API UFormButton : public UWidget
{
    GENERATED_BODY()

public:
    UFormButton(const FObjectInitializer& ObjectInitializer);

    /** Legacy-like API */
    UFUNCTION(BlueprintCallable, Category = "Form|Button")
    void SetText(const FText& InText);

    UFUNCTION(BlueprintCallable, Category = "Form|Button")
    void SetEnabled(bool bInEnabled);

    UFUNCTION(BlueprintCallable, Category = "Form|Button")
    void SetFont(const FSlateFontInfo& InFont);

    UFUNCTION(BlueprintCallable, Category = "Form|Button")
    void SetTextColor(const FSlateColor& InColor);

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Form|Button")
    FText Text;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Form|Button")
    bool bEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Form|Button")
    FSlateFontInfo Font;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Form|Button")
    FSlateColor TextColor;

    /** UE event hook */
    UPROPERTY(BlueprintAssignable, Category = "Form|Button")
    FFormButtonClicked OnClicked;

protected:
    // UWidget
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void SynchronizeProperties() override;

private:
    FReply HandleClicked();

private:
    TSharedPtr<SButton> Button;
    TSharedPtr<STextBlock> Label;
};

