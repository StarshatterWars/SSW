#pragma once
#include "Widgets/SLeafWidget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "SceneView.h"
#include "Camera/PlayerCameraManager.h"
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
    TArray<AActor*> VisualActors;
    Actor->GetAttachedActors(VisualActors, false, true);
    VisualActors.Add(Actor);
    TSet<UMeshComponent*> Seen;
    for (AActor* Visual : VisualActors)
    {
        if (!IsValid(Visual)) continue;
        TArray<UMeshComponent*> Meshes;
        Visual->GetComponents<UMeshComponent>(Meshes, true);
        for (UMeshComponent* Mesh : Meshes)
        {
            if (!IsValid(Mesh) || Seen.Contains(Mesh) || !Mesh->IsVisible() || Mesh->bHiddenInGame) continue;
            Seen.Add(Mesh);
            Bounds += Mesh->CalcBounds(Mesh->GetComponentTransform().GetRelativeTransform(Actor->GetActorTransform())).GetBox();
        }
    }
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
        const TCHAR* Names[]={TEXT("CHASE_L"),TEXT("CHASE_R"),TEXT("CHASE_T"),TEXT("CHASE_B")};
        for (int32 I=0;I<4;++I)
        {
            const FString Path=FString::Printf(TEXT("/Game/UI/HUD/%s.%s"),Names[I],Names[I]);
            ChaseTextures[I].Reset(LoadObject<UTexture2D>(nullptr,*Path));
            ChaseBrushes[I].SetResourceObject(ChaseTextures[I].Get());
            ChaseBrushes[I].DrawAs=ESlateBrushDrawType::Image;
            if (ChaseTextures[I].IsValid()) ChaseBrushes[I].ImageSize=FVector2D(ChaseTextures[I]->GetSizeX(),ChaseTextures[I]->GetSizeY());
        }
        ForceVolatile(true);
        SetVisibility(EVisibility::HitTestInvisible);
    }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry,
        const FSlateRect& CullRect, FSlateWindowElementList& Elements, int32 Layer,
        const FWidgetStyle& Style, bool bParentEnabled) const override
    {
        APlayerController* PC = Controller.Get();
        AActor* Actor = Target.Get().Get();
        if (!PC || !IsValid(Actor) || (Actor->IsHidden() && !Actor->ActorHasTag(TEXT("MissionNavigationTarget")))) return Layer;
        int32 Width = 0, Height = 0;
        PC->GetViewportSize(Width, Height);
        if (Width <= 0 || Height <= 0) return Layer;
        const FBox CachedBounds = LocalBounds.Get();
        if (!CachedBounds.IsValid) return Layer;
        const FBox Bounds = CachedBounds.TransformBy(Actor->GetActorTransform());
        // Navigation markers represent the actor origin, not animated BP mesh bounds.
        const FVector TargetPosition = Actor->ActorHasTag(TEXT("MissionNavigationTarget"))
            ? Actor->GetActorLocation() : Bounds.GetCenter();
        ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
        FSceneViewProjectionData ProjectionData;
        if (!LocalPlayer || !LocalPlayer->ViewportClient || !LocalPlayer->ViewportClient->Viewport ||
            !LocalPlayer->GetProjectionData(LocalPlayer->ViewportClient->Viewport, ProjectionData)) return Layer;
        const FVector4 Clip = ProjectionData.ComputeViewProjectionMatrix().TransformFVector4(FVector4(TargetPosition, 1.0));
        const FIntRect Rect = ProjectionData.GetConstrainedViewRect();
        const FVector2D Size=Geometry.GetLocalSize();
        const FVector2D ViewCenter((Rect.Min.X+Rect.Width()*0.5)*Size.X/Width,
                                   (Rect.Min.Y+Rect.Height()*0.5)*Size.Y/Height);
        const double HudScale=FMath::Min(Size.X/1280.0,Size.Y/720.0);
        const FVector2D Half(Rect.Width()*0.5*Size.X/Width-42*HudScale,
                             Rect.Height()*0.5*Size.Y/Height-42*HudScale);
        if (Half.X<=0 || Half.Y<=0) return Layer;
        const double Divisor=FMath::Max(FMath::Abs(Clip.W),1.e-6);
        FVector2D Direction(Clip.X/Divisor*Rect.Width()*0.5*Size.X/Width,
                           -Clip.Y/Divisor*Rect.Height()*0.5*Size.Y/Height);
        FVector2D Center=ViewCenter+Direction;
        if (LastTarget.Get()!=Actor) { LastTarget=Actor; LastOffscreenDirection=FVector2D(0,1); }
        if (Clip.W<=0 || FMath::Abs(Direction.X)>Half.X || FMath::Abs(Direction.Y)>Half.Y)
        {
            // Both marker modes use the SAME projection and inset boundary.
            // At directly aft, the bearing is undefined: retain the last cue
            // instead of flipping sides in response to floating-point noise.
            if (Direction.SizeSquared()<FMath::Square(4.0*HudScale)) Direction=LastOffscreenDirection;
            else LastOffscreenDirection=Direction.GetSafeNormal();
            const double EdgeScale=FMath::Min(Half.X/FMath::Max(FMath::Abs(Direction.X),1.e-6),
                                             Half.Y/FMath::Max(FMath::Abs(Direction.Y),1.e-6));
            const FVector2D Edge=Direction*EdgeScale;
            Center=ViewCenter+Edge;
            const bool Side=FMath::Abs(Edge.X)/Half.X>=FMath::Abs(Edge.Y)/Half.Y;
            const int32 Index=Side?(Direction.X<0?0:1):(Direction.Y<0?2:3);
            const FLinearColor Color=TargetColor.Get();
            const FVector2D Native=ChaseBrushes[Index].ImageSize;
            const FVector2D D=Native*(32.0*HudScale/FMath::Max(1.0,FMath::Max(Native.X,Native.Y)));
            if (ChaseTextures[Index].IsValid())
                FSlateDrawElement::MakeBox(Elements,Layer+1,Geometry.ToPaintGeometry(FVector2f(D),
                    FSlateLayoutTransform(FVector2f(Center-D*0.5))),&ChaseBrushes[Index],ESlateDrawEffect::None,Color);
            const FSlateFontInfo Font=FCoreStyle::GetDefaultFontStyle("Bold",FMath::Max(8,int32(12*HudScale)));
            const FText Label=TargetName.Get();
            const FVector2D Extent=FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Label,Font);
            const FVector2D Position(FMath::Clamp(Center.X-Extent.X*0.5,0.0,FMath::Max(0.0,Size.X-Extent.X)),
                FMath::Clamp(Center.Y+(Index==3?-Extent.Y-22*HudScale:22*HudScale),0.0,FMath::Max(0.0,Size.Y-Extent.Y)));
            FSlateDrawElement::MakeText(Elements,Layer+2,Geometry.ToPaintGeometry(FVector2f(Extent),
                FSlateLayoutTransform(FVector2f(Position))),Label,Font,ESlateDrawEffect::None,Color);
            return Layer+2;
        }
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
    mutable TWeakObjectPtr<AActor> LastTarget;
    mutable FVector2D LastOffscreenDirection = FVector2D(0,1);
    TStrongObjectPtr<UTexture2D> ReticleTexture;
    FSlateBrush ReticleBrush;
    TStrongObjectPtr<UTexture2D> ChaseTextures[4];
    FSlateBrush ChaseBrushes[4];
    TWeakObjectPtr<APlayerController> Controller;
    TAttribute<TWeakObjectPtr<AActor>> Target;
    TAttribute<FText> TargetName;
    TAttribute<FLinearColor> TargetColor;
    TAttribute<FBox> LocalBounds;
};
