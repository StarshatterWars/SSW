#pragma once
#include "WaypointActor.h"
#include "NativeAsteroidActor.h"
#include "Asteroid.h"
#include "SystemSceneBuilder.h"
#include "Ship.h"
#include "SimRegion.h"
#include "SimElement.h"
#include "Game.h"
#include "EngineUtils.h"
inline constexpr TCHAR AirbaseContactIcon[] = TEXT("/Game/UI/HUD/AirbaseHud.AirbaseHud");
inline constexpr TCHAR StationContactIcon[] = TEXT("/Game/UI/HUD/StationHud.StationHud");

struct FMissionObjectData
{
    FString Name, Type, ImagePath;
    double RangeKm = -1;
};
inline bool ReadMissionObject(AActor* Actor, Ship* Player, SimRegion* Region, FMissionObjectData& Out)
{
    if (!IsValid(Actor) || Actor->IsActorBeingDestroyed() || !Player || !Region) return false;
    if (const auto* Visual = Cast<ANativeAsteroidActor>(Actor))
    {
        const Asteroid* Rock = Visual->ResolveContact(Region);
        if (!Rock) return false;
        Out.Name = FString::Printf(TEXT("Asteroid %02d"), Visual->GetContactIndex() + 1);
        Out.Type = TEXT("Asteroid");
        Out.ImagePath = AWaypointActor::AsteroidContactIcon;
        Out.RangeKm = FVector::Distance(Player->GetLocation(), Rock->GetLocation()) / 1000.0;
        return true;
    }
    FVector Authored;
    if (AWaypointActor* Waypoint = Cast<AWaypointActor>(Actor))
    {
        if (!Waypoint->SectorName.Equals(ANSI_TO_TCHAR(Region->GetName()), ESearchCase::IgnoreCase)) return false;
        if (Waypoint->WaypointIndex > 0)
        {
            SimElement* Flight = Player->GetElement();
            Instruction* Next = Flight ? Flight->GetNextNavPoint() : nullptr;
            if (!Next || Waypoint->ElementIdentity != Flight->GetIdentity() ||
                Waypoint->WaypointIndex != Flight->GetNavIndex(Next)) return false;
        }
        Out.Name = Waypoint->CompactLabel();
        Out.Type = !Waypoint->NamedLocationType.IsEmpty() ? Waypoint->NamedLocationType :
            Waypoint->bNextWaypoint ? TEXT("Waypoint (next)") : TEXT("Waypoint");
        if (Out.Type.Equals(TEXT("Airbase"), ESearchCase::IgnoreCase))
            Out.ImagePath = AirbaseContactIcon;
        if (Waypoint->bAsteroidFieldContact)
        {
            if (!Region->GetTrainingAsteroidField().IsSet()) return false;
            Out.ImagePath = AWaypointActor::AsteroidContactIcon;
        }
        Out.RangeKm = FVector::Distance(OtherHand(Player->GetLocation()), Waypoint->LegacyLocation)/1000.0;
        return true;
    }
    for (TActorIterator<ASystemSceneBuilder> It(Actor->GetWorld()); It; ++It)
        if (It->GetMissionBodyInfo(Actor, Out.Name, Out.Type, Authored, Out.ImagePath))
        {
            Out.RangeKm = FVector::Distance(Region->GetLocation()+OtherHand(Player->GetLocation()), Authored)/1000.0;
            return true;
        }
    return false;
}
