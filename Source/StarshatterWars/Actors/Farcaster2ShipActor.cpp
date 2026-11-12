#include "Farcaster2ShipActor.h"
#include "FarcasterEffectActor.h"
#include "Components/StaticMeshComponent.h"

AFarcaster2ShipActor::AFarcaster2ShipActor()
{
    SetActorScale3D(FVector(20.0f));
    CenterMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FarcasterCenter"));
    CenterMesh->SetupAttachment(HullMesh);
    CenterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    bLegacyEmitterCoordinates=true;
    bLegacyEmitterUsesHullAxes=true;
    bEnableMainEngineEmitters=false;
    bEnableThrusterEmitters=false;
    // Bound ships build their lights, engines and mounts from the parsed design.
    NavLightDefs.Reset();
}

void AFarcaster2ShipActor::BeginPlay()
{
    Super::BeginPlay();
    AFarcasterEffectActor::InstallOn(*this, true);
}
