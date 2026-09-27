/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2024-2026. All Rights Reserved.

    SUBSYSTEM:    SSW
    FILE:         RailButton.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Dedicated stretch-style button for dialog rails.

    Supports:
      - selected / hovered / normal visual states
      - horizontal fill behavior
      - optional fixed height
      - optional label font size override
      - click / hover sounds
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RailButton.generated.h"

class UButton;
class UImage;
class USizeBox;
class UTextBlock;
class USoundBase;
class URailButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRailButtonSelected, URailButton*, SelectedButton);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRailButtonHovered, URailButton*, HoveredButton);

UCLASS()
class STARSHATTERWARS_API URailButton : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
    FString MenuOption;

    UPROPERTY(meta = (BindWidgetOptional))
    USizeBox* RootSizeBox = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadWrite, Category = "Widgets")
    UTextBlock* Label = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UImage* BackgroundImage = nullptr;

    UPROPERTY(meta = (BindWidget))
    UButton* Button = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* ClickSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* HoverSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor SelectedColor = FLinearColor(0.1f, 0.4f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor HoveredColor = FLinearColor(0.7f, 0.7f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor NormalColor = FLinearColor::Transparent;

    // Height only. Width is controlled by parent layout.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    float HeightOverride = 32.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    int32 LabelFontSize = 16;

    void SetSelected(bool bInSelected);
    bool IsSelected() const { return bIsSelected; }

    void SetButtonText(const FText& InText);
    void SetMenuOption(const FString& InMenuOption);
    void SetButtonHeight(float InHeight);
    void SetLabelFontSizeValue(int32 InFontSize);

    FString GetMenuOption() const { return MenuOption; }

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnRailButtonSelected OnSelected;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnRailButtonHovered OnHovered;

private:
    bool bIsSelected = false;
    bool bIsHovered = false;

    void UpdateVisuals();
    void ApplyLayoutOverrides();

    UFUNCTION()
    void HandleClicked();

    UFUNCTION()
    void HandleHovered();

    UFUNCTION()
    void HandleUnhovered();
};