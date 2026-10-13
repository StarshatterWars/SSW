#include "BeamWeaponActor.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

ABeamWeaponActor::ABeamWeaponActor()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    BeamFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BeamFX"));
    BeamFX->SetupAttachment(Root);
    BeamFX->SetAutoActivate(false);
}

ABeamWeaponActor* ABeamWeaponActor::SpawnBeam(UObject* WorldContextObject, TSubclassOf<ABeamWeaponActor> BeamClass,
    FVector Start, FVector End, AActor* OwnerActor)
{
    UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
    if (!World && GEngine)
    {
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            if (UWorld* Candidate = Context.World())
            {
                if (Candidate->WorldType == EWorldType::Game || Candidate->WorldType == EWorldType::PIE)
                {
                    World = Candidate;
                    break;
                }
            }
        }
    }
    if (!World) return nullptr;

    TSubclassOf<ABeamWeaponActor> ClassToSpawn = BeamClass ? BeamClass : TSubclassOf<ABeamWeaponActor>(ABeamWeaponActor::StaticClass());
    FActorSpawnParameters Params;
    Params.Owner = OwnerActor;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ABeamWeaponActor* Beam = World->SpawnActor<ABeamWeaponActor>(ClassToSpawn, Start, FRotator::ZeroRotator, Params);
    if (Beam)
    {
        Beam->SetBeamPoints(Start, End);
        Beam->SetBeamActive(true);
    }
    return Beam;
}

void ABeamWeaponActor::BeginPlay()
{
    Super::BeginPlay();
    if (BeamSystem && BeamFX)
    {
        BeamFX->SetAsset(BeamSystem);
    }
    ApplyVisual();
}

void ABeamWeaponActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (LifeTime > 0.0f)
    {
        Age += DeltaSeconds;
        if (Age >= LifeTime)
        {
            Destroy();
        }
    }
}

void ABeamWeaponActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (BeamFX)
    {
        BeamFX->DeactivateImmediate();
    }
    Super::EndPlay(EndPlayReason);
}

void ABeamWeaponActor::SetBeamPoints(FVector Start, FVector End)
{
    BeamStart = Start;
    BeamEnd = End;
    SetActorLocation(Start);
    ApplyVisual();
    OnBeamUpdated(Start, End, FVector::Distance(Start, End));
}

void ABeamWeaponActor::SetBeamActive(bool bActive)
{
    bBeamActive = bActive;
    if (BeamFX)
    {
        if (bActive) BeamFX->Activate(true);
        else BeamFX->Deactivate();
    }
    if (!bActive && bAutoDestroyOnDeactivate)
    {
        Destroy();
    }
}

void ABeamWeaponActor::SetBeamColor(FLinearColor Color)
{
    BeamColor = Color;
    ApplyVisual();
}

void ABeamWeaponActor::SetBeamWidth(float Width)
{
    BeamWidth = Width;
    ApplyVisual();
}

void ABeamWeaponActor::ApplyVisual()
{
    if (!BeamFX) return;

    // Common Niagara beam user parameters. A Blueprint child can ignore these
    // and drive its own system from OnBeamUpdated instead.
    BeamFX->SetVariableVec3(TEXT("User.BeamStart"), FVector::ZeroVector);
    BeamFX->SetVariableVec3(TEXT("User.BeamEnd"), BeamEnd - BeamStart);
    BeamFX->SetVariableLinearColor(TEXT("User.BeamColor"), BeamColor);
    BeamFX->SetVariableFloat(TEXT("User.BeamWidth"), BeamWidth);
    BeamFX->SetVariableFloat(TEXT("User.BeamLength"), FVector::Distance(BeamStart, BeamEnd));

    if (bBeamActive && !BeamFX->IsActive())
    {
        BeamFX->Activate(true);
    }
}
