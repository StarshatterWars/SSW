#include "MiningDroneActor.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "DrawDebugHelpers.h"
AMiningDroneActor::AMiningDroneActor()
{
    PrimaryActorTick.bCanEverTick=true;
    Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DroneBody"));SetRootComponent(Body);
    Arms=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MiningArms"));Arms->SetupAttachment(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if(Cube.Succeeded()){Body->SetStaticMesh(Cube.Object);Arms->SetStaticMesh(Cube.Object);}
    Body->SetRelativeScale3D(FVector(.15,.08,.06));Arms->SetRelativeScale3D(FVector(.55,3,.3));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);Arms->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetCastShadow(false);Arms->SetCastShadow(false);
}
void AMiningDroneActor::UpdateVisual(FVector Position,FVector Heading,bool Mining,FVector BeamEnd)
{
    Destination=Position;BeamTarget=BeamEnd;bMining=Mining;
    if(bFirstPosition){SetActorLocation(Position);bFirstPosition=false;}
    if(!Heading.IsNearlyZero())SetActorRotation(Heading.Rotation());
}
void AMiningDroneActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);SetActorLocation(FMath::VInterpTo(GetActorLocation(),Destination,DeltaSeconds,6));
    if(bMining)DrawDebugLine(GetWorld(),GetActorLocation(),BeamTarget,FColor(70,220,240),false,0,0,2);
}
