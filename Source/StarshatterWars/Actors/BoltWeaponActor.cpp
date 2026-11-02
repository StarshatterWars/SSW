#include "BoltWeaponActor.h"
#include "AlphaBoltActor.h"
#include "DeltaBoltActor.h"
#include "HeavyBoltActor.h"
#include "FighterBoltActor.h"
#include "LCABoltActor.h"
#include "BlockadeBoltActor.h"
#include "VulcanBoltActor.h"
#include "PhalanxBoltActor.h"
#include "VikingBoltActor.h"
#include "UrsaBoltActor.h"
#include "OrcaBoltActor.h"
#include "OrcaInvertedBoltActor.h"
#include "Orca2BoltActor.h"
#include "Orca2InvertedBoltActor.h"
#include "VanguardBoltActor.h"
#include "HeliconBoltActor.h"
#include "LightPDBBoltActor.h"
#include "Rail5BoltActor.h"
#include "Rail12BoltActor.h"
#include "Rail12InvertedBoltActor.h"
#include "Rail15BoltActor.h"
#include "Mag18BoltActor.h"
#include "AAABoltActor.h"
#include "QuadAAABoltActor.h"


#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ABoltWeaponActor::ABoltWeaponActor()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    SetActorEnableCollision(false);
    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
    ProjectileMesh->SetupAttachment(Root);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProjectileMesh->SetGenerateOverlapEvents(false);
    ProjectileMesh->SetCastShadow(false);
    ProjectileMesh->SetRelativeScale3D(FVector(0.04f)); // 4 cm core; BP may override.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Sphere.Succeeded()) ProjectileMesh->SetStaticMesh(Sphere.Object);

    CoreFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("CoreFX"));
    CoreFX->SetupAttachment(Root);
    CoreFX->SetAutoActivate(false);
    CoreFX->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
    CoreFX->SetRelativeScale3D(FVector(0.01f));
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Energy(
        TEXT("/Game/RocketThrusterExhaustFX/FX/NS_RocketExhaust_Energy.NS_RocketExhaust_Energy"));
    if (Energy.Succeeded()) { CoreSystem = Energy.Object; CoreFX->SetAsset(CoreSystem); }

    TrailFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailFX"));
    TrailFX->SetupAttachment(Root);
    TrailFX->SetAutoActivate(false);
    TrailFX->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
    TrailFX->SetRelativeScale3D(FVector(0.015f));
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Exhaust(
        TEXT("/Game/RocketThrusterExhaustFX/FX/NS_RocketExhaust_Blue.NS_RocketExhaust_Blue"));
    if (Exhaust.Succeeded()) { BoltSystem = Exhaust.Object; TrailFX->SetAsset(BoltSystem); }
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

void ABoltWeaponActor::ApplyPresentationDefaults()
{
    if (ProjectileMesh && BoltMaterial) ProjectileMesh->SetMaterial(0, BoltMaterial);
    if (CoreFX) CoreFX->SetAsset(CoreSystem);
    if (TrailFX) TrailFX->SetAsset(BoltSystem);
}

void ABoltWeaponActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplyPresentationDefaults();
}

void ABoltWeaponActor::BeginPlay()
{
    Super::BeginPlay();
    ApplyPresentationDefaults();
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
    ActivateVisuals();
}

void ABoltWeaponActor::BeginSimulationFlight()
{
    bLaunched = false;
    SetActorTickEnabled(false);
    SetLifeSpan(0.f);
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    ApplyPresentationDefaults();
    ActivateVisuals();
}

