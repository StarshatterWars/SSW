#include "FighterHUDDetails.h"
#include "Ship.h"
#include "FlightDeck.h"
#include "Sim.h"
#include "SimRegion.h"
#include "SSWRuntimeSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"

void SFighterHUDDetails::DrawILS(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer) const
{
    Sim* Simulation = Sim::GetSim();
    Ship* Player = Simulation ? Simulation->GetPlayerShip() : nullptr;
    SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
    if (!bILSMode || !Player || !Region || !Region->GetShips().contains(Player) ||
        !Player->IsDropship() || Player->IsDead() || Player->InTransition() ||
        (Player->GetFlightPhase() != EOPSMode::ACTIVE &&
         Player->GetFlightPhase() != EOPSMode::APPROACH && Player->GetFlightPhase() != EOPSMode::RECOVERY) ||
        !GEngine || !FSlateApplication::IsInitialized()) return;

    APlayerController* Controller = nullptr;
    USSWRuntimeSubsystem* Runtime = nullptr;
    for (const FWorldContext& Context : GEngine->GetWorldContexts())
    {
        UWorld* World = Context.World();
        if (!World || !World->IsGameWorld() || !World->GetGameInstance()) continue;
        APlayerController* Candidate = World->GetFirstPlayerController();
        if (!Candidate || !Candidate->IsLocalController()) continue;
        Runtime = World->GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>();
        if (Runtime) { Controller = Candidate; break; }
    }
    if (!Controller || !Runtime) return;

    const FGeometry ScreenGeometry = UWidgetLayoutLibrary::GetPlayerScreenWidgetGeometry(Controller);
    auto Project = [&](Ship* Carrier, const FVector& SimulationPoint, FVector2D& Local) -> bool
    {
        FVector World;
        FVector2D Screen;
        if (!Runtime->GetCarrierVisualPoint(Carrier, SimulationPoint, World) ||
            !UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(Controller, World, Screen, true)) return false;
        Local = Geometry.AbsoluteToLocal(ScreenGeometry.LocalToAbsolute(Screen));
        return !Local.ContainsNaN();
    };

    const FLinearColor ILSColor(0.15f, 0.55f, 1.0f, 0.9f);
    auto DrawDeck = [&](FlightDeck* Deck) -> bool
    {
        Ship* Carrier = Deck ? const_cast<Ship*>(Deck->GetCarrier()) : nullptr;
        if (!Deck || !Deck->IsRecoveryDeck() || !Carrier || Carrier->IsDead() || Carrier->GetRegion() != Region) return false;
        // Assigned deck first. Like legacy HUDView::DrawILS, draw its mounting cross
        // and the exact four simulation hoops; never create a parallel approach path.
        FVector2D Mount;
        if (Project(Carrier, Deck->GetMountLocation(), Mount))
        {
            TArray<FVector2D> First = {Mount + FVector2D(-6,-6), Mount + FVector2D(6,6)};
            TArray<FVector2D> Second = {Mount + FVector2D(6,-6), Mount + FVector2D(-6,6)};
            FSlateDrawElement::MakeLines(Elements, Layer+1, Geometry.ToPaintGeometry(), First, ESlateDrawEffect::None, ILSColor, true);
            FSlateDrawElement::MakeLines(Elements, Layer+1, Geometry.ToPaintGeometry(), Second, ESlateDrawEffect::None, ILSColor, true);
        }
        bool HasHoops = false;
        // Far to near, matching the foreground translucent legacy frames.
        for (int32 Index = Deck->NumHoops()-1; Index >= 0; --Index)
        {
            FVector Corners[4];
            if (!Deck->GetILSHoopCorners(Index, Corners)) continue;
            HasHoops = true;
            FVector2D Points[4];
            bool InFront = true;
            for (int32 Corner = 0; Corner < 4; ++Corner)
                if (!Project(Carrier, Corners[Corner], Points[Corner])) { InFront = false; break; }
            if (!InFront) continue;
            if (ILSTexture.IsValid())
            {
                const FVector2f UVs[4] = {FVector2f(0,1), FVector2f(1,1), FVector2f(1,0), FVector2f(0,0)};
                TArray<FSlateVertex> Vertices;
                for (int32 Corner = 0; Corner < 4; ++Corner)
                    Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
                        Geometry.GetAccumulatedRenderTransform(), FVector2f(Points[Corner]), UVs[Corner], ILSColor.ToFColor(true)));
                const TArray<SlateIndex> Indices = {0,1,2,0,2,3};
                FSlateDrawElement::MakeCustomVerts(Elements, Layer+1,
                    FSlateApplication::Get().GetRenderer()->GetResourceHandle(ILSBrush), Vertices, Indices, nullptr, 0, 0);
            }
            else
            {
                TArray<FVector2D> Outline = {Points[0],Points[1],Points[2],Points[3],Points[0]};
                FSlateDrawElement::MakeLines(Elements, Layer+1, Geometry.ToPaintGeometry(), Outline,
                    ESlateDrawEffect::None, ILSColor, true, 1.5f);
            }
        }
        return HasHoops;
    };

    InboundSlot* Arrival = Player->GetInbound();
    if (Arrival && DrawDeck(Arrival->GetDeck())) return;
    // Legacy ILS preview: without an assigned deck, show friendly/neutral recovery decks.
    ListIter<Ship> Carriers = Region->GetCarriers();
    while (++Carriers)
    {
        Ship* Carrier = Carriers.value();
        if (!Carrier || (Carrier->GetIFF() != Player->GetIFF() && Carrier->GetIFF() != 0)) continue;
        for (int32 Index = 0; Index < Carrier->NumFlightDecks(); ++Index)
            DrawDeck(Carrier->GetFlightDeck(Index));
    }
}
