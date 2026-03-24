/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe (UE)
    FILE:         FormButton.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    UE port of legacy FormButton control.
*/

#include "FormButton.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

UFormButton::UFormButton(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    Text = FText::FromString(TEXT("BUTTON"));
    bEnabled = true;
    Font = FSlateFontInfo();
    TextColor = FSlateColor(FLinearColor::White);
}

TSharedRef<SWidget> UFormButton::RebuildWidget()
{
    Label =
        SNew(STextBlock)
        .Text(Text)
        .Font(Font)
        .ColorAndOpacity(TextColor);

    Button =
        SNew(SButton)
        .IsEnabled(bEnabled)
        .OnClicked(FOnClicked::CreateUObject(this, &UFormButton::HandleClicked))
        [
            Label.ToSharedRef()
        ];

    return Button.ToSharedRef();
}

void UFormButton::SynchronizeProperties()
{
    Super::SynchronizeProperties();

    if (Button.IsValid())
        Button->SetEnabled(bEnabled);

    if (Label.IsValid())
    {
        Label->SetText(Text);
        Label->SetFont(Font);
        Label->SetColorAndOpacity(TextColor);
    }
}

FReply UFormButton::HandleClicked()
{
    OnClicked.Broadcast();
    return FReply::Handled();
}

void UFormButton::SetText(const FText& InText)
{
    Text = InText;
    if (Label.IsValid()) Label->SetText(Text);
}

void UFormButton::SetEnabled(bool bInEnabled)
{
    bEnabled = bInEnabled;
    if (Button.IsValid()) Button->SetEnabled(bEnabled);
}

void UFormButton::SetFont(const FSlateFontInfo& InFont)
{
    Font = InFont;
    if (Label.IsValid()) Label->SetFont(Font);
}

void UFormButton::SetTextColor(const FSlateColor& InColor)
{
    TextColor = InColor;
    if (Label.IsValid()) Label->SetColorAndOpacity(TextColor);
}
