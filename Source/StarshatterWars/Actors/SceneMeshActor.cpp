/*  Project Starshatter Wars
    Fractal Dev Studios
*/

#include "SceneMeshActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

ASceneMeshActor::ASceneMeshActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(SceneRoot);
    MeshComponent->SetMobility(EComponentMobility::Movable);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeshComponent->SetGenerateOverlapEvents(false);
    MeshComponent->SetCastShadow(true);
}

bool ASceneMeshActor::SetSceneMesh(UStaticMesh* InMesh)
{
    if (!MeshComponent)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SceneMeshActor] SetSceneMesh: MeshComponent is null"));
        return false;
    }

    MeshComponent->SetStaticMesh(InMesh);

    UE_LOG(LogTemp, Warning,
        TEXT("[SceneMeshActor] SetSceneMesh: Mesh=%s"),
        *GetNameSafe(InMesh));

    return (InMesh != nullptr);
}