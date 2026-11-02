#include "RocketMissileActor.h"
#include "Components/StaticMeshComponent.h"

ARocketMissileActor::ARocketMissileActor()
{
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(MissileRoot);
    MeshComponent->bEditableWhenInherited = true;
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeshComponent->SetGenerateOverlapEvents(false);
    MeshComponent->SetCanEverAffectNavigation(false);
}
