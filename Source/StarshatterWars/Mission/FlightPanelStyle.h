#pragma once

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace FlightPanelStyle
{
    inline constexpr double CenterPanelWidth = 326;
    inline constexpr float RightPanelTop = 16.f;
    inline constexpr float RightPanelBottom = .67f;
    inline constexpr float RightPanelInset = 16.f;
    inline FLinearColor DividerColor()
    {
        return FLinearColor(.18f, .45f, .63f, .65f);
    }

    inline const FSlateRoundedBoxBrush& Glass()
    {
        static const FSlateRoundedBoxBrush Brush(FLinearColor(.012f, .024f, .038f, .82f), 10.f);
        return Brush;
    }

    inline const FSlateRoundedBoxBrush& Window()
    {
        static const FSlateRoundedBoxBrush Brush(FLinearColor(.017f, .032f, .049f, 1.f), 8.f);
        return Brush;
    }

    inline const FSlateRoundedBoxBrush& TitleBar()
    {
        static const FSlateRoundedBoxBrush Brush(FLinearColor(.045f, .088f, .125f, 1.f), 6.f);
        return Brush;
    }

    inline void PaintGlass(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
        FVector2D Origin, FVector2D Size)
    {
        const auto& Brush = Glass();
        FSlateDrawElement::MakeBox(Elements, Layer, Geometry.ToPaintGeometry(FVector2f(Size),
            FSlateLayoutTransform(FVector2f(Origin))), &Brush, ESlateDrawEffect::None,
            Brush.TintColor.GetSpecifiedColor());
    }

    inline void PaintHeader(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
        FVector2D Origin, double Width, float Scale, const FString& Caption, FLinearColor Tint)
    {
        FSlateDrawElement::MakeText(Elements, Layer, Geometry.ToPaintGeometry(FVector2f(1, 1),
            FSlateLayoutTransform(FVector2f(Origin + FVector2D(12, 8) * Scale))), Caption,
            FCoreStyle::GetDefaultFontStyle("Bold", FMath::Max(6, int32(11 * Scale))), ESlateDrawEffect::None, Tint);
        FSlateDrawElement::MakeBox(Elements, Layer, Geometry.ToPaintGeometry(FVector2f((Width - 24) * Scale, Scale),
            FSlateLayoutTransform(FVector2f(Origin + FVector2D(12, 24) * Scale))),
            FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, DividerColor());
    }
}
