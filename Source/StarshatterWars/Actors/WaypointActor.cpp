#include "WaypointActor.h"
#include "WaypointArrival.h"
#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
AWaypointActor::AWaypointActor()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("WaypointRoot"));
    ArrivalSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ArrivalSphere"));
    ArrivalSphere->SetupAttachment(RootComponent);
    ArrivalSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ArrivalSphere->SetGenerateOverlapEvents(false);
    ArrivalSphere->SetHiddenInGame(true);
    ArrivalSphere->SetSphereRadius(0.0f, false);
    WaypointMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaypointMesh"));
    WaypointMesh->SetupAttachment(RootComponent);
    WaypointMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WaypointMesh->SetGenerateOverlapEvents(false);
    WaypointMesh->SetCastShadow(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Sphere.Succeeded()) WaypointMesh->SetStaticMesh(Sphere.Object);
    // The sphere exposes Materials > Element 0 in BP_Waypoint.
    // Visible for authoring, hidden in gameplay along with its owning actor.
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    bReplicates = false;
    Tags.Add(TEXT("MissionNavigationTarget"));
}

double AWaypointActor::LegacyArrivalRadius(double HullRadius)
{
    return LegacyWaypointArrival::Radius(HullRadius);
}
void AWaypointActor::SetArrivalHullRadius(double HullRadius)
{
    ArrivalRadiusMeters = LegacyArrivalRadius(HullRadius);
    // Display-only volume. Simulation arrival never depends on UE overlaps.
    if (ArrivalSphere) ArrivalSphere->SetSphereRadius(float(ArrivalRadiusMeters), false);
}
