#include "DebrisActor.h"

#include "Components/StaticMeshComponent.h"
#include "Debris.h"
#include "Engine/StaticMesh.h"
#include "Misc/PackageName.h"
#include "WeaponPresentationFrame.h"

ADebrisActor::ADebrisActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SetActorEnableCollision(false);
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("DebrisRoot")));
    DebrisMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebrisMesh"));
    DebrisMesh->SetupAttachment(GetRootComponent());
    DebrisMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DebrisMesh->SetCanEverAffectNavigation(false);
}

TSubclassOf<ADebrisActor> ADebrisActor::ResolveVisualClass(FName Model)
{
    if (Model.IsNone()) return nullptr;
    const FString Package = Model.ToString();
    if (!FPackageName::DoesPackageExist(Package)) return nullptr;
    const FString Blueprint = FPackageName::GetShortName(Package);
    return LoadClass<ADebrisActor>(nullptr, *(Package + TEXT(".") + Blueprint + TEXT("_C")));
}

double ADebrisActor::GetVisualRadius() const
{
    const UStaticMesh* Mesh = DebrisMesh->GetStaticMesh();
    if (!Mesh) return 0.0;
    const FBoxSphereBounds Bounds = Mesh->GetBounds().TransformBy(DebrisMesh->GetRelativeTransform());
    return Bounds.Origin.Size() + Bounds.SphereRadius;
}

void ADebrisActor::UpdateFromSimulation(const Debris& Piece, const FVector& Position)
{
    SetActorLocationAndRotation(Position, WeaponPresentationFrame::Rotation(Piece.GetCam()));
    const FVector Scale(Piece.GetVisualScale());
    if (!GetActorScale3D().Equals(Scale)) SetActorScale3D(Scale);
}
