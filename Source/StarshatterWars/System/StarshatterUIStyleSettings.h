/*=============================================================================
    Project:        Starshatter Wars (Unreal Engine Port)
    Studio:         Fractal Dev Studios
    Copyright:      (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:      UI Style
    FILE:           StarshatterUIStyleSettings.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UStarshatterUIStyleSettings

    Project Settings (config) for UI theme assets.

    Option A:
      - Configure textures/fonts/colors in Project Settings
      - UIStyle subsystem loads settings on boot
      - No hard-coded /Game paths in code

=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"
#include "Sound/SoundBase.h"
#include "StarshatterUIStyleSettings.generated.h"

UCLASS(config = Game, defaultconfig, BlueprintType)
class STARSHATTERWARS_API UStarshatterUIStyleSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    // ------------------------------------------------------------
    // Project Settings placement
    // ------------------------------------------------------------
    virtual FName GetCategoryName() const override
    {
        return TEXT("Starshatter Wars");
    }

    virtual FName GetSectionName() const override
    {
        return TEXT("UI Style");
    }

    virtual FText GetSectionText() const override
    {
        return FText::FromString(TEXT("UI Style"));
    }

    virtual FText GetSectionDescription() const override
    {
        return FText::FromString(TEXT("Centralized UI theme bindings (textures/fonts/colors) used by the UI Style subsystem."));
    }

public:
    // ------------------------------------------------------------
    // Default text styling
    // ------------------------------------------------------------
    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI|Text")
    TSoftObjectPtr<UFont> DefaultUIFont;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI|Text")
    FLinearColor DefaultTextColor = FLinearColor::Black;

    // "Title" / emphasis font (optional)
    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI|Text")
    TSoftObjectPtr<UFont> TitleUIFont;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI|Text")
    FLinearColor TitleTextColor = FLinearColor::White;

    // ------------------------------------------------------------
    // Default button skin (dialog buttons)
    // ------------------------------------------------------------
    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI|Buttons|Default")
    TSoftObjectPtr<UTexture2D> Btn_NormalTex;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI|Buttons|Default")
    TSoftObjectPtr<UTexture2D> Btn_HoverTex;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI|Buttons|Default")
    TSoftObjectPtr<UTexture2D> Btn_PressedTex;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI|Buttons|Default")
    TSoftObjectPtr<UTexture2D> Btn_DisabledTex;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI|Buttons|Default")
    FMargin Btn_9SliceMargin = FMargin(4.f / 16.f);

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI|Buttons|Default")
    FVector2D Btn_ImageSize = FVector2D(0.f, 0.f);

    // ------------------------------------------------------------
    // Menu button skin (separate)
    // ------------------------------------------------------------
    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI|Buttons|Menu")
    TSoftObjectPtr<UTexture2D> MenuBtn_NormalTex;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI|Buttons|Menu")
    TSoftObjectPtr<UTexture2D> MenuBtn_HoverTex;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI|Buttons|Menu")
    TSoftObjectPtr<UTexture2D> MenuBtn_PressedTex;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI|Buttons|Menu")
    TSoftObjectPtr<UTexture2D> MenuBtn_DisabledTex;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI|Buttons|Menu")
    FMargin MenuBtn_9SliceMargin = FMargin(4.f / 16.f);

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI|Buttons|Menu")
    int32 MenuButtonFontSize = 24;

    UPROPERTY(EditAnywhere, config, Category = "Starshatter|UI|Buttons|Menu")
    FLinearColor MenuButtonTextColor = FLinearColor(1.f, 0.9f, 0.2f, 1.f);

    // ----------------------------------------------------
    // UI SFX
    // ----------------------------------------------------
    UPROPERTY(EditAnywhere, config, Category = "UI|Sounds")
    TSoftObjectPtr<USoundBase> ButtonHoverSound;

    UPROPERTY(EditAnywhere, config, Category = "UI|Sounds")
    TSoftObjectPtr<USoundBase> ButtonClickSound;

    UPROPERTY(EditAnywhere, config, Category = "UI|Sounds", meta = (ClampMin = "0.0"))
    float ButtonSoundVolume = 1.0f;
};
