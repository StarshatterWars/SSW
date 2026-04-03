/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2024-2026. All Rights Reserved.

    SUBSYSTEM:    SSW
    FILE:         MenuButton.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Shared menu button widget.

    Supports:
      - selected / hovered / normal visual states
      - optional compact sizing through RootSizeBox
      - optional label font size override
      - click / hover sounds
      - layout modes for fixed, desired, or fill-width behavior
*/

#pragma once

#include "CoreMinimal.h"
#include "GameStructs_UI.h"
#include "Blueprint/UserWidget.h"
#include "MenuButton.generated.h"

class UButton;
class UImage;
class USizeBox;
class UTextBlock;
class USoundBase;


UCLASS()
class STARSHATTERWARS_API UMenuButton : public UUserWidget
{
    GENERATED_BODY()


protected:
    virtual void NativeConstruct() override;

public:
    // -----------------------------------------------------------------
    // Data
    // -----------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
    FString MenuOption;

    // -----------------------------------------------------------------
    // Widget Bindings
    // -----------------------------------------------------------------

    UPROPERTY(meta = (BindWidgetOptional))
    USizeBox* RootSizeBox = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), EditAnywhere, Category = "Data")
    UTextBlock* Label = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UImage* BackgroundImage = nullptr;

    UPROPERTY(meta = (BindWidget))
    UButton* Button = nullptr;

    // -----------------------------------------------------------------
    // Sound
    // -----------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* ClickSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* HoverSound = nullptr;

    // -----------------------------------------------------------------
    // Appearance
    // -----------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor SelectedColor = FLinearColor(0.1f, 0.4f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor HoveredColor = FLinearColor(0.7f, 0.7f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor NormalColor = FLinearColor::Transparent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    float WidthOverride = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    float HeightOverride = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    int32 LabelFontSize = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    ELayoutMode LayoutMode = ELayoutMode::FixedSize;

    // -----------------------------------------------------------------
    // State
    // -----------------------------------------------------------------

    void SetSelected(bool bInSelected);
    bool IsSelected() const { return bIsSelected; }

    // -----------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------

    void SetButtonText(const FText& InText);
    void SetMenuOption(const FString& InMenuOption);
    void SetButtonSize(float InWidth, float InHeight);
    void SetLabelFontSizeValue(int32 InFontSize);
    void SetLayoutMode(ELayoutMode InLayoutMode);

    FString GetMenuOption() const { return MenuOption; }

    // -----------------------------------------------------------------
    // Events
    // -----------------------------------------------------------------

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMenuButtonSelected, UMenuButton*, SelectedButton);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMenuButtonHovered, UMenuButton*, HoveredButton);

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMenuButtonSelected OnSelected;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMenuButtonHovered OnHovered;

private:
    bool bIsSelected = false;
    bool bIsHovered = false;

    void UpdateVisuals();
    void ApplyLayoutOverrides();
    void ApplyFontOverride();

    UFUNCTION()
    void HandleClicked();

    UFUNCTION()
    void HandleHovered();

    UFUNCTION()
    void HandleUnhovered();
};