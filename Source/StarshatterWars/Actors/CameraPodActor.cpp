#include "CameraPodActor.h"

ACameraPodActor::ACameraPodActor()
{
    ApplyCameraPodDefaults();
}

void ACameraPodActor::OnConstruction(const FTransform& Transform)
{
    ApplyCameraPodDefaults();

    Super::OnConstruction(Transform);

    ApplyCameraPodVisibility();
}

void ACameraPodActor::BeginPlay()
{
    Super::BeginPlay();

    ApplyCameraPodVisibility();

    UE_LOG(LogTemp, Warning,
        TEXT("[CameraPodActor] Spawned as hidden ShipActor '%s'"),
        *GetName());
}

void ACameraPodActor::ApplyCameraPodDefaults()
{
    bAutoRebuildGeneratedComponents = false;
    bRebuildOnConstruction = false;

    bEnableMainEngineEmitters = false;
    bEnableThrusterEmitters = false;

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

    FocusPointOffset = FVector::ZeroVector;
    BridgePointOffset = FVector::ZeroVector;
    ChasePointOffset = FVector(0.0f, -1000.0f, 200.0f);

    SetActorScale3D(FVector::OneVector);
    SetCanBeDamaged(false);
}

void ACameraPodActor::ApplyCameraPodVisibility()
{
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);

    TArray<UActorComponent*> Components;
    GetComponents(Components);

    for (UActorComponent* Comp : Components)
    {
        if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Comp))
        {
            Prim->SetHiddenInGame(true);
            Prim->SetVisibility(false, true);
            Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Prim->SetGenerateOverlapEvents(false);
        }
    }
}