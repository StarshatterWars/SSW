/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2024-2026. All Rights Reserved.

    SUBSYSTEM:    SSW
    FILE:         MenuButton.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Shared menu button widget implementation.
*/

#include "MenuButton.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void UMenuButton::NativeConstruct()
{
    Super::NativeConstruct();

    if (Button)
    {
        Button->OnClicked.RemoveAll(this);
        Button->OnClicked.AddUniqueDynamic(this, &UMenuButton::HandleClicked);

        Button->OnHovered.RemoveAll(this);
        Button->OnHovered.AddUniqueDynamic(this, &UMenuButton::HandleHovered);

        Button->OnUnhovered.RemoveAll(this);
        Button->OnUnhovered.AddUniqueDynamic(this, &UMenuButton::HandleUnhovered);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuButton] Button is null"));
    }

    if (Label)
    {
        Label->SetText(FText::FromString(MenuOption));

        if (LabelFontSize > 0)
        {
            FSlateFontInfo FontInfo = Label->GetFont();
            FontInfo.Size = LabelFontSize;
            Label->SetFont(FontInfo);
        }
    }

    ApplyLayoutOverrides();
    UpdateVisuals();
}

void UMenuButton::ApplyLayoutOverrides()
{
    if (!RootSizeBox)
    {
        return;
    }

    if (WidthOverride > 0.f)
    {
        RootSizeBox->SetWidthOverride(WidthOverride);
    }

    if (HeightOverride > 0.f)
    {
        RootSizeBox->SetHeightOverride(HeightOverride);
    }
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
    if (!BackgroundImage)
    {
        return;
    }

    FLinearColor Color = NormalColor;

    if (bIsSelected)
    {
        Color = SelectedColor;
    }
    else if (bIsHovered)
    {
        Color = HoveredColor;
    }

    BackgroundImage->SetColorAndOpacity(Color);
}