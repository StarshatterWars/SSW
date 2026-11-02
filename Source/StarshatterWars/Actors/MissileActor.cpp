#include "MissileActor.h"
#include "MissileFlightPresentation.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AMissileActor::AMissileActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SetActorEnableCollision(false);
    MissileRoot = CreateDefaultSubobject<USceneComponent>(TEXT("MissileRoot"));
    SetRootComponent(MissileRoot);
    TrailComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailComponent"));
    TrailComponent->SetupAttachment(MissileRoot);
    TrailComponent->SetAutoActivate(false);
    TrailComponent->SetVisibility(false);
    TrailComponent->SetHiddenInGame(true);
    TrailComponent->SetRelativeRotation(FRotator(0,180,0));
    TrailComponent->SetRelativeScale3D(FVector(0.05f));
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Exhaust(
        TEXT("/Game/RocketThrusterExhaustFX/FX/NS_RocketExhaust_Blue.NS_RocketExhaust_Blue"));
    if (Exhaust.Succeeded()) TrailComponent->SetAsset(Exhaust.Object);
}

void AMissileActor::PositionTrail()
{
    if (!bAutoPositionTrail || !TrailComponent) return;
    FBox Bounds(ForceInit);
    TArray<UStaticMeshComponent*> Meshes;
    GetComponents(Meshes);
    for (const auto* Mesh : Meshes)
        if (Mesh->GetStaticMesh())
            Bounds += Mesh->CalcBounds(Mesh->GetComponentTransform().GetRelativeTransform(GetActorTransform())).GetBox();
    if (Bounds.IsValid)
        TrailComponent->SetRelativeLocation(FVector(Bounds.Min.X,Bounds.GetCenter().Y,Bounds.GetCenter().Z));
}

void AMissileActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    PositionTrail();
    if (!bInFlight && TrailComponent)
    {
        TrailComponent->DeactivateImmediate();
        TrailComponent->SetVisibility(false);
        TrailComponent->SetHiddenInGame(true);
    }
}

void AMissileActor::BeginSimulationFlight()
{
    MissileFlightPresentation::Release(this);
    SetLifeSpan(0.f);
    bInFlight = true;
    PositionTrail();
    if (bEnableTrail && TrailComponent)
    {
        TrailComponent->SetVisibility(true);
        TrailComponent->SetHiddenInGame(false);
        TrailComponent->SetFloatParameter(TEXT("EnginePower"),1.f);
        TrailComponent->SetFloatParameter(TEXT("Throttle"),100.f);
        TrailComponent->SetFloatParameter(TEXT("ThrottleRequest"),100.f);
        TrailComponent->Activate(true);
    }
}

void AMissileActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (TrailComponent) TrailComponent->DeactivateImmediate();
    Super::EndPlay(EndPlayReason);
}
