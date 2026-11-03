#include "TurretBaseActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
ATurretBaseActor::ATurretBaseActor()
{
    PrimaryActorTick.bCanEverTick=false;SetActorEnableCollision(false);
    BaseRoot=CreateDefaultSubobject<USceneComponent>(TEXT("BaseRoot"));SetRootComponent(BaseRoot);
    ModelRoot=CreateDefaultSubobject<USceneComponent>(TEXT("ModelRoot"));ModelRoot->SetupAttachment(BaseRoot);
    BaseMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));BaseMesh->SetupAttachment(ModelRoot);
    BaseMesh->bEditableWhenInherited=true;ModelRoot->bEditableWhenInherited=true;
    BaseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BaseMesh->SetGenerateOverlapEvents(false);BaseMesh->SetCanEverAffectNavigation(false);
}
void ATurretBaseActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);ModelRoot->SetRelativeScale3D(FVector(LegacyModelScale));
}
void ATurretBaseActor::ApplyDefinition(const FString& Model,float Scale)
{
    LegacyBaseModel=Model;LegacyModelScale=Scale;
    if(ModelRoot)ModelRoot->SetRelativeScale3D(FVector(Scale));
}
