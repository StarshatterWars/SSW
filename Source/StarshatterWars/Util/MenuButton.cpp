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
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "GameStructs_UI.h"

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
    }

    ApplyFontOverride();
    ApplyLayoutOverrides();
    UpdateVisuals();
}

void UMenuButton::ApplyFontOverride()
{
    if (Label && LabelFontSize > 0)
    {
        FSlateFontInfo FontInfo = Label->GetFont();
        FontInfo.Size = LabelFontSize;
        Label->SetFont(FontInfo);
    }
}

void UMenuButton::ApplyLayoutOverrides()
{
    if (!RootSizeBox)
    {
        return;
    }

    USizeBoxSlot* InnerSlot = Cast<USizeBoxSlot>(RootSizeBox->GetContentSlot());

    switch (LayoutMode)
    {
    case ELayoutMode::FixedSize:
        if (WidthOverride > 0.f)
        {
            RootSizeBox->SetWidthOverride(WidthOverride);
        }
        else
        {
            RootSizeBox->ClearWidthOverride();
        }

        if (HeightOverride > 0.f)
        {
            RootSizeBox->SetHeightOverride(HeightOverride);
        }
        else
        {
            RootSizeBox->ClearHeightOverride();
        }

        if (InnerSlot)
        {
            InnerSlot->SetHorizontalAlignment(HAlign_Center);
            InnerSlot->SetVerticalAlignment(VAlign_Center);
        }
        break;

    case ELayoutMode::FillWidth:
        RootSizeBox->ClearWidthOverride();

        if (HeightOverride > 0.f)
        {
            RootSizeBox->SetHeightOverride(HeightOverride);
        }
        else
        {
            RootSizeBox->ClearHeightOverride();
        }

        if (InnerSlot)
        {
            InnerSlot->SetHorizontalAlignment(HAlign_Fill);
            InnerSlot->SetVerticalAlignment(VAlign_Fill);
        }
        break;

    case ELayoutMode::DesiredSize:
        RootSizeBox->ClearWidthOverride();
        RootSizeBox->ClearHeightOverride();

        if (InnerSlot)
        {
            InnerSlot->SetHorizontalAlignment(HAlign_Center);
            InnerSlot->SetVerticalAlignment(VAlign_Center);
        }
        break;

    default:
        break;
    }
}

void UMenuButton::SetSelected(bool bInSelected)
{
    bIsSelected = bInSelected;
    UpdateVisuals();
}

void UMenuButton::SetButtonText(const FText& InText)
{
    MenuOption = InText.ToString();

    if (Label)
    {
        Label->SetText(InText);
    }
}

void UMenuButton::SetMenuOption(const FString& InMenuOption)
{
    MenuOption = InMenuOption;

    if (Label)
    {
        Label->SetText(FText::FromString(MenuOption));
    }
}

void UMenuButton::SetButtonSize(float InWidth, float InHeight)
{
    WidthOverride = InWidth;
    HeightOverride = InHeight;
    ApplyLayoutOverrides();
}

void UMenuButton::SetLabelFontSizeValue(int32 InFontSize)
{
    LabelFontSize = InFontSize;
    ApplyFontOverride();
}

void UMenuButton::SetLayoutMode(ELayoutMode InLayoutMode)
{
    LayoutMode = InLayoutMode;
    ApplyLayoutOverrides();
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