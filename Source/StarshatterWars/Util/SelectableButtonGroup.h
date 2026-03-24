// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MenuButton.h"
#include "SelectableButtonGroup.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API USelectableButtonGroup : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;

	/** Register a button to this group */
	UFUNCTION(BlueprintCallable, Category = "Button Group")
	void RegisterButton(UMenuButton* Button);

protected:
	/** Called when any button in the group is selected */
	UFUNCTION()
	void OnButtonSelected(UMenuButton* SelectedButton);

	UPROPERTY()
	TArray<UMenuButton*> RegisteredButtons;
};
	
	

