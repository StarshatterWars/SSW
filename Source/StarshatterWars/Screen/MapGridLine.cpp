// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "MapGridLine.h"
#include "Layout/Geometry.h"             // FGeometry
#include "Layout/PaintGeometry.h"        // ToPaintGeometry
#include "Styling/SlateColor.h"          // FSlateColor / FLinearColor
#include "Rendering/DrawElements.h"

void UMapGridLine::NativeConstruct()
{
}

void UMapGridLine::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{

}

void UMapGridLine::SetGalaxyData(const TArray<FS_Galaxy>& Systems, float InMapScale)
{
    GalaxySystems = Systems;
    MapScale = InMapScale;
    Invalidate(EInvalidateWidget::Paint);
}

int32 UMapGridLine::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
    int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    if (GalaxySystems.Num() == 0)
        return LayerId;

    float MinX = FLT_MAX, MaxX = -FLT_MAX;
    float MinY = FLT_MAX, MaxY = -FLT_MAX;

    for (const FS_Galaxy& System : GalaxySystems)
    {
        MinX = FMath::Min(MinX, System.Location.X);
        MaxX = FMath::Max(MaxX, System.Location.X);
        MinY = FMath::Min(MinY, System.Location.Y);
        MaxY = FMath::Max(MaxY, System.Location.Y);
    }

    // Snap to grid
    MinX = FMath::FloorToFloat(MinX / GridStep) * GridStep;
    MaxX = FMath::CeilToFloat(MaxX / GridStep) * GridStep;
    MinY = FMath::FloorToFloat(MinY / GridStep) * GridStep;
    MaxY = FMath::CeilToFloat(MaxY / GridStep) * GridStep;

    FVector2D Size = AllottedGeometry.GetLocalSize();
    FVector2D CenterOffset = Size * 0.5f;

    FLinearColor GridColor(0.2f, 0.2f, 0.2f, 0.5f);
    FVector2D Margin = { 120.0f, 0.f };

    const float Left = Margin.X;
    const float Right = Size.X - Margin.X;
    const float Top = Margin.Y;
    const float Bottom = Size.Y - Margin.Y;

    // Vertical lines
    for (float X = MinX; X <= MaxX; X += GridStep)
    {
        float PixelX = X * MapScale + CenterOffset.X;

        FVector2D Start(PixelX, 0);
        FVector2D End(PixelX, Size.Y);
        
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            { Start, End },
            ESlateDrawEffect::None,
            GridColor,
            true,
            1.f
        );
    }

    // Horizontal lines
    for (float Y = MinY; Y <= MaxY; Y += GridStep)
    {
        float PixelY = -Y * MapScale + CenterOffset.Y;

        FVector2D Start(0 + Margin.X, PixelY);
        FVector2D End(Size.X - Margin.X, PixelY);

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            { Start, End },
            ESlateDrawEffect::None,
            GridColor,
            true,
            1.f
        );
    }

    return LayerId + 1;
}
