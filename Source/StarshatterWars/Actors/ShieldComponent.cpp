#include "ShieldComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"

namespace
{
constexpr float ImpactDuration = 0.35f;
}

UShieldComponent::UShieldComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UShieldComponent::ShowImpact(const FVector& LocalImpact, const FVector& LocalNormal, double WorldSize)
{
    AActor* Owner = GetOwner();
    if (!ShieldTexture || !Owner || !Owner->GetRootComponent() || LocalNormal.IsNearlyZero()) return;
    if (!ImpactMesh)
    {
        auto* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
        auto* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/Ships/M_ShieldImpact.M_ShieldImpact"));
        if (!Plane || !Material) return;
        ImpactMesh = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient);
        ImpactMesh->SetupAttachment(Owner->GetRootComponent());
        ImpactMesh->SetStaticMesh(Plane);
        ImpactMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ImpactMesh->SetGenerateOverlapEvents(false);
        ImpactMesh->SetCastShadow(false);
        ImpactMesh->SetCanEverAffectNavigation(false);
        ImpactMaterial = ImpactMesh->CreateDynamicMaterialInstance(0, Material);
        ImpactMesh->RegisterComponent();
    }

    if (!ImpactMaterial) return;
    const FVector Scale = Owner->GetActorScale3D();
    const FVector ReciprocalScale = FTransform::GetSafeScaleReciprocal(Scale);
    ImpactMesh->SetRelativeLocationAndRotation((LocalImpact + LocalNormal * 4.0) * ReciprocalScale,
        FRotationMatrix::MakeFromZ(LocalNormal).Rotator());
    ImpactMesh->SetWorldScale3D(FVector(WorldSize / 100.0));
    ImpactMaterial->SetTextureParameterValue(TEXT("ShieldTexture"), ShieldTexture);
    ImpactMaterial->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
    ImpactMesh->SetVisibility(true);
    RemainingTime = ImpactDuration;
    SetComponentTickEnabled(true);
}

void UShieldComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    RemainingTime = FMath::Max(0.0f, RemainingTime - DeltaTime);
    if (ImpactMaterial)
        ImpactMaterial->SetScalarParameterValue(TEXT("Opacity"), RemainingTime / ImpactDuration);
    if (RemainingTime <= 0.0f)
    {
        if (ImpactMesh) ImpactMesh->SetVisibility(false);
        SetComponentTickEnabled(false);
    }
}
