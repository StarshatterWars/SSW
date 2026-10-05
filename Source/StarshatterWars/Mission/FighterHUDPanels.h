#pragma once
#include "Widgets/SLeafWidget.h"
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"
#include "Rendering/DrawElements.h"

// Fighter HUD frame layout from HUDView: two centered 256-pixel panels.
class SFighterHUDPanels : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SFighterHUDPanels) {} 
        SLATE_ATTRIBUTE(bool, ShowPanels)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        ShowPanels = Args._ShowPanels;
        LeftTexture.Reset(LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/HUD/HUDleft.HUDleft")));
        RightTexture.Reset(LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/HUD/HUDright.HUDright")));
        LeftBrush.SetResourceObject(LeftTexture.Get());
        RightBrush.SetResourceObject(RightTexture.Get());
        LeftBrush.DrawAs = RightBrush.DrawAs = ESlateBrushDrawType::Image;
        LeftBrush.ImageSize = RightBrush.ImageSize = FVector2D(256, 256);
        SetVisibility(EVisibility::HitTestInvisible);
        if (!LeftTexture.IsValid() || !RightTexture.IsValid())
            UE_LOG(LogTemp, Warning, TEXT("[FighterHUD] Missing HUDleft or HUDright texture in /Game/UI/HUD."));
    }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry,
        const FSlateRect& CullRect, FSlateWindowElementList& Elements, int32 Layer,
        const FWidgetStyle& Style, bool bParentEnabled) const override
    {
        if (!ShowPanels.Get()) return Layer;
        const FVector2D ViewSize = Geometry.GetLocalSize();
        const float Scale = FMath::Min(1.0, FMath::Min(ViewSize.X / 512.0, ViewSize.Y / 256.0));
        const FVector2f PanelSize(256.0f * Scale, 256.0f * Scale);
        const FVector2f Center(ViewSize.X * 0.5, ViewSize.Y * 0.5);
        const FLinearColor Blue(0.15f, 0.55f, 1.0f, 1.0f);
        if (LeftTexture.IsValid())
            FSlateDrawElement::MakeBox(Elements, Layer + 1,
                Geometry.ToPaintGeometry(PanelSize, FSlateLayoutTransform(Center - FVector2f(PanelSize.X, PanelSize.Y * 0.5f))),
                &LeftBrush, ESlateDrawEffect::None, Blue);
        if (RightTexture.IsValid())
            FSlateDrawElement::MakeBox(Elements, Layer + 1,
                Geometry.ToPaintGeometry(PanelSize, FSlateLayoutTransform(Center - FVector2f(0, PanelSize.Y * 0.5f))),
                &RightBrush, ESlateDrawEffect::None, Blue);
        return Layer + 1;
    }
private:
    TAttribute<bool> ShowPanels;
    TStrongObjectPtr<UTexture2D> LeftTexture;
    TStrongObjectPtr<UTexture2D> RightTexture;
    FSlateBrush LeftBrush;
    FSlateBrush RightBrush;
};
