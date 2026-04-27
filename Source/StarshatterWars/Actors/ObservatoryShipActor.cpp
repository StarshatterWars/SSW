#include "ObservatoryShipActor.h"

AObservatoryShipActor::AObservatoryShipActor()
{
    ApplyObservatoryDefaults();
}

void AObservatoryShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyObservatoryDefaults();

    Super::OnConstruction(Transform);

    ApplyObservatoryFixedPoints();
}

void AObservatoryShipActor::ApplyObservatoryDefaults()
{
    bAutoRebuildGeneratedComponents = true;
    bRebuildOnConstruction = true;

    bEnableMainEngineEmitters = false;
    bEnableThrusterEmitters = false;

    bMainEnginesActive = false;
    bThrustersActive = false;

    SetActorScale3D(FVector(5.0f));

    FocusPointOffset = FVector(0.0f, 0.0f, 0.0f);
    BridgePointOffset = FVector(0.0f, 0.0f, 0.0f);
    ChasePointOffset = FVector(50.0f, 0.0f, -800.0f);

    DriveCenterPointOffset = FVector::ZeroVector;
    QuantumPointOffset = FVector::ZeroVector;
    ShieldPointOffset = FVector::ZeroVector;
    SensorPointOffset = FVector(380.0f, 0.0f, -16.0f);
    NavPointOffset = FVector::ZeroVector;
    ComputerPointAOffset = FVector::ZeroVector;
    ComputerPointBOffset = FVector::ZeroVector;
    ReactorPointOffset = FVector::ZeroVector;

    NumMainEnginePoints = 0;
    NumThrusterPoints = 0;
    NumWeaponMountPoints = 0;
    NumTurretBasePoints = 0;
    NumDockPoints = 2;
    NumLandingPoints = 2;

    MainEnginePointDefs.Empty();
    ThrusterPointDefs.Empty();
    WeaponMountPointDefs.Empty();
    TurretBasePointDefs.Empty();

    DockPointDefs.Empty();
    {
        FShipPointDef D;

        D.LocalOffset = FVector(360.0f, 380.0f, 0.0f);
        D.LocalRotation = FRotator::ZeroRotator;
        D.PointName = TEXT("Launch_Bay");
        DockPointDefs.Add(D);

        D.LocalOffset = FVector(360.0f, -380.0f, 0.0f);
        D.LocalRotation = FRotator::ZeroRotator;
        D.PointName = TEXT("Docking_Bay");
        DockPointDefs.Add(D);
    }

    LandingPointDefs.Empty();
    {
        FShipPointDef L;

        L.LocalOffset = FVector(360.0f, 380.0f, -8.0f);
        L.LocalRotation = FRotator::ZeroRotator;
        L.PointName = TEXT("Launch_Bay_Spot");
        LandingPointDefs.Add(L);

        L.LocalOffset = FVector(360.0f, -380.0f, -8.0f);
        L.LocalRotation = FRotator::ZeroRotator;
        L.PointName = TEXT("Docking_Bay_Spot");
        LandingPointDefs.Add(L);
    }

    NavLightDefs.Empty();
    {
        FShipNavLightDef L;

        L.LocalOffset = FVector(342.0f, 13.0f, 300.0f);
        L.LocalRotation = FRotator::ZeroRotator;
        L.Color = FLinearColor::White;
        L.Intensity = 3000.0f;
        L.Radius = 120.0f;
        L.Mode = EShipNavLightMode::Blink;
        L.BlinkInterval = 2.5f;
        L.PhaseOffset = 0.0f;
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(-1120.0f, -8.0f, 0.0f);
        L.LocalRotation = FRotator::ZeroRotator;
        L.Color = FLinearColor::White;
        L.Intensity = 3000.0f;
        L.Radius = 120.0f;
        L.Mode = EShipNavLightMode::Blink;
        L.BlinkInterval = 2.5f;
        L.PhaseOffset = 0.5f;
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(770.0f, 0.0f, 64.0f);
        L.LocalRotation = FRotator::ZeroRotator;
        L.Color = FLinearColor(0.6f, 0.8f, 1.0f);
        L.Intensity = 2500.0f;
        L.Radius = 100.0f;
        L.Mode = EShipNavLightMode::Blink;
        L.BlinkInterval = 2.5f;
        L.PhaseOffset = 0.25f;
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(360.0f, 400.0f, 64.0f);
        L.LocalRotation = FRotator::ZeroRotator;
        L.Color = FLinearColor(0.6f, 0.8f, 1.0f);
        L.Intensity = 2500.0f;
        L.Radius = 100.0f;
        L.Mode = EShipNavLightMode::Blink;
        L.BlinkInterval = 2.5f;
        L.PhaseOffset = 0.25f;
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(360.0f, -400.0f, 64.0f);
        L.LocalRotation = FRotator::ZeroRotator;
        L.Color = FLinearColor(0.6f, 0.8f, 1.0f);
        L.Intensity = 2500.0f;
        L.Radius = 100.0f;
        L.Mode = EShipNavLightMode::Blink;
        L.BlinkInterval = 2.5f;
        L.PhaseOffset = 0.25f;
        NavLightDefs.Add(L);
    }
}

void AObservatoryShipActor::ApplyObservatoryFixedPoints()
{
    if (FocusPoint)
    {
        FocusPoint->SetRelativeLocation(FocusPointOffset);
    }

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