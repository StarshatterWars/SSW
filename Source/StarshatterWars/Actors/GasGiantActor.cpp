#include "GasGiantActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AGasGiantActor::AGasGiantActor()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    CoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreMesh"));
    CoreMesh->SetupAttachment(SceneRoot);
    CoreMesh->SetMobility(EComponentMobility::Movable);
    CoreMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CoreMesh->SetGenerateOverlapEvents(false);
    CoreMesh->SetCastShadow(false);

    // Optional fallback sphere (can be hidden in BP)
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(
        TEXT("/Script/Engine.StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"));

    if (SphereMeshFinder.Succeeded())
    {
        CoreMesh->SetStaticMesh(SphereMeshFinder.Object);
    }
}

void AGasGiantActor::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning,
        TEXT("[GasGiantActor] BeginPlay Actor=%s"),
        *GetName());

    // Do NOT touch materials here
    // Blueprint owns materials + rings
}

void AGasGiantActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bEnableAxialRotation || FMath::IsNearlyZero(AxialRotationDegreesPerSecond))
    {
        return;
    }

    AddActorLocalRotation(
        FRotator(0.0f, AxialRotationDegreesPerSecond * DeltaTime, 0.0f));
}

void AGasGiantActor::SetPlanetRadius(float InRadiusUnits)
{
    PlanetRadiusUnits = FMath::Max(InRadiusUnits, 1.0f);

    const float MeshRadius = 50.0f;
    const float Scale = PlanetRadiusUnits / MeshRadius;

    SetActorScale3D(FVector(Scale));

    UE_LOG(LogTemp, Warning,
        TEXT("[GasGiantActor] SetPlanetRadius Actor=%s RadiusUnits=%.2f Scale=%.3f"),
        *GetName(),
        PlanetRadiusUnits,
        Scale);
}

void AGasGiantActor::SetLightDirection(const FVector& InDirection)
{
    FVector SafeDirection = InDirection;

    if (SafeDirection.IsNearlyZero())
    {
        SafeDirection = FVector(1, 0, 0);
    }

    LightDirection = SafeDirection.GetSafeNormal();

    // Blueprint can read this variable and drive material parameters
    UE_LOG(LogTemp, Warning,
        TEXT("[GasGiantActor] LightDirection Actor=%s Dir=%s"),
        *GetName(),
        *LightDirection.ToString());
}

void AGasGiantActor::SetAxialRotationDegreesPerSecond(float DegreesPerSecond)
{
    AxialRotationDegreesPerSecond = DegreesPerSecond;
    bEnableAxialRotation = !FMath::IsNearlyZero(DegreesPerSecond);
}

void AGasGiantActor::SetAxialRotationEnabled(bool bEnabled)
{
    bEnableAxialRotation = bEnabled;
}

float AGasGiantActor::GetPlanetRadius() const
{
    return PlanetRadiusUnits;
}