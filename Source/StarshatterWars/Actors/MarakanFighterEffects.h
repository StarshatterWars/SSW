#pragma once
#include "ShipActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "FighterExhaustTuning.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

// Visual attachment defaults generated from the seven current fighter DEFs.
// Simulation ports, fire masks, drive output and timing remain authoritative.
namespace MarakanFighterEffects
{
inline void AlignWeapons(AShipActor& Ship)
{
    // Match the FBX handedness conversion (Z,-X,Y). Keep all authored mount
    // and per-barrel offsets intact under one mirrored presentation parent.
    if (Ship.HullWeaponRoot) Ship.HullWeaponRoot->SetRelativeScale3D(FVector(1,-1,1));
    if (Ship.WeaponHardpointRoot) Ship.WeaponHardpointRoot->SetRelativeScale3D(FVector(1,-1,1));
}
inline void ConfigureSystems(AShipActor& Ship)
{
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Engine(TEXT("/Game/RocketThrusterExhaustFX/FX/NS_RocketExhaust_Yellow.NS_RocketExhaust_Yellow"));
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Thruster(TEXT("/Game/RocketThrusterExhaustFX/FX/NS_RocketExhaust_Yellow.NS_RocketExhaust_Yellow"));
    Ship.MainEngineEmitterSystem = Engine.Object;
    Ship.ThrusterEmitterSystem = Thruster.Object;
    Ship.ThrusterTrailSystem = Thruster.Object;
    Ship.ThrusterFlareSystem = Thruster.Object;
    AlignWeapons(Ship);
    if (Ship.HullMesh) Ship.HullMesh->SetRelativeRotation(FRotator(0,180,0));
    Ship.bEnableNavLights = true;
}

inline void Avenger(AShipActor& Ship)
{
    ConfigureSystems(Ship);
    Ship.bLegacyEmitterCoordinates = true;
    Ship.MainEnginePointDefs.Reset();
    Ship.ThrusterPointDefs.Reset();
    Ship.NavLightDefs.Reset();
    { FShipPointDef P; P.PointName = TEXT("Engine_1"); P.LocalOffset = FVector(-176.000000f, -39.000000f, 7.000000f); Ship.MainEnginePointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("Engine_2"); P.LocalOffset = FVector(-185.000000f, -0.000000f, 25.000000f); Ship.MainEnginePointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("Engine_3"); P.LocalOffset = FVector(-176.000000f, 39.000000f, 7.000000f); Ship.MainEnginePointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("fore_1"); P.LocalOffset = FVector(-67.000000f, 161.000000f, -30.000000f); P.LocalRotation = FRotator(0.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("fore_2"); P.LocalOffset = FVector(-67.000000f, -161.000000f, -30.000000f); P.LocalRotation = FRotator(0.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("aft_3"); P.LocalOffset = FVector(-134.000000f, 161.000000f, -30.000000f); P.LocalRotation = FRotator(0.000000f, 180.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("aft_4"); P.LocalOffset = FVector(-134.000000f, -161.000000f, -30.000000f); P.LocalRotation = FRotator(0.000000f, 180.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_5"); P.LocalOffset = FVector(54.000000f, 6.000000f, 11.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_6"); P.LocalOffset = FVector(54.000000f, -5.000000f, 11.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_7"); P.LocalOffset = FVector(59.000000f, 14.000000f, -24.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_8"); P.LocalOffset = FVector(59.000000f, -14.000000f, -24.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_9"); P.LocalOffset = FVector(-81.000000f, 161.000000f, -25.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_10"); P.LocalOffset = FVector(-121.000000f, 161.000000f, -25.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_11"); P.LocalOffset = FVector(-81.000000f, -161.000000f, -25.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_12"); P.LocalOffset = FVector(-121.000000f, -161.000000f, -25.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_13"); P.LocalOffset = FVector(-81.000000f, 161.000000f, -35.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_14"); P.LocalOffset = FVector(-121.000000f, 161.000000f, -35.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_15"); P.LocalOffset = FVector(-81.000000f, -161.000000f, -35.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_16"); P.LocalOffset = FVector(-121.000000f, -161.000000f, -35.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("left_17"); P.LocalOffset = FVector(173.000000f, 25.000000f, -9.000000f); P.LocalRotation = FRotator(0.000000f, 90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("right_18"); P.LocalOffset = FVector(173.000000f, -25.000000f, -9.000000f); P.LocalRotation = FRotator(0.000000f, -90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-90.000000f, -140.000000f, 8.000000f); L.Color = FLinearColor::Red;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Steady;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-90.000000f, 140.000000f, 8.000000f); L.Color = FLinearColor::Green;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Steady;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-70.000000f, 0.000000f, 25.000000f); L.Color = FLinearColor::White;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Blink;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-70.000000f, 0.000000f, -15.000000f); L.Color = FLinearColor::White;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Blink;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    Ship.NumMainEnginePoints = Ship.MainEnginePointDefs.Num();
    Ship.NumThrusterPoints = Ship.ThrusterPointDefs.Num();
    Ship.ThrusterEmitterRelativeRotation = FRotator::ZeroRotator;
    Ship.ThrusterEmitterRelativeScale = FVector(0.120000f);
}

inline void Cobra(AShipActor& Ship)
{
    ConfigureSystems(Ship);
    Ship.bLegacyEmitterCoordinates = false;
    Ship.MainEnginePointDefs.Reset();
    Ship.ThrusterPointDefs.Reset();
    Ship.NavLightDefs.Reset();
    { FShipPointDef P; P.PointName = TEXT("Engine_1"); P.LocalOffset = FVector(-203.000000f, 0.000000f, 0.000000f); Ship.MainEnginePointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("fore_1"); P.LocalOffset = FVector(-10.000000f, -166.000000f, -15.000000f); P.LocalRotation = FRotator(0.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("fore_2"); P.LocalOffset = FVector(-10.000000f, 167.000000f, -15.000000f); P.LocalRotation = FRotator(0.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("aft_3"); P.LocalOffset = FVector(-70.000000f, -166.000000f, -15.000000f); P.LocalRotation = FRotator(0.000000f, 180.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("aft_4"); P.LocalOffset = FVector(-70.000000f, 167.000000f, -15.000000f); P.LocalRotation = FRotator(0.000000f, 180.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_5"); P.LocalOffset = FVector(-117.000000f, -29.000000f, 2.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_6"); P.LocalOffset = FVector(-117.000000f, 29.000000f, 2.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_7"); P.LocalOffset = FVector(65.000000f, -24.000000f, 2.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_8"); P.LocalOffset = FVector(65.000000f, 24.000000f, 2.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_9"); P.LocalOffset = FVector(-117.000000f, -29.000000f, -8.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_10"); P.LocalOffset = FVector(-117.000000f, 29.000000f, -8.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_11"); P.LocalOffset = FVector(65.000000f, -24.000000f, -8.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_12"); P.LocalOffset = FVector(65.000000f, 24.000000f, -8.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_13"); P.LocalOffset = FVector(-23.000000f, -166.000000f, -9.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_14"); P.LocalOffset = FVector(-56.000000f, -166.000000f, -9.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_15"); P.LocalOffset = FVector(-23.000000f, 167.000000f, -9.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_16"); P.LocalOffset = FVector(-56.000000f, 167.000000f, -9.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_17"); P.LocalOffset = FVector(-23.000000f, -166.000000f, -20.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_18"); P.LocalOffset = FVector(-56.000000f, -166.000000f, -20.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_19"); P.LocalOffset = FVector(-23.000000f, 167.000000f, -20.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_20"); P.LocalOffset = FVector(-56.000000f, 167.000000f, -20.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("left_21"); P.LocalOffset = FVector(-70.000000f, -169.000000f, -14.500000f); P.LocalRotation = FRotator(0.000000f, -90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("left_22"); P.LocalOffset = FVector(-10.000000f, -169.000000f, -14.500000f); P.LocalRotation = FRotator(0.000000f, -90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("right_23"); P.LocalOffset = FVector(-70.000000f, 169.000000f, -14.500000f); P.LocalRotation = FRotator(0.000000f, 90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("right_24"); P.LocalOffset = FVector(-10.000000f, 169.000000f, -14.500000f); P.LocalRotation = FRotator(0.000000f, 90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-85.000000f, -130.000000f, 6.000000f); L.Color = FLinearColor::Red;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Steady;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-85.000000f, 130.000000f, 6.000000f); L.Color = FLinearColor::Green;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Steady;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-65.000000f, 0.000000f, 22.000000f); L.Color = FLinearColor::White;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Blink;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-65.000000f, 0.000000f, -12.000000f); L.Color = FLinearColor::White;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Blink;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    Ship.NumMainEnginePoints = Ship.MainEnginePointDefs.Num();
    Ship.NumThrusterPoints = Ship.ThrusterPointDefs.Num();
    Ship.ThrusterEmitterRelativeRotation = FRotator::ZeroRotator;
    Ship.ThrusterEmitterRelativeScale = FVector(0.120000f);
}

inline void Intruder(AShipActor& Ship)
{
    ConfigureSystems(Ship);
    Ship.bLegacyEmitterCoordinates = true;
    Ship.MainEnginePointDefs.Reset();
    Ship.ThrusterPointDefs.Reset();
    Ship.NavLightDefs.Reset();
    { FShipPointDef P; P.PointName = TEXT("Engine_1"); P.LocalOffset = FVector(-178.000000f, -28.000000f, 11.000000f); Ship.MainEnginePointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("Engine_2"); P.LocalOffset = FVector(-178.000000f, 28.000000f, 11.000000f); Ship.MainEnginePointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("fore_1"); P.LocalOffset = FVector(-69.000000f, 123.000000f, -22.000000f); P.LocalRotation = FRotator(0.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("fore_2"); P.LocalOffset = FVector(-69.000000f, -123.000000f, -22.000000f); P.LocalRotation = FRotator(0.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("aft_3"); P.LocalOffset = FVector(-129.000000f, 123.000000f, -22.000000f); P.LocalRotation = FRotator(0.000000f, 180.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("aft_4"); P.LocalOffset = FVector(-129.000000f, -123.000000f, -22.000000f); P.LocalRotation = FRotator(0.000000f, 180.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_5"); P.LocalOffset = FVector(-99.000000f, 5.000000f, 12.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_6"); P.LocalOffset = FVector(-99.000000f, -5.000000f, 12.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_7"); P.LocalOffset = FVector(35.000000f, 5.000000f, 18.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_8"); P.LocalOffset = FVector(35.000000f, -5.000000f, 18.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_9"); P.LocalOffset = FVector(35.000000f, 14.000000f, -8.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_10"); P.LocalOffset = FVector(35.000000f, -14.000000f, -8.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_11"); P.LocalOffset = FVector(-82.000000f, 123.000000f, -17.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_12"); P.LocalOffset = FVector(-117.000000f, 123.000000f, -17.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_13"); P.LocalOffset = FVector(-82.000000f, -123.000000f, -17.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_14"); P.LocalOffset = FVector(-117.000000f, -123.000000f, -17.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_15"); P.LocalOffset = FVector(-82.000000f, 123.000000f, -27.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_16"); P.LocalOffset = FVector(-117.000000f, 123.000000f, -27.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_17"); P.LocalOffset = FVector(-82.000000f, -123.000000f, -27.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_18"); P.LocalOffset = FVector(-117.000000f, -123.000000f, -27.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("left_19"); P.LocalOffset = FVector(128.000000f, 15.000000f, 2.000000f); P.LocalRotation = FRotator(0.000000f, 90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("right_20"); P.LocalOffset = FVector(128.000000f, -15.000000f, 2.000000f); P.LocalRotation = FRotator(0.000000f, -90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-80.000000f, -120.000000f, 6.000000f); L.Color = FLinearColor::Red;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Steady;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-80.000000f, 120.000000f, 6.000000f); L.Color = FLinearColor::Green;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Steady;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-60.000000f, 0.000000f, 20.000000f); L.Color = FLinearColor::White;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Blink;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-60.000000f, 0.000000f, -12.000000f); L.Color = FLinearColor::White;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Blink;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    Ship.NumMainEnginePoints = Ship.MainEnginePointDefs.Num();
    Ship.NumThrusterPoints = Ship.ThrusterPointDefs.Num();
    Ship.ThrusterEmitterRelativeRotation = FRotator::ZeroRotator;
    Ship.ThrusterEmitterRelativeScale = FVector(0.120000f);
}

inline void Jackal(AShipActor& Ship)
{
    ConfigureSystems(Ship);
    Ship.bLegacyEmitterCoordinates = true;
    Ship.MainEnginePointDefs.Reset();
    Ship.ThrusterPointDefs.Reset();
    Ship.NavLightDefs.Reset();
    { FShipPointDef P; P.PointName = TEXT("Engine_1"); P.LocalOffset = FVector(-160.000000f, 147.000000f, 32.000000f); Ship.MainEnginePointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("Engine_2"); P.LocalOffset = FVector(-160.000000f, 147.000000f, -32.000000f); Ship.MainEnginePointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("Engine_3"); P.LocalOffset = FVector(-160.000000f, -147.000000f, 32.000000f); Ship.MainEnginePointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("Engine_4"); P.LocalOffset = FVector(-160.000000f, -147.000000f, -32.000000f); Ship.MainEnginePointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("fore_1"); P.LocalOffset = FVector(-10.000000f, 140.000000f, -20.000000f); P.LocalRotation = FRotator(0.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("fore_2"); P.LocalOffset = FVector(-10.000000f, -140.000000f, -20.000000f); P.LocalRotation = FRotator(0.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("aft_3"); P.LocalOffset = FVector(-120.000000f, 140.000000f, -20.000000f); P.LocalRotation = FRotator(0.000000f, 180.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("aft_4"); P.LocalOffset = FVector(-120.000000f, -140.000000f, -20.000000f); P.LocalRotation = FRotator(0.000000f, 180.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_5"); P.LocalOffset = FVector(40.000000f, 20.000000f, 5.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_6"); P.LocalOffset = FVector(40.000000f, -20.000000f, 5.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_7"); P.LocalOffset = FVector(40.000000f, 20.000000f, -10.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_8"); P.LocalOffset = FVector(40.000000f, -20.000000f, -10.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("left_9"); P.LocalOffset = FVector(100.000000f, 15.000000f, 0.000000f); P.LocalRotation = FRotator(0.000000f, 90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("right_10"); P.LocalOffset = FVector(100.000000f, -15.000000f, 0.000000f); P.LocalRotation = FRotator(0.000000f, -90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-100.000000f, -150.000000f, 10.000000f); L.Color = FLinearColor::Red;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Steady;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-100.000000f, 150.000000f, 10.000000f); L.Color = FLinearColor::Green;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Steady;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-80.000000f, 0.000000f, 28.000000f); L.Color = FLinearColor::White;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Blink;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-80.000000f, 0.000000f, -16.000000f); L.Color = FLinearColor::White;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Blink;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    Ship.NumMainEnginePoints = Ship.MainEnginePointDefs.Num();
    Ship.NumThrusterPoints = Ship.ThrusterPointDefs.Num();
    Ship.ThrusterEmitterRelativeRotation = FRotator::ZeroRotator;
    Ship.ThrusterEmitterRelativeScale = FVector(0.120000f);
}

inline void Raptor(AShipActor& Ship)
{
    ConfigureSystems(Ship);
    Ship.bLegacyEmitterCoordinates = true;
    Ship.MainEnginePointDefs.Reset();
    Ship.ThrusterPointDefs.Reset();
    Ship.NavLightDefs.Reset();
    { FShipPointDef P; P.PointName = TEXT("Engine_1"); P.LocalOffset = FVector(-136.000000f, -38.000000f, 2.000000f); Ship.MainEnginePointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("Engine_2"); P.LocalOffset = FVector(-136.000000f, 38.000000f, 2.000000f); Ship.MainEnginePointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("fore_1"); P.LocalOffset = FVector(-90.000000f, 154.000000f, -46.000000f); P.LocalRotation = FRotator(0.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("fore_2"); P.LocalOffset = FVector(-90.000000f, -154.000000f, -46.000000f); P.LocalRotation = FRotator(0.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("aft_3"); P.LocalOffset = FVector(-150.000000f, 154.000000f, -46.000000f); P.LocalRotation = FRotator(0.000000f, 180.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("aft_4"); P.LocalOffset = FVector(-150.000000f, -154.000000f, -46.000000f); P.LocalRotation = FRotator(0.000000f, 180.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_5"); P.LocalOffset = FVector(-92.000000f, -0.000000f, 6.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_6"); P.LocalOffset = FVector(54.000000f, 6.000000f, 6.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_7"); P.LocalOffset = FVector(54.000000f, -5.000000f, 6.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_8"); P.LocalOffset = FVector(59.000000f, 14.000000f, -18.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_9"); P.LocalOffset = FVector(59.000000f, -14.000000f, -18.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_10"); P.LocalOffset = FVector(-104.000000f, 154.000000f, -41.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_11"); P.LocalOffset = FVector(-138.000000f, 154.000000f, -41.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_12"); P.LocalOffset = FVector(-104.000000f, -154.000000f, -41.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_13"); P.LocalOffset = FVector(-138.000000f, -154.000000f, -41.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_14"); P.LocalOffset = FVector(-104.000000f, 154.000000f, -51.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_15"); P.LocalOffset = FVector(-138.000000f, 154.000000f, -51.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_16"); P.LocalOffset = FVector(-104.000000f, -154.000000f, -51.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_17"); P.LocalOffset = FVector(-138.000000f, -154.000000f, -51.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("left_18"); P.LocalOffset = FVector(139.000000f, 25.000000f, -9.000000f); P.LocalRotation = FRotator(0.000000f, 90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("right_19"); P.LocalOffset = FVector(139.000000f, -25.000000f, -9.000000f); P.LocalRotation = FRotator(0.000000f, -90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-85.000000f, -135.000000f, 7.000000f); L.Color = FLinearColor::Red;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Steady;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-85.000000f, 135.000000f, 7.000000f); L.Color = FLinearColor::Green;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Steady;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-65.000000f, 0.000000f, 22.000000f); L.Color = FLinearColor::White;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Blink;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-65.000000f, 0.000000f, -14.000000f); L.Color = FLinearColor::White;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Blink;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    Ship.NumMainEnginePoints = Ship.MainEnginePointDefs.Num();
    Ship.NumThrusterPoints = Ship.ThrusterPointDefs.Num();
    Ship.ThrusterEmitterRelativeRotation = FRotator::ZeroRotator;
    Ship.ThrusterEmitterRelativeScale = FVector(0.120000f);
}

inline void Razor(AShipActor& Ship)
{
    ConfigureSystems(Ship);
    Ship.bLegacyEmitterCoordinates = false;
    Ship.MainEnginePointDefs.Reset();
    Ship.ThrusterPointDefs.Reset();
    Ship.NavLightDefs.Reset();
    { FShipPointDef P; P.PointName = TEXT("Engine_1"); P.LocalOffset = FVector(-166.000000f, 0.000000f, 0.000000f); Ship.MainEnginePointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("fore_1"); P.LocalOffset = FVector(5.000000f, -159.000000f, -24.000000f); P.LocalRotation = FRotator(0.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("fore_2"); P.LocalOffset = FVector(5.000000f, 159.000000f, -24.000000f); P.LocalRotation = FRotator(0.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("aft_3"); P.LocalOffset = FVector(-54.000000f, -159.000000f, -24.000000f); P.LocalRotation = FRotator(0.000000f, 180.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("aft_4"); P.LocalOffset = FVector(-54.000000f, 159.000000f, -24.000000f); P.LocalRotation = FRotator(0.000000f, 180.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_5"); P.LocalOffset = FVector(-125.000000f, -39.000000f, -13.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_6"); P.LocalOffset = FVector(-125.000000f, 39.000000f, -13.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_7"); P.LocalOffset = FVector(127.000000f, -39.000000f, -4.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_8"); P.LocalOffset = FVector(127.000000f, 39.000000f, -4.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_9"); P.LocalOffset = FVector(-125.000000f, -39.000000f, -24.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_10"); P.LocalOffset = FVector(-125.000000f, 39.000000f, -24.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_11"); P.LocalOffset = FVector(127.000000f, -39.000000f, -15.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_12"); P.LocalOffset = FVector(127.000000f, 39.000000f, -15.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_13"); P.LocalOffset = FVector(-8.000000f, -159.000000f, -19.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_14"); P.LocalOffset = FVector(-41.000000f, -159.000000f, -19.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_15"); P.LocalOffset = FVector(-8.000000f, 159.000000f, -19.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_16"); P.LocalOffset = FVector(-41.000000f, 159.000000f, -19.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_17"); P.LocalOffset = FVector(-8.000000f, -159.000000f, -29.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_18"); P.LocalOffset = FVector(-41.000000f, -159.000000f, -29.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_19"); P.LocalOffset = FVector(-8.000000f, 159.000000f, -29.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_20"); P.LocalOffset = FVector(-41.000000f, 159.000000f, -29.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("left_21"); P.LocalOffset = FVector(-54.000000f, -161.000000f, -24.000000f); P.LocalRotation = FRotator(0.000000f, -90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("left_22"); P.LocalOffset = FVector(5.000000f, -161.000000f, -24.000000f); P.LocalRotation = FRotator(0.000000f, -90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("right_23"); P.LocalOffset = FVector(-54.000000f, 161.000000f, -24.000000f); P.LocalRotation = FRotator(0.000000f, 90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("right_24"); P.LocalOffset = FVector(5.000000f, 161.000000f, -24.000000f); P.LocalRotation = FRotator(0.000000f, 90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-75.000000f, -125.000000f, 6.000000f); L.Color = FLinearColor::Red;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Steady;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-75.000000f, 125.000000f, 6.000000f); L.Color = FLinearColor::Green;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Steady;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-55.000000f, 0.000000f, 20.000000f); L.Color = FLinearColor::White;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Blink;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-55.000000f, 0.000000f, -12.000000f); L.Color = FLinearColor::White;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Blink;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    Ship.NumMainEnginePoints = Ship.MainEnginePointDefs.Num();
    Ship.NumThrusterPoints = Ship.ThrusterPointDefs.Num();
    Ship.ThrusterEmitterRelativeRotation = FRotator::ZeroRotator;
    Ship.ThrusterEmitterRelativeScale = FVector(0.120000f);
}

inline void Viper(AShipActor& Ship)
{
    ConfigureSystems(Ship);
    Ship.bLegacyEmitterCoordinates = false;
    Ship.MainEnginePointDefs.Reset();
    Ship.ThrusterPointDefs.Reset();
    Ship.NavLightDefs.Reset();
    { FShipPointDef P; P.PointName = TEXT("Engine_1"); P.LocalOffset = FVector(-166.000000f, 0.000000f, 0.000000f); Ship.MainEnginePointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("fore_1"); P.LocalOffset = FVector(5.000000f, -110.000000f, -20.000000f); P.LocalRotation = FRotator(0.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("fore_2"); P.LocalOffset = FVector(5.000000f, 110.000000f, -20.000000f); P.LocalRotation = FRotator(0.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("aft_3"); P.LocalOffset = FVector(-45.000000f, -110.000000f, -20.000000f); P.LocalRotation = FRotator(0.000000f, 180.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("aft_4"); P.LocalOffset = FVector(-45.000000f, 110.000000f, -20.000000f); P.LocalRotation = FRotator(0.000000f, 180.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_5"); P.LocalOffset = FVector(40.000000f, -20.000000f, 5.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_6"); P.LocalOffset = FVector(40.000000f, 20.000000f, 5.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_7"); P.LocalOffset = FVector(40.000000f, -20.000000f, -10.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_8"); P.LocalOffset = FVector(40.000000f, 20.000000f, -10.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("left_9"); P.LocalOffset = FVector(100.000000f, -15.000000f, 0.000000f); P.LocalRotation = FRotator(0.000000f, -90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("right_10"); P.LocalOffset = FVector(100.000000f, 15.000000f, 0.000000f); P.LocalRotation = FRotator(0.000000f, 90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_11"); P.LocalOffset = FVector(-25.000000f, -110.000000f, -15.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("top_12"); P.LocalOffset = FVector(-25.000000f, 110.000000f, -15.000000f); P.LocalRotation = FRotator(90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_13"); P.LocalOffset = FVector(-25.000000f, -110.000000f, -25.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("bottom_14"); P.LocalOffset = FVector(-25.000000f, 110.000000f, -25.000000f); P.LocalRotation = FRotator(-90.000000f, 0.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("left_15"); P.LocalOffset = FVector(-45.000000f, -112.000000f, -20.000000f); P.LocalRotation = FRotator(0.000000f, -90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("left_16"); P.LocalOffset = FVector(5.000000f, -112.000000f, -20.000000f); P.LocalRotation = FRotator(0.000000f, -90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("right_17"); P.LocalOffset = FVector(-45.000000f, 112.000000f, -20.000000f); P.LocalRotation = FRotator(0.000000f, 90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipPointDef P; P.PointName = TEXT("right_18"); P.LocalOffset = FVector(5.000000f, 112.000000f, -20.000000f); P.LocalRotation = FRotator(0.000000f, 90.000000f, 0.000000f); Ship.ThrusterPointDefs.Add(P); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-70.000000f, -115.000000f, 5.000000f); L.Color = FLinearColor::Red;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Steady;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-70.000000f, 115.000000f, 5.000000f); L.Color = FLinearColor::Green;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Steady;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-50.000000f, 0.000000f, 18.000000f); L.Color = FLinearColor::White;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Blink;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    { FShipNavLightDef L; L.LocalOffset = FVector(-50.000000f, 0.000000f, -10.000000f); L.Color = FLinearColor::White;
      L.Intensity = 100.000000f; L.Radius = 30.000000f; L.Mode = EShipNavLightMode::Blink;
      L.BlinkInterval = 1.000000f; Ship.NavLightDefs.Add(L); }
    Ship.NumMainEnginePoints = Ship.MainEnginePointDefs.Num();
    Ship.NumThrusterPoints = Ship.ThrusterPointDefs.Num();
    Ship.ThrusterEmitterRelativeRotation = FRotator::ZeroRotator;
    Ship.ThrusterEmitterRelativeScale = FVector(0.120000f);
}

}


