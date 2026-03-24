/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe (UE)
    FILE:         FormLabel.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    UE port of legacy FormLabel control.
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Styling/SlateColor.h"
#include "Fonts/SlateFontInfo.h"
#include "FormLabel.generated.h"

class STextBlock;

UCLASS(BlueprintType, Blueprintable)
class STARSHATTERWARS_API UFormLabel : public UWidget
{
    GENERATED_BODY()

public:
    UFormLabel(const FObjectInitializer& ObjectInitializer);

    /** Legacy-like API */
    UFUNCTION(BlueprintCallable, Category = "Form|Label")
    void SetText(const FText& InText);

    UFUNCTION(BlueprintCallable, Category = "Form|Label")
    FText GetText() const { return Text; }

    UFUNCTION(BlueprintCallable, Category = "Form|Label")
    void SetColorAndOpacity(const FSlateColor& InColor);

    UFUNCTION(BlueprintCallable, Category = "Form|Label")
    void SetFont(const FSlateFontInfo& InFont);

public:
    /** Designer-exposed defaults */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Form|Label")
    FText Text;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Form|Label")
    FSlateColor ColorAndOpacity;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Form|Label")
    FSlateFontInfo Font;

protected:
    // UWidget
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void SynchronizeProperties() override;

private:
    TSharedPtr<STextBlock> TextBlock;
};

