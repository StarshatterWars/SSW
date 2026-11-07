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
    PlaceholderMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundPlaceholder"));
    PlaceholderMesh->SetupAttachment(VisualRoot);
    PlaceholderMesh->SetCollisionResponseToAllChannels(ECR_Block);
    if(Cube.Succeeded())PlaceholderMesh->SetStaticMesh(Cube.Object);
    UpdateGroundPlaceholder();
}
void AMasterBuildingActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    UpdateGroundPlaceholder();
}
void AMasterBuildingActor::UpdateGroundPlaceholder()
{
    // Keep the placeholder separate so a real mesh inherits normal UE scale.
    if(!PlaceholderMesh)return;
    const bool bShow=!(HullMesh && HullMesh->GetStaticMesh());
    PlaceholderMesh->SetVisibility(bShow);
    PlaceholderMesh->SetCollisionEnabled(bShow?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
    const FVector Size=PlaceholderSizeCm.ComponentMax(FVector(1.f));
    PlaceholderMesh->SetRelativeScale3D(Size/100.f);
    PlaceholderMesh->SetRelativeLocation(FVector(0.f,0.f,Size.Z*.5f));
}
