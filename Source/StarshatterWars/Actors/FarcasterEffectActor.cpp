#include "FarcasterEffectActor.h"

#include "Components/ChildActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Ship.h"
#include "ShipActor.h"
#include "UObject/ConstructorHelpers.h"

AFarcasterEffectActor::AFarcasterEffectActor()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> PortalMaterial(
        TEXT("/Game/WarpGates/Materials/Effects/MI_WarpEffect_CircleRed.MI_WarpEffect_CircleRed"));

    for (const FName Name : { FName(TEXT("PortalFront")), FName(TEXT("PortalMiddle")), FName(TEXT("PortalRear")) })
    {
        auto* Layer = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Layer->SetupAttachment(RootComponent);
        Layer->SetStaticMesh(Plane.Object);
        Layer->SetMaterial(0, PortalMaterial.Object);
        Layer->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
        Layer->SetGenerateOverlapEvents(false);
        Layer->SetCastShadow(false);
        Layer->SetHiddenInGame(true);
        PortalLayers.Add(Layer);
    }
}

void AFarcasterEffectActor::InstallOn(AShipActor& Gate, bool bMarakan)
{
    if (Gate.GetNetMode() == NM_DedicatedServer)
        return;

    USceneComponent* Anchor = Gate.GetRootComponent();
    const TInlineComponentArray<UChildActorComponent*> ChildComponents(&Gate);
    for (UChildActorComponent* Visual : ChildComponents)
    {
        if (Visual->GetChildActorClass() == StaticClass())
            return;
        if (Visual->GetFName() == TEXT("WarpGateVisual"))
            Anchor = Visual;
    }

    auto* EffectComponent = NewObject<UChildActorComponent>(&Gate, TEXT("FarcasterEffect"));
    // Keep the frame's authored scale when the gate has a separate frame component.
    EffectComponent->SetupAttachment(Anchor);
    EffectComponent->SetChildActorClass(StaticClass());
    Gate.AddInstanceComponent(EffectComponent);
    EffectComponent->RegisterComponent();
    if (auto* Effect = Cast<AFarcasterEffectActor>(EffectComponent->GetChildActor()))
        Effect->ConfigureAppearance(bMarakan);
}

AFarcasterEffectActor* AFarcasterEffectActor::FindFor(const Ship* Gate)
{
    const AShipActor* Presentation = Gate ? Gate->GetNativePresentation() : nullptr;
    if (!Presentation)
        return nullptr;

    for (const UChildActorComponent* Child : TInlineComponentArray<UChildActorComponent*>(Presentation))
    {
        if (auto* Effect = Cast<AFarcasterEffectActor>(Child->GetChildActor()))
            return Effect;
    }
    return nullptr;
}

void AFarcasterEffectActor::ConfigureAppearance(bool bMarakan)
{
    // The original WarpGate 33 and 31 layer transforms are in frame-local space.
    const double Depths[] = { -230.0, 280.0, 0.0 };
    const FRotator Rotations[] = { {10, 270, 90}, {-30, 90, -90}, {-90, 0, 0} };
    const double Sizes[] = { 62.671261, 62.671261, 88.668060 };
    const double MarakanDepths[] = { 230.0, -120.0, -520.0 };
    const FRotator MarakanRotations[] = { {250, 90, 90}, {170, 90, 90}, {90, 180, 180} };
    const double MarakanSizes[] = { -41.676674, -52.741005, -63.312851 };
    AuthoredScales.Reset(PortalLayers.Num());
    for (int32 Index = 0; Index < PortalLayers.Num(); ++Index)
    {
        const double Depth = bMarakan ? MarakanDepths[Index] : Depths[Index];
        const FRotator Rotation = bMarakan ? MarakanRotations[Index] : Rotations[Index];
        const FVector Scale(bMarakan ? MarakanSizes[Index] : Sizes[Index]);
        PortalLayers[Index]->SetRelativeTransform(FTransform(Rotation, FVector(Depth, 0, 0), Scale));
        AuthoredScales.Add(Scale);
    }
}

void AFarcasterEffectActor::SetPortalScale(float Fraction)
{
    for (int32 Index = 0; Index < AuthoredScales.Num(); ++Index)
    {
        auto* Layer = PortalLayers[Index].Get();
        Layer->SetHiddenInGame(Fraction <= 0.f);
        Layer->SetRelativeScale3D(AuthoredScales[Index] * (Fraction > 0.f ? Fraction : 1.f));
    }
}

void AFarcasterEffectActor::SetOpening(float Progress)
{
    GetWorldTimerManager().ClearTimer(CloseTimer);
    SetPortalScale(FMath::Lerp(0.15f, 1.f, FMath::Clamp(Progress, 0.f, 1.f)));
}

void AFarcasterEffectActor::FlashDeparture()
{
    SetOpening(1.f);
    GetWorldTimerManager().SetTimer(CloseTimer, this, &AFarcasterEffectActor::ClosePortal, 0.25f);
}

void AFarcasterEffectActor::ClosePortal()
{
    GetWorldTimerManager().ClearTimer(CloseTimer);
    SetPortalScale(0.f);
}
