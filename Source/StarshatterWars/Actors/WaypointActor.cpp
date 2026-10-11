#include "WaypointActor.h"
#include "Components/SceneComponent.h"
AWaypointActor::AWaypointActor()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("WaypointRoot"));
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    bReplicates = false;
}
