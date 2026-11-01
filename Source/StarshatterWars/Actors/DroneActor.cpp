#include "DroneActor.h"
#include "Components/SceneComponent.h"
#include "NiagaraSystem.h"

ADroneActor::ADroneActor()
{
    // TargetDrone.def authoring scale; do not reapply over saved BP/instance scale.
    ShipRoot->SetRelativeScale3D(FVector(0.08f));
    ApplyDroneDefaults();
}

void ADroneActor::OnConstruction(const FTransform& Transform)
{
    ApplyDroneDefaults();
    Super::OnConstruction(Transform);
}

void ADroneActor::ApplyDroneDefaults()
{
    // Legacy local (right, up, forward) -> UE flight-local (forward, right, up).
    BridgePointOffset = FVector(8, 0, 145);
    ChasePointOffset = FVector(250, 0, -1200);
    FocusPointOffset = BridgePointOffset;
    DriveCenterPointOffset = FVector(-180, 0, 0);
    SensorPointOffset = FVector(100, 0, 0);
    NavPointOffset = FVector(60, 0, 0);
    ReactorPointOffset = FVector::ZeroVector;
    ComputerPointAOffset = FVector(80, 20, 0);
    ComputerPointBOffset = FVector(80, -20, 0);

    // Editor preview only. Bound ships use the parsed runtime Drive ports.
    MainEnginePointDefs.Reset();
    FShipPointDef Engine;
    Engine.PointName = TEXT("DroneEngine");
    Engine.LocalOffset = FVector(-292, 0, -58);
    Engine.LocalRotation = FRotator::ZeroRotator;
    MainEnginePointDefs.Add(Engine);
    NumMainEnginePoints = 1;
    bEnableMainEngineEmitters = true;
    if (!MainEngineEmitterSystem)
        MainEngineEmitterSystem = LoadObject<UNiagaraSystem>(nullptr,
            TEXT("/Game/RocketThrusterExhaustFX/FX/NS_RocketExhaust_Blue.NS_RocketExhaust_Blue"));
    MainEngineEmitterRelativeScale = FVector(0.8f);
    MainEngineEmitterRelativeRotation = FRotator(0, 180, 0);
    // No generic running lights: bound ships build only their parsed DEF navlights.
    NavLightDefs.Reset();
    // The reference drone has no weapons or additional thruster ports.
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    ThrusterPointDefs.Reset();
    NumWeaponMountPoints = 0;
    NumThrusterPoints = 0;
    SyncShipAttachmentCounts();
}
