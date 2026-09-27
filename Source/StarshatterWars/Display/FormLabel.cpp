/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe (UE)
    FILE:         FormLabel.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    UE port of legacy FormLabel control.
*/

#include "FormLabel.h"

#include "Widgets/Text/STextBlock.h"

UFormLabel::UFormLabel(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    Text = FText::GetEmpty();
    ColorAndOpacity = FSlateColor(FLinearColor::White);
    Font = FSlateFontInfo();
}

TSharedRef<SWidget> UFormLabel::RebuildWidget()
{
    TextBlock =
        SNew(STextBlock)
        .Text(Text)
        .ColorAndOpacity(ColorAndOpacity)
        .Font(Font);

    return TextBlock.ToSharedRef();
}

void UFormLabel::SynchronizeProperties()
{
    Super::SynchronizeProperties();

    if (TextBlock.IsValid())
    {
        TextBlock->SetText(Text);
        TextBlock->SetColorAndOpacity(ColorAndOpacity);
        TextBlock->SetFont(Font);
    }
}

void UFormLabel::SetText(const FText& InText)
{
    Text = InText;

    if (TextBlock.IsValid())
        TextBlock->SetText(Text);
}

void UFormLabel::SetColorAndOpacity(const FSlateColor& InColor)
{
    ColorAndOpacity = InColor;

    if (TextBlock.IsValid())
        TextBlock->SetColorAndOpacity(ColorAndOpacity);
}

void UFormLabel::SetFont(const FSlateFontInfo& InFont)
{
    Font = InFont;

    if (TextBlock.IsValid())
        TextBlock->SetFont(Font);
}
