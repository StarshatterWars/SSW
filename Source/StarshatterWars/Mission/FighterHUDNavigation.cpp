#include "FighterHUDDetails.h"

#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Game.h"
#include "GameFramework/PlayerController.h"
#include "Instruction.h"
#include "Rendering/DrawElements.h"
#include "SceneView.h"
#include "Ship.h"
#include "ShipActor.h"
#include "SSWRuntimeSubsystem.h"
#include "Styling/CoreStyle.h"

void SFighterHUDDetails::DrawFlightNavigation(Ship& Player, const FGeometry& Geometry,
    FSlateWindowElementList& Elements, int32 Layer) const
{
    const bool bNavigation = FlightHUDMode == EFlightHUDMode::Navigation;
    const bool bFlightPath = !ThirdPerson.Get() && Player.GetVelocity().SizeSquared() > FMath::Square(50.0);
    if (!bNavigation && !bFlightPath) return;

    const AShipActor* Presentation = Player.GetNativePresentation();
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
    const FVector2D ViewScale(Size.X / Width, Size.Y / Height);
    const FVector2D HalfView = FVector2D(Rect.Width(), Rect.Height()) * ViewScale * 0.5;
    const FVector2D Center = FVector2D(Rect.Min) * ViewScale + HalfView;
    const FMatrix Matrix = Projection.ComputeViewProjectionMatrix();
    const FLinearColor ActiveColor(0.35f, 1.0f, 0.65f, 1.0f);
    const auto Project = [&](const FVector& Position)
    {
        return Matrix.TransformFVector4(FVector4(Runtime->GetVisualLocationForRegion(Region, Position), 1.0));
    };
    const auto IsOnScreen = [](const FVector4& Clip)
    {
        return Clip.W > 1.0 && FMath::Abs(Clip.X) < Clip.W && FMath::Abs(Clip.Y) < Clip.W;
    };
    const auto DrawLine = [&](const TArray<FVector2D>& Points, const FLinearColor& Color)
    {
        FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Points,
            ESlateDrawEffect::None, Color, true, 1.25f);
    };
    if (bFlightPath)
    {
        const FVector4 Clip = Project(Player.GetLocation() + Player.GetVelocity().GetSafeNormal() * 1e8);
        if (IsOnScreen(Clip))
        {
            const FVector2D At = Center + FVector2D(Clip.X, -Clip.Y) / Clip.W * HalfView;
            TArray<FVector2D> Ring;
            for (int32 Index = 0; Index <= 16; ++Index)
            {
                const double Angle = 2.0 * PI * Index / 16.0;
                Ring.Add(At + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * 7.0);
            }
            DrawLine(Ring, ActiveColor);
            DrawLine({At+FVector2D(-15,0), At+FVector2D(-7,0)}, ActiveColor);
            DrawLine({At+FVector2D(7,0), At+FVector2D(15,0)}, ActiveColor);
            DrawLine({At+FVector2D(0,-12), At+FVector2D(0,-7)}, ActiveColor);
        }
    }

    if (!bNavigation) return;
    const Instruction* Next = Player.GetNextNavPoint();
    ListIter<Instruction> Route = Player.GetFlightPlan();
    for (int32 Index = 1; ++Route; ++Index)
    {
        const Instruction* Point = Route.value();
        if (!Point || Point->GetRegion() != Region) continue;
        const bool bNext = Point == Next;
        const FVector4 Clip = Project(OtherHand(Point->GetLocation()));
        const FLinearColor Color = bNext ? ActiveColor : Point->GetStatus() > INSTRUCTION_STATUS::ACTIVE
            ? FLinearColor(.27f,.48f,.44f,.75f) : FLinearColor(.37f,.77f,.73f,.95f);
        const FVector2D Direction(Clip.X, -Clip.Y);
        FVector2D Label;
        if (IsOnScreen(Clip))
        {
            const FVector2D At = Center + Direction / Clip.W * HalfView;
            DrawLine({At+FVector2D(0,-6), At+FVector2D(6,0), At+FVector2D(0,6),
                At+FVector2D(-6,0), At+FVector2D(0,-6)}, Color);
            Label = At + FVector2D(9,-9);
        }
        else if (bNext && !Direction.ContainsNaN())
        {
            const FVector2D Normal = Direction.IsNearlyZero() ? FVector2D(0,1) : Direction.GetSafeNormal();
            const FVector2D Edge = HalfView - FVector2D(65,36);
            const double Distance = FMath::Min(Edge.X / FMath::Max(FMath::Abs(Normal.X), 0.001),
                Edge.Y / FMath::Max(FMath::Abs(Normal.Y), 0.001));
            if (Distance <= 0) continue;
            const FVector2D At = Center + Normal * Distance;
            const FVector2D Side(-Normal.Y, Normal.X);
            DrawLine({At-Normal*11+Side*6, At, At-Normal*11-Side*6}, Color);
            Label = At-Normal*20-FVector2D(23,8);
        }
        else continue;
        FSlateDrawElement::MakeText(Elements, Layer+1,
            Geometry.ToPaintGeometry(FVector2f(100,20), FSlateLayoutTransform(FVector2f(Label))),
            FString::Printf(TEXT("NAV %02d"), Index), FCoreStyle::GetDefaultFontStyle("Bold", 10),
            ESlateDrawEffect::None, Color);
    }
}
