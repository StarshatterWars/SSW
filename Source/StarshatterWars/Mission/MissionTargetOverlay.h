#pragma once
#include "Widgets/SLeafWidget.h"
#include "GameFramework/PlayerController.h"
#include "Components/MeshComponent.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

// Bounds shared by framing and the target box; omit particles and invisible meshes.
inline FBox MissionTargetLocalBounds(AActor* Actor)
{
    FBox Bounds(ForceInit);
    if (!IsValid(Actor)) return Bounds;
    TArray<UMeshComponent*> Meshes;
    Actor->GetComponents<UMeshComponent>(Meshes, true);
    for (UMeshComponent* Mesh : Meshes)
        if (IsValid(Mesh) && Mesh->IsVisible() && !Mesh->bHiddenInGame)
            Bounds += Mesh->CalcBounds(Mesh->GetComponentTransform().GetRelativeTransform(Actor->GetActorTransform())).GetBox();
    if (!Bounds.IsValid)
        Bounds = FBox::BuildAABB(FVector::ZeroVector, FVector(100.0));
    return Bounds;
}

class SMissionTargetOverlay : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SMissionTargetOverlay) {}
        SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controller)
        SLATE_ATTRIBUTE(TWeakObjectPtr<AActor>, Target)
        SLATE_ATTRIBUTE(FText, TargetName)
        SLATE_ATTRIBUTE(FBox, LocalBounds)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Controller = Args._Controller;
        Target = Args._Target;
        TargetName = Args._TargetName;
        LocalBounds = Args._LocalBounds;
        SetVisibility(EVisibility::HitTestInvisible);
    }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry,
        const FSlateRect& CullRect, FSlateWindowElementList& Elements, int32 Layer,
        const FWidgetStyle& Style, bool bParentEnabled) const override
    {
        APlayerController* PC = Controller.Get();
        AActor* Actor = Target.Get().Get();
        if (!PC || !IsValid(Actor) || Actor->IsHidden()) return Layer;
        int32 Width = 0, Height = 0;
        PC->GetViewportSize(Width, Height);
        if (Width <= 0 || Height <= 0) return Layer;
        const FBox CachedBounds = LocalBounds.Get();
        if (!CachedBounds.IsValid) return Layer;
        const FBox Bounds = CachedBounds.TransformBy(Actor->GetActorTransform());
        FVector2D Min(FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX);
        for (int32 Corner = 0; Corner < 8; ++Corner)
        {
            FVector Point((Corner & 1) ? Bounds.Max.X : Bounds.Min.X,
                (Corner & 2) ? Bounds.Max.Y : Bounds.Min.Y,
                (Corner & 4) ? Bounds.Max.Z : Bounds.Min.Z);
            FVector2D Pixel;
            if (!PC->ProjectWorldLocationToScreen(Point, Pixel, false)) return Layer;
            Pixel.X *= Geometry.GetLocalSize().X / Width;
            Pixel.Y *= Geometry.GetLocalSize().Y / Height;
            Min.X = FMath::Min(Min.X, Pixel.X); Min.Y = FMath::Min(Min.Y, Pixel.Y);
            Max.X = FMath::Max(Max.X, Pixel.X); Max.Y = FMath::Max(Max.Y, Pixel.Y);
        }
        Min -= FVector2D(8,8); Max += FVector2D(8,8);
        const FVector2D Size = Geometry.GetLocalSize();
        if (Max.X < 0 || Max.Y < 0 || Min.X > Size.X || Min.Y > Size.Y) return Layer;
        Min.X = FMath::Clamp(Min.X, 2.0, FMath::Max(2.0, Size.X - 2));
        Min.Y = FMath::Clamp(Min.Y, 2.0, FMath::Max(2.0, Size.Y - 2));
        Max.X = FMath::Clamp(Max.X, Min.X, FMath::Max(Min.X, Size.X - 2));
        Max.Y = FMath::Clamp(Max.Y, Min.Y, FMath::Max(Min.Y, Size.Y - 2));
        TArray<FVector2D> Points = { Min, FVector2D(Max.X,Min.Y), Max, FVector2D(Min.X,Max.Y), Min };
        const FLinearColor Color(0.2f, 1.0f, 0.3f, 1.0f);
        FSlateDrawElement::MakeLines(Elements, Layer + 1, Geometry.ToPaintGeometry(), Points,
            ESlateDrawEffect::None, Color, true, 2.0f);
        FSlateDrawElement::MakeText(Elements, Layer + 2,
            Geometry.ToPaintGeometry(FVector2f(300,24), FSlateLayoutTransform(FVector2f(Min.X, FMath::Max(0.0,Min.Y-24)))),
            TargetName.Get(), FCoreStyle::GetDefaultFontStyle("Bold",14), ESlateDrawEffect::None, Color);
        return Layer + 2;
    }
private:
    TWeakObjectPtr<APlayerController> Controller;
    TAttribute<TWeakObjectPtr<AActor>> Target;
    TAttribute<FText> TargetName;
    TAttribute<FBox> LocalBounds;
};
