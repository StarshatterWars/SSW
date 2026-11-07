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

void UShieldComponent::ShowImpact(const FVector& LocalImpact, const FVector& LocalNormal, double WorldSize,
    const FString& ModelName, USceneComponent* Hull)
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->GetRootComponent() || LocalNormal.IsNearlyZero()) return;
    const bool bWantsModel = Hull && !ModelName.IsEmpty();
    const FString RequestedModel = bWantsModel ? ModelName : FString();
    if (ImpactMesh && ActiveModelName != RequestedModel)
    {
        ImpactMesh->DestroyComponent();
        ImpactMesh = nullptr;
        ImpactMaterial = nullptr;
    }

    if (!ImpactMesh)
    {
        UStaticMesh* Mesh = nullptr;
        UMaterialInterface* Material = nullptr;
        if (bWantsModel)
        {
            const FString Path = FString::Printf(TEXT("/Game/Models/Shields/%s/SM_%s.SM_%s"),
                *ModelName, *ModelName, *ModelName);
            Mesh = LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn);
            Material = Mesh ? Mesh->GetMaterial(0) : nullptr;
        }
        if (!Mesh || !Material)
        {
            if (!ShieldTexture) return;
            Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
            Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/Ships/M_ShieldImpact.M_ShieldImpact"));
        }
        if (!Mesh || !Material) return;

        ImpactMesh = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient);
        // Attach authored shells to the hull so they share its scale and 180-degree correction.
        const bool bModel = bWantsModel && Mesh->GetName() == (TEXT("SM_") + ModelName);
        ImpactMesh->SetupAttachment(bModel ? Hull : Owner->GetRootComponent());
        ImpactMesh->SetStaticMesh(Mesh);
        ImpactMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ImpactMesh->SetGenerateOverlapEvents(false);
        ImpactMesh->SetCastShadow(false);
        ImpactMesh->SetCanEverAffectNavigation(false);
        ImpactMaterial = ImpactMesh->CreateDynamicMaterialInstance(0, Material);
        ImpactMesh->RegisterComponent();
        ActiveModelName = RequestedModel;
    }

    if (!ImpactMaterial) return;
    if (ImpactMesh->GetAttachParent() == Owner->GetRootComponent())
    {
        const FVector ReciprocalScale = FTransform::GetSafeScaleReciprocal(Owner->GetActorScale3D());
        ImpactMesh->SetRelativeLocationAndRotation((LocalImpact + LocalNormal * 4.0) * ReciprocalScale,
            FRotationMatrix::MakeFromZ(LocalNormal).Rotator());
        ImpactMesh->SetWorldScale3D(FVector(WorldSize / 100.0));
        ImpactMaterial->SetTextureParameterValue(TEXT("ShieldTexture"), ShieldTexture);
    }
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
