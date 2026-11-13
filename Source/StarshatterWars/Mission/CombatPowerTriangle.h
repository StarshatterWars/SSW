#pragma once

#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Ship.h"
#include "ShipSystemIcon.h"

class SCombatPowerTriangle : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SCombatPowerTriangle) {}
        SLATE_ARGUMENT(TFunction<Ship*()>, ResolveShip)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args)
    {
        ResolveShip = Args._ResolveShip;
        ForceVolatile(true);
    }

    FVector2D ComputeDesiredSize(float) const override { return FVector2D(300, 132); }

    FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override
    {
        if (Event.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
        Update(Geometry, Event);
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }

    FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override
    {
        if (!HasMouseCapture()) return FReply::Unhandled();
        Update(Geometry, Event);
        return FReply::Handled();
    }

    FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent& Event) override
    {
        return Event.GetEffectingButton() == EKeys::LeftMouseButton && HasMouseCapture()
            ? FReply::Handled().ReleaseMouseCapture() : FReply::Unhandled();
    }

    FReply OnMouseButtonDoubleClick(const FGeometry&, const FPointerEvent& Event) override
    {
        if (Event.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
        if (Ship* Player = ResolveShip()) Player->SetCombatAllocation(FVector(1));
        return FReply::Handled();
    }

    int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
        FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle&, bool) const override
    {
        Ship* Player = ResolveShip();
        if (!Player) return Layer;
        const FVector2D Size = Geometry.GetLocalSize();
        const FVector2D A(48, 34), B(Size.X - 48, 34), C(Size.X * 0.5, Size.Y - 26);
        const FVector Shares = Player->GetCombatAllocation();
        const FLinearColor Cyan(0.28f, 0.8f, 1.0f);
        FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(),
            {A, B, C, A}, ESlateDrawEffect::None, Cyan * 0.65f, true, 1.5f);
        const FVector2D Marker = A * Shares.X + B * Shares.Y + C * Shares.Z;
        const auto* Brush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
        FSlateDrawElement::MakeBox(Elements, Layer + 1, Geometry.ToPaintGeometry(FVector2f(8, 8),
            FSlateLayoutTransform(FVector2f(Marker - FVector2D(4, 4)))), Brush, ESlateDrawEffect::None, Cyan);

        const EShipSystemIcon Icons[] = {EShipSystemIcon::Engine, EShipSystemIcon::Weapon, EShipSystemIcon::Shield};
        const FVector2D Positions[] = {{30, 3}, {Size.X - 70, 3}, {Size.X * 0.5 - 20, Size.Y - 22}};
        for (int Index = 0; Index < 3; ++Index)
        {
            SShipSystemIcon::PaintIcon(Elements, Layer + 1, Geometry, Positions[Index], Icons[Index], Cyan);
            const FString Text = FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Shares[Index] * 100));
            FSlateDrawElement::MakeText(Elements, Layer + 1, Geometry.ToPaintGeometry(FVector2f(104, 18),
                FSlateLayoutTransform(FVector2f(Positions[Index] + FVector2D(25, 4)))), Text,
                FCoreStyle::GetDefaultFontStyle("Bold", 12), ESlateDrawEffect::None, Cyan);
        }
        return Layer + 1;
    }

private:
    void Update(const FGeometry& Geometry, const FPointerEvent& Event)
    {
        Ship* Player = ResolveShip();
        if (!Player) return;
        const FVector2D Size = Geometry.GetLocalSize();
        if (Size.X <= 96 || Size.Y <= 60) return;
        const FVector2D Point = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
        const double Shields = FMath::Clamp((Point.Y - 34.0) / (Size.Y - 60.0), 0.0, 1.0);
        const double Weapons = FMath::Clamp((Point.X - 48.0 - Shields * (Size.X * 0.5 - 48.0)) /
            (Size.X - 96.0), 0.0, 1.0 - Shields);
        Player->SetCombatAllocation(FVector(1.0 - Weapons - Shields, Weapons, Shields));
    }

    TFunction<Ship*()> ResolveShip;
};
