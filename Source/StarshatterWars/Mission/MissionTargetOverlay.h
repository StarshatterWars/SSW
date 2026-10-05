#pragma once
#include "Widgets/SLeafWidget.h"
#include "GameFramework/PlayerController.h"
#include "Components/MeshComponent.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"

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
    SLATE_BEGIN_ARGS(SMissionTargetOverlay) : _TargetColor(FLinearColor::Yellow) {}
        SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controller)
        SLATE_ATTRIBUTE(TWeakObjectPtr<AActor>, Target)
        SLATE_ATTRIBUTE(FText, TargetName)
        SLATE_ATTRIBUTE(FLinearColor, TargetColor)
        SLATE_ATTRIBUTE(FBox, LocalBounds)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Controller = Args._Controller;
        Target = Args._Target;
        TargetName = Args._TargetName;
        TargetColor = Args._TargetColor;
        LocalBounds = Args._LocalBounds;
        ReticleTexture.Reset(LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/HUD/lead.lead")));
        ReticleBrush.SetResourceObject(ReticleTexture.Get());
        ReticleBrush.DrawAs = ESlateBrushDrawType::Image;
        ReticleBrush.ImageSize = FVector2D(64.0, 64.0);
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
        FVector2D Pixel;
        if (!PC->ProjectWorldLocationToScreen(Bounds.GetCenter(), Pixel, false)) return Layer;
        const FVector2D Size = Geometry.GetLocalSize();
        const FVector2D Center(Pixel.X * Size.X / Width, Pixel.Y * Size.Y / Height);
        if (Center.X < 0 || Center.Y < 0 || Center.X > Size.X || Center.Y > Size.Y) return Layer;
        // Scale viewport pixels into this widget's local coordinates (including DPI).
        const FVector2D ReticleSize(64.0 * Size.X / Width, 64.0 * Size.Y / Height);
        const FVector2D TopLeft = Center - ReticleSize * 0.5;
        const FLinearColor Color = TargetColor.Get();
        if (ReticleTexture.IsValid())
            FSlateDrawElement::MakeBox(Elements, Layer + 1,
                Geometry.ToPaintGeometry(FVector2f(ReticleSize), FSlateLayoutTransform(FVector2f(TopLeft))),
                &ReticleBrush, ESlateDrawEffect::None, Color);
        const FText Label = TargetName.Get();
        const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", 14);
        const FVector2D TextSize = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Label, Font);
        const FVector2D TextPosition(Center.X - TextSize.X * 0.5,
            FMath::Max(0.0, TopLeft.Y - TextSize.Y - 6.0));
        FSlateDrawElement::MakeText(Elements, Layer + 2,
            Geometry.ToPaintGeometry(FVector2f(TextSize), FSlateLayoutTransform(FVector2f(TextPosition))),
            Label, Font, ESlateDrawEffect::None, Color);
        return Layer + 2;
    }
private:
    TStrongObjectPtr<UTexture2D> ReticleTexture;
    FSlateBrush ReticleBrush;
    TWeakObjectPtr<APlayerController> Controller;
    TAttribute<TWeakObjectPtr<AActor>> Target;
    TAttribute<FText> TargetName;
    TAttribute<FLinearColor> TargetColor;
    TAttribute<FBox> LocalBounds;
};
