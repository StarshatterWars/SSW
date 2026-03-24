// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "OOBListView.h"

#include "Components/TextBlock.h"
#include "UObject/ConstructorHelpers.h"

#include "GameStructs.h"
#include "OOBForceItem.h"
#include "OOBFleetItem.h"
#include "OOBCarrierGroupItem.h" 
#include "OOBBattleItem.h"
#include "OOBDestroyerItem.h"
#include "OOBWingItem.h"

void UOOBListView::NativeConstruct()
{
	//Super::NativeConstruct();

	// Binding selection event
	OnItemSelectionChanged().AddUObject(this, &UOOBListView::OnItemSelected);
}

void UOOBListView::OnItemSelected(UObject* SelectedItem)
{
	// Call UpdateSelectedItemDetails to handle UI update when item is selected
	UpdateSelectedItemDetails(SelectedItem);
}

void UOOBListView::UpdateSelectedItemDetails(UObject* SelectedItem)
{
	if (UOOBForceItem* ForceItem = Cast<UOOBForceItem>(SelectedItem))
	{
		// Update UI for ForceItem selection
		UE_LOG(LogTemp, Log, TEXT("Selected Force: %s"), *ForceItem->Data.Name);
	}
	else if (UOOBFleetItem* FleetItem = Cast<UOOBFleetItem>(SelectedItem))
	{
		// Update UI for FleetItem selection
		UE_LOG(LogTemp, Log, TEXT("Selected Fleet: %s"), *FleetItem->Data.Name);
	}
	// Add additional else if blocks for other types (Carrier, Battle, etc.)
}
