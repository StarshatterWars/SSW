#include "FighterHUDDetails.h"

#include "CameraManager.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"
#include "SceneView.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "ShipActor.h"
#include "SimContact.h"
#include "SSWRuntimeSubsystem.h"
#include "Styling/CoreStyle.h"

#include <array>

namespace
{
bool ClipTrackSegment(FVector4& Start, FVector4& End)
{
    // Clip in homogeneous coordinates before dividing by W, including when
    // the contact is offscreen and its trail crosses the visible view.
    const auto Distances = [](const FVector4& Point)
    {
        return std::array<double, 5>{
            Point.W - 1.0, Point.W + Point.X, Point.W - Point.X,
            Point.W + Point.Y, Point.W - Point.Y};
    };
    const auto StartDistances = Distances(Start);
    const auto EndDistances = Distances(End);
    double First = 0.0;
    double Last = 1.0;
    for (int32 Plane = 0; Plane < 5; ++Plane)
    {
        const double A = StartDistances[Plane];
        const double B = EndDistances[Plane];
        if (A < 0.0 && B < 0.0) return false;
        if (A < 0.0) First = FMath::Max(First, A / (A - B));
        if (B < 0.0) Last = FMath::Min(Last, A / (A - B));
    }
    if (First >= Last) return false;

    const FVector4 Delta = End - Start;
    End = Start + Delta * Last;
    Start += Delta * First;
    return true;
}
}

void SFighterHUDDetails::DrawSensorTracks(Ship& Player, const FGeometry& Geometry,
    FSlateWindowElementList& Elements, int32 Layer) const
{
    const auto* Presentation = Player.GetNativePresentation();
    UWorld* World = IsValid(Presentation) ? Presentation->GetWorld() : nullptr;
    const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
    const ULocalPlayer* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
    const auto* Runtime = World && World->GetGameInstance()
        ? World->GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>() : nullptr;
    SimRegion* Region = Player.GetRegion();
    FSceneViewProjectionData Projection;
    if (!Runtime || !Region || !LocalPlayer || !LocalPlayer->ViewportClient ||
        !LocalPlayer->ViewportClient->Viewport ||
        !LocalPlayer->GetProjectionData(LocalPlayer->ViewportClient->Viewport, Projection))
        return;

    int32 Width, Height;
    Controller->GetViewportSize(Width, Height);
    if (Width <= 0 || Height <= 0) return;

    const FVector2D Size = Geometry.GetLocalSize();
    const FIntRect Rect = Projection.GetConstrainedViewRect();
    const FMatrix ViewProjection = Projection.ComputeViewProjectionMatrix();
    const auto ToClip = [&](const FVector& Point)
    {
        return ViewProjection.TransformFVector4(FVector4(Runtime->GetVisualLocationForRegion(Region, Point), 1.0));
    };
    const auto ToWidget = [&](const FVector4& Point)
    {
        return FVector2D(
            (Rect.Min.X + (0.5 + 0.5 * Point.X / Point.W) * Rect.Width()) * Size.X / Width,
            (Rect.Min.Y + (0.5 - 0.5 * Point.Y / Point.W) * Rect.Height()) * Size.Y / Height);
    };
    const auto DrawTrack = [&](const auto& Track, const FVector& Location, FLinearColor Color)
    {
        const int32 Count = Track.TrackLength();
        FVector4 Previous = ToClip(Location);
        for (int32 Index = 0; Index < Count; ++Index)
        {
            const FVector4 Current = ToClip(Track.TrackPoint(Index));
            FVector4 Start = Previous;
            FVector4 End = Current;
            Previous = Current;
            if (!ClipTrackSegment(Start, End)) continue;

            const FVector2D A = ToWidget(Start);
            const FVector2D B = ToWidget(End);
            if (A.ContainsNaN() || B.ContainsNaN() || A.Equals(B, 0.25)) continue;

            FLinearColor SegmentColor = Color;
            SegmentColor.A *= 1.0f - static_cast<float>(FMath::Max(0, Index - 1)) / Count;
            FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(),
                {A, B}, ESlateDrawEffect::None, SegmentColor, true, 1.0f);
        }
    };

    ListIter<SimContact> ContactIterator = Player.GetContactList();
    while (++ContactIterator)
    {
        const SimContact* Contact = ContactIterator.value();
        const Ship* ContactShip = Contact ? Contact->GetShip() : nullptr;
        if (!ContactShip || ContactShip == &Player || ContactShip->GetRegion() != Region || ContactShip->IsDead())
            continue;

        FLinearColor Color(Ship::IFFColor(Contact->GetIFF(&Player)));
        Color.A *= FMath::Clamp(Contact->Age(), 0.0, 1.0);
        if (Color.A <= 0.0f) continue;
        DrawTrack(*Contact, Contact->Location(), Color);

        const ShipDesign* Design = ContactShip->Design();
        const FVector4 Clip = ToClip(Contact->Location());
        if (!Design || Design->integrity <= 0.0 || Clip.W <= 1.0 ||
            FMath::Abs(Clip.X) > Clip.W || FMath::Abs(Clip.Y) > Clip.W) continue;
        const FVector2D Center = ToWidget(Clip);
        if (Center.ContainsNaN()) continue;
        const FVector2f Position(Center.X - 19.0, Center.Y - 23.0);
        const auto* Brush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
        const auto Box = [&](int32 DrawLayer, float BarWidth, FLinearColor Tint)
        {
            FSlateDrawElement::MakeBox(Elements, DrawLayer,
                Geometry.ToPaintGeometry(FVector2f(BarWidth, 4.0f), FSlateLayoutTransform(Position)),
                Brush, ESlateDrawEffect::None, Tint);
        };
        Box(Layer + 1, 38.0f, FLinearColor(0.02f, 0.03f, 0.04f, Color.A * 0.85f));
        Box(Layer + 2, 38.0f * FMath::Clamp(ContactShip->GetIntegrity() / Design->integrity, 0.0, 1.0), Color);
    }

    if (CameraManager::GetCameraMode() == CameraManager::MODE_ORBIT && Player.TrackLength() > 0)
        DrawTrack(Player, Player.GetLocation(), FLinearColor(Player.MarkerColor()));
}
