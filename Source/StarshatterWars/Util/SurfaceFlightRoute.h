#pragma once
#include "CoreMinimal.h"
#include "Instruction.h"

// Mission coordinates are X/Y horizontal, Z altitude. Sample the full leg,
// not just its endpoint, so a hill between two markers cannot hide the route.
namespace SurfaceFlightRoute
{
    constexpr double CruiseClearance = 1500.0;
    template<typename HeightSampler>
    double ClearedAltitude(const FVector& Start, const FVector& End, HeightSampler&& Height)
    {
        double Altitude = End.Z;
        const double Distance = FVector2D(End.X-Start.X, End.Y-Start.Y).Size();
        const int32 Samples = FMath::Clamp(FMath::CeilToInt(Distance / 500.0), 1, 512);
        for (int32 Index=0; Index<=Samples; ++Index)
        {
            const FVector Point = FMath::Lerp(Start, End, double(Index)/Samples);
            const double Ground = Height(Point.X, Point.Y);
            if (FMath::IsFinite(Ground)) Altitude = FMath::Max(Altitude, Ground + CruiseClearance);
        }
        return Altitude;
    }

    template<typename HeightSampler>
    bool PrepareWaypoint(Instruction& Point, SimRegion* Region, const char* RegionName,
        FVector& Previous, HeightSampler&& Height)
    {
        if (!Region) return false;
        if (!Point.GetRegion())
        {
            const char* NamedRegion = Point.GetRegionName();
            if (NamedRegion && NamedRegion[0] && FCStringAnsi::Stricmp(NamedRegion, RegionName)) return false;
            Point.SetRegion(Region);
        }
        if (Point.GetRegion()!=Region) return false;
        switch (Point.GetAction())
        {
        case EInstruction::Vector: case EInstruction::Sweep: case EInstruction::Strike:
        case EInstruction::RTB: case EInstruction::Patrol: case EInstruction::Intercept:
        case EInstruction::Recon: case EInstruction::Escort: case EInstruction::Defend:
        case EInstruction::Assault: case EInstruction::StopAt: case EInstruction::Hold:
            break;
        default:
            // Landing/departure profiles belong to the deck/ILS controller.
            return false;
        }
        FVector Position=Point.GetLocation();
        Position.Z=ClearedAltitude(Previous,Position,Height);
        Point.SetLocation(Position);
        Previous=Position;
        return true;
    }

    // Physics coordinates: X/Z horizontal and Y vertical. Check the projected
    // flight path, including descent, rather than waiting for low current AGL.
    template<typename HeightSampler>
    double RecoveryAltitude(const FVector& Position, const FVector& Velocity, HeightSampler&& Height)
    {
        double Required = Position.Y;
        for (int32 Step = 0; Step <= 8; ++Step)
        {
            const double Seconds = double(Step);
            const FVector Future = Position + Velocity * Seconds;
            const double Ground = Height(Future.X, Future.Z);
            if (FMath::IsFinite(Ground) && Future.Y < Ground + CruiseClearance)
                Required = FMath::Max(Required, Ground + CruiseClearance + FMath::Max(0.0, -Velocity.Y * Seconds));
        }
        return Required;
    }
}
