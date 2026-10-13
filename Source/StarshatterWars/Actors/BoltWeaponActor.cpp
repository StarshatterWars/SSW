#include "BoltWeaponActor.h"

#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

ABoltWeaponActor::ABoltWeaponActor()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    TrailFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailFX"));
    TrailFX->SetupAttachment(Root);
    TrailFX->SetAutoActivate(false);
}

ABoltWeaponActor* ABoltWeaponActor::SpawnBolt(UObject* WorldContextObject,
    TSubclassOf<ABoltWeaponActor> BoltClass,
    FVector Origin, FVector Direction, float Speed, float Life, AActor* OwnerActor)
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

    TSubclassOf<ABoltWeaponActor> ClassToSpawn = BoltClass
        ? BoltClass
        : TSubclassOf<ABoltWeaponActor>(ABoltWeaponActor::StaticClass());

    FActorSpawnParameters Params;
    Params.Owner = OwnerActor;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    const FRotator Rot = Direction.IsNearlyZero() ? FRotator::ZeroRotator : Direction.Rotation();
    ABoltWeaponActor* Bolt = World->SpawnActor<ABoltWeaponActor>(ClassToSpawn, Origin, Rot, Params);
    if (Bolt)
    {
        Bolt->Launch(Origin, Direction, Speed, Life);
    }
    return Bolt;
}

void ABoltWeaponActor::BeginPlay()
{
    Super::BeginPlay();
    if (BoltSystem && TrailFX)
    {
        TrailFX->SetAsset(BoltSystem);
    }
}

void ABoltWeaponActor::Launch(FVector Origin, FVector Direction, float Speed, float Life)
{
    MoveDirection = Direction.GetSafeNormal();
    MoveSpeed = Speed;
    LifeRemaining = Life;
    bLaunched = true;
    SetActorLocation(Origin);
    if (!MoveDirection.IsNearlyZero())
    {
        SetActorRotation(MoveDirection.Rotation());
    }
    if (TrailFX)
    {
        TrailFX->SetVariableLinearColor(TEXT("User.Color"), BoltColor);
        TrailFX->SetVariableFloat(TEXT("User.Scale"), BoltScale);
        TrailFX->Activate(true);
    }
}

void ABoltWeaponActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bLaunched) return;

    const FVector Step = MoveDirection * MoveSpeed * DeltaSeconds;
    AddActorWorldOffset(Step, false);

    LifeRemaining -= DeltaSeconds;
    if (LifeRemaining <= 0.0f)
    {
        Expire();
    }
}

void ABoltWeaponActor::Expire()
{
    OnBoltExpired(GetActorLocation());
    if (TrailFX)
    {
        TrailFX->DeactivateImmediate();
    }
    Destroy();
}
