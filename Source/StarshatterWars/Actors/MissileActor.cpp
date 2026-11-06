#include "MissileActor.h"
#include "MissileFlightPresentation.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

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

void AMissileActor::AlignImportedMeshes()
{
    if (!bAlignImportedMeshForward) return;
    TArray<UStaticMeshComponent*> Meshes;
    GetComponents(Meshes);
    for (UStaticMeshComponent* Mesh : Meshes)
    {
        const UStaticMesh* Model = Mesh->GetStaticMesh();
        if (Mesh == GetRootComponent()) continue; // Never turn the simulation actor frame.
        if (!Model || !Model->GetPathName().StartsWith(TEXT("/Game/Models/Weapons/"))) continue;
        // This import convention applies to the elongated stock +Y missile
        // bodies, not decorations or arbitrary custom meshes attached to them.
        const FVector Extent = Model->GetBounds().BoxExtent;
        if (Extent.Y <= 2.0 * FMath::Max(Extent.X,Extent.Z)) continue;
        const FVector ModelForward = Mesh->GetComponentTransform().TransformVector(FVector::RightVector).GetSafeNormal();
        const FVector ActorForward = GetActorForwardVector();
        if (ModelForward.IsNearlyZero() || ModelForward.Equals(ActorForward,0.0001)) continue;
        // Rotate the body about its existing attachment origin. Neither the
        // station nor the actor frame moves. Already-corrected BPs are a no-op.
        Mesh->SetWorldRotation(FQuat::FindBetweenNormals(ModelForward,ActorForward) * Mesh->GetComponentQuat());
    }
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
    AlignImportedMeshes();
    PositionTrail();
    if (!bInFlight && TrailComponent)
    {
        TrailComponent->DeactivateImmediate();
        TrailComponent->SetVisibility(false);
        TrailComponent->SetHiddenInGame(true);
    }
}

void AMissileActor::BeginSimulationFlight(bool bIgnited)
{
    MissileFlightPresentation::Release(this);
    SetLifeSpan(0.f);
    bInFlight = true;
    AlignImportedMeshes();
    PositionTrail();
    if (bIgnited) IgniteSimulationTrail();
}

void AMissileActor::IgniteSimulationTrail()
{
    if (bEnableTrail && TrailComponent && !TrailComponent->IsActive())
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
