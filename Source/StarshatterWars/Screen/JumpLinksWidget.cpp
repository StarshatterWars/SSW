// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "JumpLinksWidget.h"
#include "Layout/Geometry.h"             // FGeometry
#include "Layout/PaintGeometry.h"        // ToPaintGeometry
#include "Styling/SlateColor.h"          // FSlateColor / FLinearColor
#include "Rendering/DrawElements.h"

void UJumpLinksWidget::SetJumpLinks(const TArray<FJumpLink>& InLinks)
{
    JumpLinks = InLinks;
    Invalidate(EInvalidateWidget::Paint); // request redraw
}

int32 UJumpLinksWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
    int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    for (const FJumpLink& Link : JumpLinks)
    {
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            { Link.Start, Link.End },
            ESlateDrawEffect::None,
            FLinearColor::White,
            true,
            2.f
        );
    }

    return LayerId + 1;
}

