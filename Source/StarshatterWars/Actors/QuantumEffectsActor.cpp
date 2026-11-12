#include "QuantumEffectsActor.h"

#include "Components/MaterialBillboardComponent.h"
#include "Engine/CollisionProfile.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AQuantumEffectsActor::AQuantumEffectsActor()
{
    Flash = CreateDefaultSubobject<UMaterialBillboardComponent>(TEXT("QuantumFlash"));
    Flash->SetupAttachment(EffectRoot);
    Flash->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
    Flash->SetGenerateOverlapEvents(false);
    Flash->SetCastShadow(false);

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(
        TEXT("/Game/WarpGates/Materials/Effects/MI_WarpEffect_CircleBlue.MI_WarpEffect_CircleBlue"));
    if (Material.Succeeded())
    {
        constexpr float HalfSizeCm = 3500.f;
        Flash->AddElement(Material.Object, nullptr, false, HalfSizeCm, HalfSizeCm, nullptr);
    }
}
