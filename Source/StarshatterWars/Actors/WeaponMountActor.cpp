#include "WeaponMountActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
AWeaponMountActor::AWeaponMountActor()
{
    PrimaryActorTick.bCanEverTick=false;
    MountRoot=CreateDefaultSubobject<USceneComponent>(TEXT("MountRoot"));
    SetRootComponent(MountRoot);
    MountMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MountMesh"));
    MountMesh->SetupAttachment(MountRoot);
    StoreMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StoreMesh"));
    StoreMesh->SetupAttachment(MountRoot);
    for(UStaticMeshComponent* Mesh : {MountMesh.Get(),StoreMesh.Get()}) {
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetGenerateOverlapEvents(false);
        Mesh->SetCanEverAffectNavigation(false);
    }
}