void ABoltWeaponActor::ActivateVisuals()
{
    if (CoreFX)
    {
        if (CoreSystem) CoreFX->SetAsset(CoreSystem);
        CoreFX->SetVariableLinearColor(TEXT("User.Color"), BoltColor);
        CoreFX->SetVariableFloat(TEXT("EnginePower"), 1.f);
        CoreFX->SetVariableFloat(TEXT("Throttle"), 100.f);
        CoreFX->Activate(true);
    }
    if (TrailFX)
    {
        if (BoltSystem) TrailFX->SetAsset(BoltSystem);
        TrailFX->SetVariableFloat(TEXT("EnginePower"), 1.f);
        TrailFX->SetVariableFloat(TEXT("Throttle"), 100.f);
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
    if (CoreFX) CoreFX->DeactivateImmediate();
    if (TrailFX)
    {
        TrailFX->DeactivateImmediate();
    }
    Destroy();
}

TSubclassOf<ABoltWeaponActor> ABoltWeaponActor::ClassForWeapon(const FString& WeaponName)
{
    if (WeaponName == TEXT("Alpha Cannon")) return AAlphaBoltActor::StaticClass();
    if (WeaponName == TEXT("Delta Cannon")) return ADeltaBoltActor::StaticClass();
    if (WeaponName == TEXT("Heavy Cannon")) return AHeavyBoltActor::StaticClass();
    if (WeaponName == TEXT("Fighter Turret")) return AFighterBoltActor::StaticClass();
    if (WeaponName == TEXT("LCA Turret")) return ALCABoltActor::StaticClass();
    if (WeaponName == TEXT("Blockade Cannon")) return ABlockadeBoltActor::StaticClass();
    if (WeaponName == TEXT("Vulcan Cannon")) return AVulcanBoltActor::StaticClass();
    if (WeaponName == TEXT("Phalanx Cannon")) return APhalanxBoltActor::StaticClass();
    if (WeaponName == TEXT("Viking Cannon")) return AVikingBoltActor::StaticClass();
    if (WeaponName == TEXT("Ursa Cannon")) return AUrsaBoltActor::StaticClass();
    if (WeaponName == TEXT("Orca Cannon")) return AOrcaBoltActor::StaticClass();
    if (WeaponName == TEXT("Orca Inverted")) return AOrcaInvertedBoltActor::StaticClass();
    if (WeaponName == TEXT("Orca 2 Cannon")) return AOrca2BoltActor::StaticClass();
    if (WeaponName == TEXT("Orca 2 Inverted")) return AOrca2InvertedBoltActor::StaticClass();
    if (WeaponName == TEXT("Vanguard")) return AVanguardBoltActor::StaticClass();
    if (WeaponName == TEXT("Helicon")) return AHeliconBoltActor::StaticClass();
    if (WeaponName == TEXT("Light-PDB")) return ALightPDBBoltActor::StaticClass();
    if (WeaponName == TEXT("Rail-5")) return ARail5BoltActor::StaticClass();
    if (WeaponName == TEXT("Rail-12")) return ARail12BoltActor::StaticClass();
    if (WeaponName == TEXT("Rail-12 Inverted")) return ARail12InvertedBoltActor::StaticClass();
    if (WeaponName == TEXT("Rail-15")) return ARail15BoltActor::StaticClass();
    if (WeaponName == TEXT("Mag-18")) return AMag18BoltActor::StaticClass();
    if (WeaponName == TEXT("AAA Cannon")) return AAAABoltActor::StaticClass();
    if (WeaponName == TEXT("Quad AAA")) return AQuadAAABoltActor::StaticClass();
    return ABoltWeaponActor::StaticClass();
}

FString ABoltWeaponActor::BlueprintPathForWeapon(const FString& WeaponName)
{
    UClass* Native = ClassForWeapon(WeaponName).Get();
    if (Native == ABoltWeaponActor::StaticClass()) return FString();
    const FString BoltName = Native->GetName().LeftChop(5);
    const FString Folder = BoltName.LeftChop(4);
    return FString::Printf(TEXT("/Game/Models/Weapons/Bolt/%s/BP_%s.BP_%s_C"), *Folder, *BoltName, *BoltName);
}

TSubclassOf<ABoltWeaponActor> ABoltWeaponActor::BlueprintClassForWeapon(const FString& WeaponName)
{
    const FString Path = BlueprintPathForWeapon(WeaponName);
    if (!Path.IsEmpty())
        if (UClass* Blueprint = LoadClass<ABoltWeaponActor>(nullptr, *Path)) return Blueprint;
    return ClassForWeapon(WeaponName);
}
