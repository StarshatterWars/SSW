// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "MenuButton.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"

void UMenuButton::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button)
	{
		Button->OnClicked.AddUniqueDynamic(this, &UMenuButton::HandleClicked);
		Button->OnHovered.AddUniqueDynamic(this, &UMenuButton::HandleHovered);
		Button->OnUnhovered.AddUniqueDynamic(this, &UMenuButton::HandleUnhovered);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Button is null in UMenuButton!"));
	}

	if (Label)
	{
		Label->SetText(FText::FromString(MenuOption));
	}

	UpdateVisuals();
}

void UMenuButton::SetSelected(bool bInSelected)
{
	bIsSelected = bInSelected;
	UpdateVisuals();
}

void UMenuButton::HandleClicked()
{
	OnSelected.Broadcast(this);

	if (ClickSound)
	{
		UGameplayStatics::PlaySound2D(this, ClickSound);
	}
}

void UMenuButton::HandleHovered()
{
	bIsHovered = true;
	UpdateVisuals();

	OnHovered.Broadcast(this);

	if (HoverSound)
	{
		UGameplayStatics::PlaySound2D(this, HoverSound);
	}
}

void UMenuButton::HandleUnhovered()
{
	bIsHovered = false;
	UpdateVisuals();
}

void UMenuButton::UpdateVisuals()
{
	if (!BackgroundImage) return;

	FLinearColor Color;

	if (bIsSelected)
	{
		Color = SelectedColor;
	}
	else if (bIsHovered)
	{
		Color = HoveredColor;
	}
	else
	{
		Color = NormalColor;
	}

	BackgroundImage->SetColorAndOpacity(Color);
}