#pragma once

#include "Brushes/SlateRoundedBoxBrush.h"

namespace FlightPanelStyle
{
    inline FLinearColor DividerColor()
    {
        return FLinearColor(.18f, .45f, .63f, .65f);
    }

    inline const FSlateRoundedBoxBrush& Glass()
    {
        static const FSlateRoundedBoxBrush Brush(FLinearColor(.012f, .024f, .038f, .82f), 10.f);
        return Brush;
    }
}
