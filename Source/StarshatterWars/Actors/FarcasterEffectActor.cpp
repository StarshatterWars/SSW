#include "FarcasterEffectActor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

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
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Departure(TEXT("/Game/Advanced_Space_Fx_Pack1/FX/Electric_Shockwaves/NS_Electric_Shockwave_Yellow.NS_Electric_Shockwave_Yellow"));
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Arrival(TEXT("/Game/Advanced_Space_Fx_Pack1/FX/Electric_Shockwaves/NS_Electric_Shockwave_Blue.NS_Electric_Shockwave_Blue"));
    DepartureSystem=Departure.Object; ArrivalSystem=Arrival.Object;
    for (int32 Index=0;Index<5;++Index)
    {
        auto* Ring=CreateDefaultSubobject<UNiagaraComponent>(*FString::Printf(TEXT("VortexRing%d"),Index));
        Ring->SetupAttachment(RootComponent);
        Ring->SetAutoActivate(false);
        Ring->SetAsset(DepartureSystem);
        Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Ring->SetCastShadow(false);
        VortexRings.Add(Ring);
    }

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
    const bool bWasOpen=OpeningFraction>0.f;
    OpeningFraction=FMath::Clamp(Fraction,0.f,1.f);
    SetActorTickEnabled(OpeningFraction>0.f);
    if (OpeningFraction>0.f && !bWasOpen)
    {
        EffectAge=0.f;
        for (const auto& Ring : VortexRings) { Ring->SetVisibility(true); Ring->Activate(true); }
    }
    else if (OpeningFraction<=0.f)
        for (const auto& Ring : VortexRings) { Ring->DeactivateImmediate(); Ring->SetVisibility(false); }

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
    SetArrivalAppearance(false);
    SetOpening(1.f);
    GetWorldTimerManager().SetTimer(CloseTimer, this, &AFarcasterEffectActor::ClosePortal, 0.25f);
}

void AFarcasterEffectActor::ClosePortal()
{
    GetWorldTimerManager().ClearTimer(CloseTimer);
    SetPortalScale(0.f);
    SetArrivalAppearance(false);
}


void AFarcasterEffectActor::SetArrivalAppearance(bool bArrival)
{
    bArrivalEffect=bArrival;
    const TCHAR* Blue=TEXT("/Game/WarpGates/Materials/Effects/MI_WarpEffect_CircleBlue.MI_WarpEffect_CircleBlue");
    const TCHAR* Green=TEXT("/Game/WarpGates/Materials/Effects/MI_WarpEffect_CircleGreen.MI_WarpEffect_CircleGreen");
    const TCHAR* Red=TEXT("/Game/WarpGates/Materials/Effects/MI_WarpEffect_CircleRed.MI_WarpEffect_CircleRed");
    for (int32 Index=0;Index<PortalLayers.Num();++Index)
        if (auto* Material=LoadObject<UMaterialInterface>(nullptr,bArrival ? (Index==1?Green:Blue) : Red)) PortalLayers[Index]->SetMaterial(0,Material);
    for (const auto& Ring : VortexRings)
    {
        Ring->SetAsset(bArrival?ArrivalSystem:DepartureSystem);
        Ring->SetVariableLinearColor(TEXT("User.Color"), bArrival?FLinearColor(.05f,.65f,1.f):FLinearColor(1.f,.45f,.03f));
    }
}

void AFarcasterEffectActor::FlashArrival()
{
    SetArrivalAppearance(true);
    SetOpening(1.f);
}

void AFarcasterEffectActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    EffectAge+=FMath::Max(0.f,DeltaSeconds);
    // Concentric particles taper through the portal normal into a short funnel.
    for (int32 Index=0;Index<VortexRings.Num();++Index)
    {
        auto* Ring=VortexRings[Index].Get();
        const float Depth=float(Index)/FMath::Max(1,VortexRings.Num()-1);
        const float Radius=(1.f-.62f*Depth)*OpeningFraction;
        Ring->SetRelativeLocation(FVector((bArrivalEffect?-1.f:1.f)*Depth*2400.f,0,0));
        Ring->SetRelativeRotation(FRotator(0,90,EffectAge*(18.f+Index*7.f)));
        Ring->SetRelativeScale3D(FVector(Radius*5.f));
        if (EffectAge>.5f && !Ring->IsActive()) Ring->Activate(true);
    }
}
