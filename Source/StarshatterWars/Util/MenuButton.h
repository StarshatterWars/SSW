// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MenuButton.generated.h"

class UButton;
class UImage;
class UTextBlock;
class USoundBase;
/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMenuButtonSelected, UMenuButton*, SelectedButton);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMenuButtonHovered, UMenuButton*, HoveredButton);


UCLASS()
class STARSHATTERWARS_API UMenuButton : public UUserWidget
{
	GENERATED_BODY()
	virtual void NativeConstruct() override;

public:
	// Button display name (e.g., "Empire", "Fleet", etc.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	FString MenuOption;

	// Visual label
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Label;

	// Background for color changes
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* BackgroundImage;

	// The actual button
	UPROPERTY(meta = (BindWidget))
	UButton* Button;

	// Sound cues
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* ClickSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* HoverSound;

	// Appearance colors
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor SelectedColor = FLinearColor(0.1f, 0.4f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor HoveredColor = FLinearColor(0.7f, 0.7f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor NormalColor = FLinearColor::Transparent;

	// Selection state
	void SetSelected(bool bInSelected);
	bool IsSelected() const { return bIsSelected; }

	// Delegates
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMenuButtonSelected OnSelected;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMenuButtonHovered OnHovered;

private:
	bool bIsSelected = false;
	bool bIsHovered = false;

	void UpdateVisuals();

	// Event Handlers
	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleHovered();

	UFUNCTION()
	void HandleUnhovered();
};