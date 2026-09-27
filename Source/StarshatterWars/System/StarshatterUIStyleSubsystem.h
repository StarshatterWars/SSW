/*=============================================================================
    Project:        Starshatter Wars (Unreal Engine Port)
    Studio:         Fractal Dev Studios
    Copyright:      (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:      UI Style
    FILE:           StarshatterUIStyleSubsystem.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UStarshatterUIStyleSubsystem

    Runtime style applier. Loads config from UStarshatterUIStyleSettings.

    Exposes helper functions used by UBaseScreen and other widgets:
      - ApplyDefaultButtonStyle
      - ApplyMenuButtonStyle
      - ApplyDefaultTextStyle
      - ApplyTitleTextStyle
      - ApplyDefaultEditBoxStyle

=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "Components/TextBlock.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"
#include "Fonts/SlateFontInfo.h"

#include "StarshatterUIStyleSubsystem.generated.h"

class UButton;
class UMenuButton;
class UEditableTextBox;
class UTexture2D;
class UFont;

DECLARE_LOG_CATEGORY_EXTERN(LogStarshatterUIStyle, Log, All);

UCLASS()
class STARSHATTERWARS_API UStarshatterUIStyleSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Reload settings (handy for PIE / debugging)
    UFUNCTION(BlueprintCallable, Category = "Starshatter|UI|Style")
    void ReloadFromSettings(bool bLoadNow = true);

public:
    // ------------------------------------------------------------
    // Style helpers (called by BaseScreen / widgets)
    // ------------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Starshatter|UI|Style")
    void ApplyDefaultButtonStyle(UButton* Button, int32 FontSize = 20, FLinearColor ButtonColor = FLinearColor::Black);

    UFUNCTION(BlueprintCallable, Category = "Starshatter|UI|Style")
    void ApplyMenuButtonStyle(UButton* Button);
   
    UFUNCTION(BlueprintCallable, Category = "Starshatter|UI|Style")
    void ApplyMenuListButtonStyle(UMenuButton* Button);

    UFUNCTION(BlueprintCallable, Category = "Starshatter|UI|Style")
    void ApplyDefaultTextStyle(UTextBlock* Text, int32 FontSize = 18);

    UFUNCTION(BlueprintCallable, Category = "Starshatter|UI|Style")
    void ApplyTitleTextStyle(UTextBlock* Text, int32 FontSize = 24);

    UFUNCTION(BlueprintCallable, Category = "Starshatter|UI|Style")
    void ApplyDefaultEditBoxStyle(UEditableTextBox* Edit, int32 FontSize = 18);

    // Optional override-friendly variant (NO pointers, UHT-safe)
    UFUNCTION(BlueprintCallable, Category = "Starshatter|UI|Style")
    void ApplyDefaultButtonStyle_Override(
        UButton* Button,
        int32 FontSize,
        bool bOverrideTextColor,
        FLinearColor OverrideTextColor,
        UFont* OverrideFont
    );

public:
    // ------------------------------------------------------------
    // Cached runtime assets (resolved from settings)
    // These are not config; they are loaded from config.
    // ------------------------------------------------------------

    UPROPERTY(Transient) TObjectPtr<UFont> DefaultUIFont = nullptr;
    UPROPERTY(Transient) FLinearColor DefaultTextColor = FLinearColor::White;

    UPROPERTY(Transient) TObjectPtr<UFont> TitleUIFont = nullptr;
    UPROPERTY(Transient) FLinearColor TitleTextColor = FLinearColor::Black;

    // Default button skin
    UPROPERTY(Transient) TObjectPtr<UTexture2D> Btn_NormalTex = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> Btn_HoverTex = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> Btn_PressedTex = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> Btn_DisabledTex = nullptr;
    UPROPERTY(Transient) FMargin Btn_9SliceMargin = FMargin(4.f / 16.f);
    UPROPERTY(Transient) FVector2D Btn_ImageSize = FVector2D(0.f, 0.f);

    // Menu button skin
    UPROPERTY(Transient) TObjectPtr<UTexture2D> MenuBtn_NormalTex = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> MenuBtn_HoverTex = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> MenuBtn_PressedTex = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> MenuBtn_DisabledTex = nullptr;
    UPROPERTY(Transient) FMargin MenuBtn_9SliceMargin = FMargin(4.f / 16.f);

    UPROPERTY(Transient) int32 MenuButtonFontSize = 24;
    UPROPERTY(Transient) FLinearColor MenuButtonTextColor = FLinearColor(0.1f, 0.1f, 0.1f, 1.f);

private:
    // Internal
    static FSlateBrush MakeBrush(UTexture2D* Tex, const FMargin& Margin, const FVector2D& ImageSize);
    static UTextBlock* FindFirstTextBlockRecursive(const UWidget* Root);

    void ApplyButtonStyleInternal(
        UButton* Button,
        UTexture2D* Normal,
        UTexture2D* Hover,
        UTexture2D* Pressed,
        UTexture2D* Disabled,
        const FMargin& SliceMargin,
        const FVector2D& ImageSize,
        UFont* Font,
        int32 FontSize,
        const FLinearColor& TextColor
    );

    // Default UI sounds loaded from Project Settings
    UPROPERTY(Transient)
    TObjectPtr<USoundBase> ButtonHoverSound = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USoundBase> ButtonClickSound = nullptr;

    UPROPERTY(Transient)
    float ButtonSoundVolume = 1.0f; // NOTE: SlateSound ignores volume; kept for future manual playback.
};


