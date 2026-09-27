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

    BuildNavLightsFromRuntime();
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