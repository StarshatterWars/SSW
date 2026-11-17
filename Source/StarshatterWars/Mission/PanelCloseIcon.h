#pragma once
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"

// A geometric close mark: independent of font coverage and source text encoding.
class SPanelCloseIcon : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SPanelCloseIcon) {} SLATE_END_ARGS()
    void Construct(const FArguments&) { SetVisibility(EVisibility::HitTestInvisible); }
    FVector2D ComputeDesiredSize(float) const override { return FVector2D(18, 18); }
    int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
        FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool ParentEnabled) const override
    {
        const FVector2D Size = Geometry.GetLocalSize();
        const float Half = FMath::Max(0.f, FMath::Min(6.f, float(FMath::Min(Size.X, Size.Y) * .5 - 2)));
        const FVector2f Center(Size * .5);
        const FLinearColor Color = FLinearColor::White * Style.GetColorAndOpacityTint();
        const ESlateDrawEffect Effect = ParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
        for (float Direction : {-1.f, 1.f})
        {
            TArray<FVector2f> Points = {Center + FVector2f(-Half, -Half * Direction), Center + FVector2f(Half, Half * Direction)};
            FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Points, Effect, Color, true, 2.f);
        }
        return Layer;
    }
};
