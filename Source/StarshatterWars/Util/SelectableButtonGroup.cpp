// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "SelectableButtonGroup.h"
#include "MenuButton.h"

void USelectableButtonGroup::NativeConstruct()
{
	Super::NativeConstruct();

	// Clear any buttons registered via Blueprints at design time
	RegisteredButtons.Empty();
}

void USelectableButtonGroup::RegisterButton(UMenuButton* Button)
{
	if (!Button || RegisteredButtons.Contains(Button))
	{
		return;
	}

	RegisteredButtons.Add(Button);
	Button->OnSelected.AddDynamic(this, &USelectableButtonGroup::OnButtonSelected);
}

void USelectableButtonGroup::OnButtonSelected(UMenuButton* SelectedButton)
{
	for (UMenuButton* Button : RegisteredButtons)
	{
		if (IsValid(Button))
		{
			Button->SetSelected(Button == SelectedButton);
		}
	}
}