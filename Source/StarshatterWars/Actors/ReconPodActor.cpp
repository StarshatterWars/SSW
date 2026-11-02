#include "ReconPodActor.h"
#include "Components/StaticMeshComponent.h"

AReconPodActor::AReconPodActor()
{
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(MissileRoot);
    MeshComponent->bEditableWhenInherited = true;
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeshComponent->SetGenerateOverlapEvents(false);
    MeshComponent->SetCanEverAffectNavigation(false);
}
