#include "GasGiantActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/UnrealType.h"

#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
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

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));

    if (SphereMeshFinder.Succeeded())
    {
        CoreMesh->SetStaticMesh(SphereMeshFinder.Object);
    }
}

void AGasGiantActor::BeginPlay()
{
    Super::BeginPlay();

    CreateMID();

    UE_LOG(LogTemp, Warning,
        TEXT("[GasGiantActor] BeginPlay Actor=%s"),
        *GetName());
}

void AGasGiantActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AGasGiantActor::CreateMID()
{
    if (!CoreMesh)
    {
        return;
    }

    UMaterialInterface* BaseMat = CoreMesh->GetMaterial(0);

    if (BaseMat)
    {
        DynamicMaterial = CoreMesh->CreateAndSetMaterialInstanceDynamic(0);
    }
}

void AGasGiantActor::SetPlanetRadius(float InRadiusUnits)
{
    PlanetRadiusUnits = FMath::Max(InRadiusUnits, 1.0f);

    const float MeshRadius = 50.0f;
    const float Scale = PlanetRadiusUnits / MeshRadius;

    SetActorScale3D(FVector(Scale));

    UE_LOG(LogTemp, Warning,
        TEXT("[GasGiantActor] Radius=%f Scale=%f"),
        PlanetRadiusUnits,
        Scale);
}

float AGasGiantActor::GetPlanetRadius() const
{
    return PlanetRadiusUnits;
}

void AGasGiantActor::SetLightDirection(const FVector& InDirection)
{
    LightDirection = InDirection.IsNearlyZero()
        ? FVector(1, 0, 0)
        : InDirection.GetSafeNormal();

    if (DynamicMaterial)
    {
        DynamicMaterial->SetVectorParameterValue(
            TEXT("LightDirection"),
            FLinearColor(
                LightDirection.X,
                LightDirection.Y,
                LightDirection.Z));
    }
}

void AGasGiantActor::SetGasGiantMaterialByName(const FString& MaterialName)
{
    const FString CleanName = MaterialName.TrimStartAndEnd();

    if (CleanName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[GasGiantActor] Empty material name Actor=%s"),
            *GetName());
        return;
    }

    // ----------------------------------------------------
    // BUILD MATERIAL PATH
    // ----------------------------------------------------
    const FString AssetName = CleanName.StartsWith(TEXT("MI_"))
        ? CleanName
        : FString(TEXT("MI_")) + CleanName;

    const FString Path = FString::Printf(
        TEXT("/Script/Engine.MaterialInterface'%s%s.%s'"),
        *MaterialBasePath,   // "/Game/GameData/Galaxy/GasGiants/"
        *AssetName,
        *AssetName);

    UE_LOG(LogTemp, Warning,
        TEXT("[GasGiantActor] Loading Material Path=%s"),
        *Path);

    UMaterialInterface* Mat =
        LoadObject<UMaterialInterface>(nullptr, *Path);

    if (!Mat)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[GasGiantActor] FAILED to load material '%s' Actor=%s"),
            *AssetName,
            *GetName());
        return;
    }

    // ----------------------------------------------------
    // SET BLUEPRINT VARIABLE (Planet Material)
    // ----------------------------------------------------
    FObjectProperty* MaterialProperty =
        FindFProperty<FObjectProperty>(GetClass(), TEXT("Planet Material"));

    if (MaterialProperty)
    {
        MaterialProperty->SetObjectPropertyValue_InContainer(this, Mat);

        UE_LOG(LogTemp, Warning,
            TEXT("[GasGiantActor] Set BP variable 'Planet Material' = %s Actor=%s"),
            *GetNameSafe(Mat),
            *GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("[GasGiantActor] BP variable 'Planet Material' NOT FOUND Actor=%s"),
            *GetName());
    }

    // ----------------------------------------------------
    // APPLY DIRECTLY TO MESHES (CRITICAL FALLBACK)
    // ----------------------------------------------------
    TArray<UStaticMeshComponent*> Meshes;
    GetComponents<UStaticMeshComponent>(Meshes);

    for (UStaticMeshComponent* MeshComp : Meshes)
    {
        if (!MeshComp)
        {
            continue;
        }

        const FString MeshName = MeshComp->GetName();

        // Skip rings
        if (MeshName.Contains(TEXT("Ring"), ESearchCase::IgnoreCase))
        {
            continue;
        }

        MeshComp->SetMaterial(0, Mat);

        UE_LOG(LogTemp, Warning,
            TEXT("[GasGiantActor] Direct-applied %s to %s"),
            *GetNameSafe(Mat),
            *MeshName);

        // ------------------------------------------------
        // CREATE MID + SET BASIC PARAMS (avoid black)
        // ------------------------------------------------
        UMaterialInstanceDynamic* MID =
            MeshComp->CreateAndSetMaterialInstanceDynamic(0);

        if (MID)
        {
            MID->SetScalarParameterValue(TEXT("LightIntensity"), 1.0f);

            MID->SetVectorParameterValue(
                TEXT("LightDirection"),
                FLinearColor(
                    LightDirection.X,
                    LightDirection.Y,
                    LightDirection.Z,
                    0.0f));
        }
    }

    // ----------------------------------------------------
    // TRIGGER BP REFRESH (procedural systems)
    // ----------------------------------------------------
    OnGasGiantMaterialChanged();
}

void AGasGiantActor::SetBandSpeed(float InSpeed)
{
    if (DynamicMaterial)
    {
        DynamicMaterial->SetScalarParameterValue(TEXT("BandSpeed"), InSpeed);
    }
}

void AGasGiantActor::SetStormIntensity(float InIntensity)
{
    if (DynamicMaterial)
    {
        DynamicMaterial->SetScalarParameterValue(TEXT("StormIntensity"), InIntensity);
    }
}