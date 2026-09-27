// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ListView.h"
#include "OOBListView.generated.h"

// Forward declarations
class UObject;
/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UOOBListView : public UListView
{
	GENERATED_BODY()
	
	public:
	// Called when an item is selected
	UFUNCTION()
	void OnItemSelected(UObject* SelectedItem);
	
protected:
	// Override the item click behavior
	virtual void NativeConstruct();

	// Helper to update the UI based on selected item
	void UpdateSelectedItemDetails(UObject* SelectedItem);
};