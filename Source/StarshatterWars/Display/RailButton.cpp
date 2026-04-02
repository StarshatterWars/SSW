/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2024-2026. All Rights Reserved.

    SUBSYSTEM:    SSW
    FILE:         RailButton.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Dedicated stretch-style button for dialog rails.
*/

#include "RailButton.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void URailButton::NativeConstruct()
{
    Super::NativeConstruct();

    if (Button)
    {
        Button->OnClicked.RemoveAll(this);
        Button->OnClicked.AddUniqueDynamic(this, &URailButton::HandleClicked);

        Button->OnHovered.RemoveAll(this);
        Button->OnHovered.AddUniqueDynamic(this, &URailButton::HandleHovered);

        Button->OnUnhovered.RemoveAll(this);
        Button->OnUnhovered.AddUniqueDynamic(this, &URailButton::HandleUnhovered);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[RailButton] Button is null"));
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

void URailButton::ApplyLayoutOverrides()
{
    // IMPORTANT:
    // Do NOT apply width override here.
    // Width is controlled by the parent layout so the button can fill horizontally.

    if (RootSizeBox)
    {
        RootSizeBox->ClearWidthOverride();

        if (HeightOverride > 0.f)
        {
            RootSizeBox->SetHeightOverride(HeightOverride);
        }
        else
        {
            RootSizeBox->ClearHeightOverride();
        }
    }

    // If no RootSizeBox exists, do nothing.
    // Let the widget hierarchy/padding determine height.
}

void URailButton::SetSelected(bool bInSelected)
{
    bIsSelected = bInSelected;
    UpdateVisuals();
}

void URailButton::SetButtonText(const FText& InText)
{
    MenuOption = InText.ToString();

    if (Label)
    {
        Label->SetText(InText);
    }
}

void URailButton::SetMenuOption(const FString& InMenuOption)
{
    MenuOption = InMenuOption;

    if (Label)
    {
        Label->SetText(FText::FromString(MenuOption));
    }
}

void URailButton::SetButtonHeight(float InHeight)
{
    HeightOverride = InHeight;
    ApplyLayoutOverrides();
}

void URailButton::SetLabelFontSizeValue(int32 InFontSize)
{
    LabelFontSize = InFontSize;

    if (Label && LabelFontSize > 0)
    {
        FSlateFontInfo FontInfo = Label->GetFont();
        FontInfo.Size = LabelFontSize;
        Label->SetFont(FontInfo);
    }
}

void URailButton::HandleClicked()
{
    OnSelected.Broadcast(this);

    if (ClickSound)
    {
        UGameplayStatics::PlaySound2D(this, ClickSound);
    }
}

void URailButton::HandleHovered()
{
    bIsHovered = true;
    UpdateVisuals();

    OnHovered.Broadcast(this);

    if (HoverSound)
    {
        UGameplayStatics::PlaySound2D(this, HoverSound);
    }
}

void URailButton::HandleUnhovered()
{
    bIsHovered = false;
    UpdateVisuals();
}

void URailButton::UpdateVisuals()
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