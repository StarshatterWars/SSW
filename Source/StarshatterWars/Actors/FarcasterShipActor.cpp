#include "FarcasterShipActor.h"

AFarcasterShipActor::AFarcasterShipActor()
{
    ApplyFarcasterDefaults();
}

void AFarcasterShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyFarcasterDefaults();

    Super::OnConstruction(Transform);

    ApplyFarcasterFixedPoints();
}

void AFarcasterShipActor::ApplyFarcasterDefaults()
{
    bAutoRebuildGeneratedComponents = true;
    bRebuildOnConstruction = true;

    bEnableMainEngineEmitters = false;
    bEnableThrusterEmitters = false;

    SetActorScale3D(FVector(15.0f));

    FocusPointOffset = FVector(0.0f, 0.0f, 24.0f);
    BridgePointOffset = FVector(0.0f, 0.0f, 24.0f);
    ChasePointOffset = FVector(0.0f, -750.0f, 80.0f);

    DriveCenterPointOffset = FVector(0.0f, 0.0f, 0.0f);
    QuantumPointOffset = FVector(0.0f, 0.0f, 0.0f);
    ShieldPointOffset = FVector(0.0f, 0.0f, 0.0f);
    SensorPointOffset = FVector(0.0f, 0.0f, 0.0f);
    NavPointOffset = FVector(0.0f, 0.0f, 0.0f);
    ComputerPointAOffset = FVector::ZeroVector;
    ComputerPointBOffset = FVector::ZeroVector;
    ReactorPointOffset = FVector(0.0f, 0.0f, -32.0f);

    NumMainEnginePoints = 0;
    NumThrusterPoints = 0;
    NumWeaponMountPoints = 0;
    NumTurretBasePoints = 0;
    NumDockPoints = 0;
    NumLandingPoints = 0;

    MainEnginePointDefs.Empty();
    ThrusterPointDefs.Empty();
    WeaponMountPointDefs.Empty();
    TurretBasePointDefs.Empty();
    DockPointDefs.Empty();
    LandingPointDefs.Empty();

    NavLightDefs.Empty();
    {
        FShipNavLightDef L;

        // navlight block 1, type 3, period 1.5
        // using bright white blink for the outer ring lights

        L.LocalOffset = FVector(-138.0f, -42.0f, -257.0f);
        L.LocalRotation = FRotator::ZeroRotator;
        L.Color = FLinearColor::White;
        L.Intensity = 14000.0f;
        L.Radius = 450.0f;
        L.Mode = EShipNavLightMode::Blink;
        L.BlinkInterval = 1.5f;
        L.PhaseOffset = 0.00f;
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(-138.0f, -42.0f, 257.0f);
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(-138.0f, 42.0f, -257.0f);
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(-138.0f, 42.0f, 257.0f);
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(138.0f, -42.0f, -257.0f);
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(138.0f, -42.0f, 257.0f);
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(138.0f, 42.0f, -257.0f);
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(138.0f, 42.0f, 257.0f);
        NavLightDefs.Add(L);

        // navlight block 2, type 2, period 1.0
        // approximated as alternating red/green blink pairs because
        // ShipActor nav lights do not currently support raw bit patterns

        L.Intensity = 12000.0f;
        L.Radius = 400.0f;
        L.Mode = EShipNavLightMode::Blink;
        L.BlinkInterval = 1.0f;

        L.LocalOffset = FVector(-138.0f, -42.0f, -257.0f);
        L.Color = FLinearColor::Red;
        L.PhaseOffset = 0.00f;
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(138.0f, -42.0f, -257.0f);
        L.Color = FLinearColor::Green;
        L.PhaseOffset = 0.50f;
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(-138.0f, 42.0f, -257.0f);
        L.Color = FLinearColor::Green;
        L.PhaseOffset = 0.50f;
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(138.0f, 42.0f, -257.0f);
        L.Color = FLinearColor::Red;
        L.PhaseOffset = 0.00f;
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(-138.0f, -42.0f, 257.0f);
        L.Color = FLinearColor::Green;
        L.PhaseOffset = 0.50f;
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(138.0f, -42.0f, 257.0f);
        L.Color = FLinearColor::Red;
        L.PhaseOffset = 0.00f;
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(-138.0f, 42.0f, 257.0f);
        L.Color = FLinearColor::Red;
        L.PhaseOffset = 0.00f;
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(138.0f, 42.0f, 257.0f);
        L.Color = FLinearColor::Green;
        L.PhaseOffset = 0.50f;
        NavLightDefs.Add(L);
    }
}

void AFarcasterShipActor::ApplyFarcasterFixedPoints()
{
    if (BridgePoint)
    {
        BridgePoint->SetRelativeLocation(BridgePointOffset);
    }

    if (ChasePoint)
    {
        ChasePoint->SetRelativeLocation(ChasePointOffset);
    }

    if (DriveCenterPoint)
    {
        DriveCenterPoint->SetRelativeLocation(DriveCenterPointOffset);
    }

    if (QuantumPoint)
    {
        QuantumPoint->SetRelativeLocation(QuantumPointOffset);
    }

    if (ShieldPoint)
    {
        ShieldPoint->SetRelativeLocation(ShieldPointOffset);
    }

    if (SensorPoint)
    {
        SensorPoint->SetRelativeLocation(SensorPointOffset);
    }

    if (NavPoint)
    {
        NavPoint->SetRelativeLocation(NavPointOffset);
    }

    if (ComputerPointA)
    {
        ComputerPointA->SetRelativeLocation(ComputerPointAOffset);
    }

    if (ComputerPointB)
    {
        ComputerPointB->SetRelativeLocation(ComputerPointBOffset);
    }

    if (ReactorPoint)
    {
        ReactorPoint->SetRelativeLocation(ReactorPointOffset);
    }
}