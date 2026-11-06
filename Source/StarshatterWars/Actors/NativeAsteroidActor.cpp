#include "NativeAsteroidActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Asteroid.h"
ANativeAsteroidActor::ANativeAsteroidActor()
{
    PrimaryActorTick.bCanEverTick=false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("AsteroidRoot")));
    AsteroidMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AsteroidMesh"));AsteroidMesh->SetupAttachment(GetRootComponent());
    AsteroidMesh->SetMobility(EComponentMobility::Movable);
    AsteroidMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
double ANativeAsteroidActor::RawRadius(double Radius){return FMath::IsFinite(Radius)&&Radius>0?Radius:0;}
void ANativeAsteroidActor::UpdateFromSimulation(const Asteroid& Rock,const FVector& Position,UStaticMesh* Mesh)
{
    if(!Mesh){SetActorHiddenInGame(true);return;}
    const double Radius=RawRadius(Mesh->GetBounds().SphereRadius);
    if(Radius<=0){SetActorHiddenInGame(true);return;}
    if (AsteroidMesh->GetStaticMesh() != Mesh)
    {
        AsteroidMesh->SetStaticMesh(Mesh);
        AsteroidMesh->SetRelativeLocation(-Mesh->GetBounds().Origin);
    }
    if (IsHidden()) SetActorHiddenInGame(false);
    const auto Axis=[](const FVector& V){return FVector(V.Z,V.X,V.Y);};
    const Camera& Cam=Rock.GetCam();
    const FMatrix Basis(FPlane(Axis(Cam.vpn()),0),FPlane(Axis(Cam.vrt()),0),FPlane(Axis(Cam.vup()),0),FPlane(0,0,0,1));
    SetActorLocationAndRotation(Position,FQuat(Basis));
    if (GetActorScale3D() != FVector::OneVector) SetActorScale3D(FVector::OneVector);
    RenderedRadius=Radius;
}
