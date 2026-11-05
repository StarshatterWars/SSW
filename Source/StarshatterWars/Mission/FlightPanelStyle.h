#pragma once

#include "Brushes/SlateRoundedBoxBrush.h"

namespace FlightPanelStyle
{
    inline const FSlateRoundedBoxBrush& Glass()
    {
        static const FSlateRoundedBoxBrush Brush(FLinearColor(.012f, .024f, .038f, .82f), 10.f);
        return Brush;
    }
}
