#include "GoliathShipActor.h"

AGoliathShipActor::AGoliathShipActor()
{

    // Native default only: preserve BP and placed-instance scale during construction.
    SetActorScale3D(FVector(3.3f));
    ApplyGoliathDefaults();
    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments(TEXT("Goliath"));
}

void AGoliathShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyGoliathDefaults();

    Super::OnConstruction(Transform);

    ApplyGoliathFixedPoints();
}

void AGoliathShipActor::ApplyGoliathDefaults()
{
    bAutoRebuildGeneratedComponents = true;
    bRebuildOnConstruction = true;

    bEnableMainEngineEmitters = true;
    bEnableThrusterEmitters = true;

    bMainEnginesActive = true;
    bThrustersActive = false;

    MainEngineEmitterRelativeScale = FVector(0.70f, 0.26f, 0.26f);
    MainEngineEmitterRelativeRotation = FRotator(0.0f, 180.0f, 0.0f);

    ThrusterEmitterRelativeScale = FVector(0.75f, 0.30f, 0.30f);
    ThrusterEmitterRelativeRotation = FRotator::ZeroRotator;



    FocusPointOffset = FVector(60.0f, 0.0f, 0.0f);
    BridgePointOffset = FVector(60.0f, 0.0f, 320.0f);
    ChasePointOffset = FVector(170.0f, 0.0f, -1800.0f);

    DriveCenterPointOffset = FVector(-450.0f, 0.0f, 0.0f);
    QuantumPointOffset = FVector(0.0f, 0.0f, 0.0f);
    ShieldPointOffset = FVector(-80.0f, 0.0f, 20.0f);
    SensorPointOffset = FVector(380.0f, 0.0f, -16.0f);
    NavPointOffset = FVector(60.0f, 0.0f, 16.0f);
    ComputerPointAOffset = FVector(80.0f, 20.0f, 16.0f);
    ComputerPointBOffset = FVector(80.0f, -20.0f, -16.0f);
    ReactorPointOffset = FVector(-280.0f, 0.0f, 4.0f);

    // Main engine ports are supplied by the runtime drive system from
    // Goliath.def. Do not add parallel generated engine mount components.
    NumMainEnginePoints = 0;
    MainEnginePointDefs.Empty();

    NumThrusterPoints = 0;
    NumWeaponMountPoints = 4;
    NumTurretBasePoints = 0;
    NumDockPoints = 2;
    NumLandingPoints = 2;

    ThrusterPointDefs.Empty();

    WeaponMountPointDefs.Empty();
    {
        FShipPointDef W;

        W.LocalOffset = FVector(64.0f, 104.0f, -17.0f);
        W.LocalRotation = FRotator(0.0f, 60.0f, 0.0f);
        W.PointName = TEXT("PDB_1");
        WeaponMountPointDefs.Add(W);

        W.LocalOffset = FVector(64.0f, -104.0f, -17.0f);
        W.LocalRotation = FRotator(0.0f, -60.0f, 0.0f);
        W.PointName = TEXT("PDB_2");
        WeaponMountPointDefs.Add(W);

        W.LocalOffset = FVector(-64.0f, 104.0f, -17.0f);
        W.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        W.PointName = TEXT("PDB_3");
        WeaponMountPointDefs.Add(W);

        W.LocalOffset = FVector(-64.0f, -104.0f, -17.0f);
        W.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
        W.PointName = TEXT("PDB_4");
        WeaponMountPointDefs.Add(W);
    }

    TurretBasePointDefs.Empty();

    DockPointDefs.Empty();
    {
        FShipPointDef D;

        D.LocalOffset = FVector(192.0f, 0.0f, -85.0f);
        D.LocalRotation = FRotator::ZeroRotator;
        D.PointName = TEXT("FlightDeck_1");
        DockPointDefs.Add(D);

        D.LocalOffset = FVector(-92.0f, 0.0f, -82.0f);
        D.LocalRotation = FRotator::ZeroRotator;
        D.PointName = TEXT("FlightDeck_2");
        DockPointDefs.Add(D);
    }

    LandingPointDefs.Empty();
    {
        FShipPointDef L;

        L.LocalOffset = FVector(160.0f, 16.0f, -85.0f);
        L.LocalRotation = FRotator::ZeroRotator;
        L.PointName = TEXT("Deck_1_Spot_0");
        LandingPointDefs.Add(L);

        L.LocalOffset = FVector(160.0f, -16.0f, -85.0f);
        L.LocalRotation = FRotator::ZeroRotator;
        L.PointName = TEXT("Deck_1_Spot_1");
        LandingPointDefs.Add(L);

        L.LocalOffset = FVector(160.0f, 0.0f, -85.0f);
        L.LocalRotation = FRotator::ZeroRotator;
        L.PointName = TEXT("Deck_1_Spot_2");
        LandingPointDefs.Add(L);

        L.LocalOffset = FVector(-92.0f, 0.0f, -82.0f);
        L.LocalRotation = FRotator::ZeroRotator;
        L.PointName = TEXT("Deck_2_Recovery");
        LandingPointDefs.Add(L);
    }

    NavLightDefs.Empty();
}

void AGoliathShipActor::ApplyGoliathFixedPoints()
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
void AGoliathShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("PDB 1");
      Point.LocalOffset = FVector(64.000000f, 104.000000f, -17.000000f);
      Point.LocalRotation = FRotator(0.000000f, 60.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 2");
      Point.LocalOffset = FVector(64.000000f, -104.000000f, -17.000000f);
      Point.LocalRotation = FRotator(0.000000f, -60.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 3");
      Point.LocalOffset = FVector(-64.000000f, 104.000000f, -17.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 4");
      Point.LocalOffset = FVector(-64.000000f, -104.000000f, -17.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 1_Base");
      Point.LocalOffset = FVector(64.000000f, 104.000000f, -17.000000f);
      Point.LocalRotation = FRotator(0.000000f, 60.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 2_Base");
      Point.LocalOffset = FVector(64.000000f, -104.000000f, -17.000000f);
      Point.LocalRotation = FRotator(0.000000f, -60.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 3_Base");
      Point.LocalOffset = FVector(-64.000000f, 104.000000f, -17.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 4_Base");
      Point.LocalOffset = FVector(-64.000000f, -104.000000f, -17.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
