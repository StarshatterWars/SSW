#include "MiningDroneActor.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "MiningBeamActor.h"
#include "Engine/World.h"
AMiningDroneActor::AMiningDroneActor()
{
    PrimaryActorTick.bCanEverTick=true;
    Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DroneBody"));SetRootComponent(Body);
    Arms=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MiningArms"));Arms->SetupAttachment(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if(Cube.Succeeded()){Body->SetStaticMesh(Cube.Object);Arms->SetStaticMesh(Cube.Object);}
    Body->SetRelativeScale3D(FVector(.15,.08,.06));Arms->SetRelativeScale3D(FVector(.55,3,.3));
    // FBX is centimetres; the live simulation maps one world unit to a metre.
    if (auto* Model=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Models/MiningDrone/SM_MiningDrone.SM_MiningDrone"),nullptr,LOAD_NoWarn))
    {
        Body->SetStaticMesh(Model);Body->SetRelativeScale3D(FVector(.01));
        Arms->SetVisibility(false);
    }
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
    if(bMining && !IsValid(ActiveBeam) && GetWorld())
    {
        ActiveBeam=GetWorld()->SpawnActor<AMiningBeamActor>();
        if(ActiveBeam){ActiveBeam->SetOwner(this);ActiveBeam->BeginSimulationBeam(TEXT("Mining Laser"));}
    }
    if(IsValid(ActiveBeam))
    {
        if(!bMining){ActiveBeam->Destroy();ActiveBeam=nullptr;}
        else
        {
            const FVector Muzzle=GetActorTransform().TransformPosition(FVector(1.58,0,.02));
            ActiveBeam->SetBeamPoints(Muzzle,BeamTarget);ActiveBeam->SetBeamActive(true);
            ActiveBeam->SetBeamWidth(.12f);
        }
    }
}

void AMiningDroneActor::EndPlay(const EEndPlayReason::Type Reason)
{
    if(IsValid(ActiveBeam))ActiveBeam->Destroy();ActiveBeam=nullptr;
    Super::EndPlay(Reason);
}
