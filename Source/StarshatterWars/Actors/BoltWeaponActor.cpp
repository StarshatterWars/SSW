#include "BoltWeaponActor.h"
#include "AlphaCannonBoltActor.h"
#include "DeltaCannonBoltActor.h"
#include "HeavyCannonBoltActor.h"
#include "FighterTurretBoltActor.h"
#include "LCATurretBoltActor.h"
#include "BlockadeCannonBoltActor.h"
#include "VulcanCannonBoltActor.h"
#include "PhalanxCannonBoltActor.h"
#include "VikingCannonBoltActor.h"
#include "UrsaCannonBoltActor.h"
#include "OrcaCannonBoltActor.h"
#include "OrcaInvertedBoltActor.h"
#include "Orca2CannonBoltActor.h"
#include "Orca2InvertedBoltActor.h"
#include "VanguardBoltActor.h"
#include "HeliconBoltActor.h"
#include "LightPDBBoltActor.h"
#include "Rail5BoltActor.h"
#include "Rail12BoltActor.h"
#include "Rail12InvertedBoltActor.h"
#include "Rail15BoltActor.h"
#include "Mag18BoltActor.h"
#include "AAACannonBoltActor.h"
#include "QuadAAABoltActor.h"


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

TSubclassOf<ABoltWeaponActor> ABoltWeaponActor::ClassForWeapon(const FString& WeaponName)
{
    if (WeaponName == TEXT("Alpha Cannon")) return AAlphaCannonBoltActor::StaticClass();
    if (WeaponName == TEXT("Delta Cannon")) return ADeltaCannonBoltActor::StaticClass();
    if (WeaponName == TEXT("Heavy Cannon")) return AHeavyCannonBoltActor::StaticClass();
    if (WeaponName == TEXT("Fighter Turret")) return AFighterTurretBoltActor::StaticClass();
    if (WeaponName == TEXT("LCA Turret")) return ALCATurretBoltActor::StaticClass();
    if (WeaponName == TEXT("Blockade Cannon")) return ABlockadeCannonBoltActor::StaticClass();
    if (WeaponName == TEXT("Vulcan Cannon")) return AVulcanCannonBoltActor::StaticClass();
    if (WeaponName == TEXT("Phalanx Cannon")) return APhalanxCannonBoltActor::StaticClass();
    if (WeaponName == TEXT("Viking Cannon")) return AVikingCannonBoltActor::StaticClass();
    if (WeaponName == TEXT("Ursa Cannon")) return AUrsaCannonBoltActor::StaticClass();
    if (WeaponName == TEXT("Orca Cannon")) return AOrcaCannonBoltActor::StaticClass();
    if (WeaponName == TEXT("Orca Inverted")) return AOrcaInvertedBoltActor::StaticClass();
    if (WeaponName == TEXT("Orca 2 Cannon")) return AOrca2CannonBoltActor::StaticClass();
    if (WeaponName == TEXT("Orca 2 Inverted")) return AOrca2InvertedBoltActor::StaticClass();
    if (WeaponName == TEXT("Vanguard")) return AVanguardBoltActor::StaticClass();
    if (WeaponName == TEXT("Helicon")) return AHeliconBoltActor::StaticClass();
    if (WeaponName == TEXT("Light-PDB")) return ALightPDBBoltActor::StaticClass();
    if (WeaponName == TEXT("Rail-5")) return ARail5BoltActor::StaticClass();
    if (WeaponName == TEXT("Rail-12")) return ARail12BoltActor::StaticClass();
    if (WeaponName == TEXT("Rail-12 Inverted")) return ARail12InvertedBoltActor::StaticClass();
    if (WeaponName == TEXT("Rail-15")) return ARail15BoltActor::StaticClass();
    if (WeaponName == TEXT("Mag-18")) return AMag18BoltActor::StaticClass();
    if (WeaponName == TEXT("AAA Cannon")) return AAAACannonBoltActor::StaticClass();
    if (WeaponName == TEXT("Quad AAA")) return AQuadAAABoltActor::StaticClass();
    return ABoltWeaponActor::StaticClass();
}
