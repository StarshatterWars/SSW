#include "MasterBuildingActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AMasterBuildingActor::AMasterBuildingActor()
{
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    SetActorScale3D(FVector::OneVector);
    bEnableThrusterEmitters = false;
    bEnableMainEngineEmitters = false;
    bEnableNavLights = false;
    if (HullMesh && Cube.Succeeded())
    {
        HullMesh->SetStaticMesh(Cube.Object);
        HullMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        HullMesh->SetCollisionResponseToAllChannels(ECR_Block);
    }
    UpdateGroundPlaceholder();
}
void AMasterBuildingActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    UpdateGroundPlaceholder();
}
void AMasterBuildingActor::UpdateGroundPlaceholder()
{
    // Never replace or resize an artist-provided building mesh.
    if (!HullMesh || !HullMesh->GetStaticMesh() ||
        HullMesh->GetStaticMesh()->GetPathName()!=TEXT("/Engine/BasicShapes/Cube.Cube")) return;
    const FVector Size=PlaceholderSizeCm.ComponentMax(FVector(1.f));
    HullMesh->SetRelativeScale3D(Size/100.f);
    HullMesh->SetRelativeLocation(FVector(0.f,0.f,Size.Z*.5f));
}
