/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    SSW
    FILE:         SystemMarker.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Galaxy system marker widget.
*/

#include "SystemMarker.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

namespace
{
    static FLinearColor GetEmpireColor(EEMPIRE_NAME Empire)
    {
        switch (Empire)
        {
        case EEMPIRE_NAME::Terellian:
            return FLinearColor::Green;

        case EEMPIRE_NAME::Marakan:
            return FLinearColor::Red;

        case EEMPIRE_NAME::Independent:
        case EEMPIRE_NAME::Neutral:
            return FLinearColor::Gray;

        default:
            return FLinearColor::Gray;
        }
    }

    static FLinearColor GetIffTint(int32 Iff)
    {
        switch (Iff)
        {
        case 0:  return FLinearColor::Gray;
        case 1:  return FLinearColor::Green;
        case 2:  return FLinearColor::Red;
        default: return FLinearColor::Gray;
        }
    }
}

void USystemMarker::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogTemp, Log,
        TEXT("USystemMarker::NativeConstruct() StarImage=%s SystemNameText=%s IffImage=%s HighlightBorder=%s"),
        StarImage ? TEXT("VALID") : TEXT("NULL"),
        SystemNameText ? TEXT("VALID") : TEXT("NULL"),
        IffImage ? TEXT("VALID") : TEXT("NULL"),
        HighlightBorder ? TEXT("VALID") : TEXT("NULL"));
}

void USystemMarker::Init(const FS_Galaxy& System, UTexture2D* InStarTexture)
{
    UE_LOG(LogTemp, Log, TEXT("USystemMarker::Init() Creating Widget: %s"), *System.Name);

    SystemData = System;
    SystemName = System.Name;
    Tint = GetIffTint(System.Iff);

    SetToolTipText(FText::FromString(System.Name));

    const FLinearColor EmpireColor = GetEmpireColor(System.Empire);

    if (SystemNameText)
    {
        SystemNameText->SetText(FText::FromString(System.Name));
        SystemNameText->SetColorAndOpacity(EmpireColor);
        SystemNameText->SetVisibility(ESlateVisibility::Hidden);
    }

    if (IffImage)
    {
        IffImage->SetColorAndOpacity(Tint);
    }

    if (InStarTexture && StarImage)
    {
        const FSlateBrush Brush = CreateBrushFromTexture(
            InStarTexture,
            FVector2D((float)InStarTexture->GetSizeX(), (float)InStarTexture->GetSizeY()));

        StarImage->SetBrush(Brush);
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("USystemMarker::Init() Missing star texture or StarImage for %s"),
            *System.Name);
    }

    SetSelected(false);
}

FSlateBrush USystemMarker::CreateBrushFromTexture(UTexture2D* Texture, FVector2D ImageSize) const
{
    FSlateBrush Brush;

    if (!Texture)
    {
        return Brush;
    }

    Brush.SetResourceObject(Texture);
    Brush.ImageSize = ImageSize;
    Brush.DrawAs = ESlateBrushDrawType::Image;

    return Brush;
}

void USystemMarker::SetSelected(bool bIsSelected)
{
    if (HighlightBorder)
    {
        HighlightBorder->SetVisibility(
            bIsSelected ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }

    if (IffImage)
    {
        IffImage->SetColorAndOpacity(
            bIsSelected ? FLinearColor::Yellow : Tint);
    }

    if (SystemNameText)
    {
        SystemNameText->SetVisibility(
            bIsSelected ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }

    if (bIsSelected)
    {
        PlayGlow();
    }
    else
    {
        StopGlow();
    }
}

FReply USystemMarker::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        OnClicked.ExecuteIfBound(SystemData.Name);
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}