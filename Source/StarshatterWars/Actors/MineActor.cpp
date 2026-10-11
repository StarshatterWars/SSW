#include "MineActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
AMineActor::AMineActor()
{
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (HullMesh && Sphere.Succeeded())
    {
        HullMesh->SetStaticMesh(Sphere.Object);
        HullMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        HullMesh->SetCastShadow(false);
    }
}
