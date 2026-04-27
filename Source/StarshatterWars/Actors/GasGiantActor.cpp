#include "GasGiantActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/UnrealType.h"

#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"


static void SetBPFloatProperty(UObject* Object, const FName PropertyName, float Value)
{
    if (!Object)
    {
        return;
    }

    FFloatProperty* FloatProp =
        FindFProperty<FFloatProperty>(Object->GetClass(), PropertyName);

    if (!FloatProp)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[GasGiantActor] BP float property not found: %s"),
            *PropertyName.ToString());
        return;
    }

    FloatProp->SetPropertyValue_InContainer(Object, Value);

    UE_LOG(LogTemp, Warning,
        TEXT("[GasGiantActor] Set BP float %s = %.3f"),
        *PropertyName.ToString(),
        Value);
}

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
    InitialActorScale = GetActorScale3D();
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

    const float BaseMeshRadius = 50.0f;
    const float PlanetScale = PlanetRadiusUnits / BaseMeshRadius;

    TArray<UStaticMeshComponent*> Meshes;
    GetComponents<UStaticMeshComponent>(Meshes);

    for (UStaticMeshComponent* MeshComp : Meshes)
    {
        if (!MeshComp)
        {
            continue;
        }

        const FString MeshName = MeshComp->GetName();

        const bool bIsRing =
            MeshName.Contains(TEXT("Ring"), ESearchCase::IgnoreCase) ||
            MeshName.Contains(TEXT("Rings"), ESearchCase::IgnoreCase);

        MeshComp->SetVisibility(true, true);
        MeshComp->SetHiddenInGame(false, true);
        MeshComp->SetCullDistance(0.0f);
        MeshComp->SetBoundsScale(10000.0f);
        MeshComp->SetMobility(EComponentMobility::Movable);

        if (bIsRing)
        {
            // Keep asset-pack ring transform for now.
            // Do not scale rings here.
            UE_LOG(LogTemp, Warning,
                TEXT("[GasGiantActor] Ring preserved Mesh=%s Loc=%s Scale=%s Mat=%s"),
                *MeshName,
                *MeshComp->GetRelativeLocation().ToString(),
                *MeshComp->GetRelativeScale3D().ToString(),
                *GetNameSafe(MeshComp->GetMaterial(0)));

            continue;
        }

        MeshComp->SetRelativeScale3D(FVector(PlanetScale));

        UE_LOG(LogTemp, Warning,
            TEXT("[GasGiantActor] Planet mesh scaled Mesh=%s Scale=%.2f"),
            *MeshName,
            PlanetScale);
    }

    OnGasGiantRadiusChanged();
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

#include "UObject/UnrealType.h"
#include "Components/StaticMeshComponent.h"

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
        *MaterialBasePath,
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
    // SET BP VARIABLE (safe to keep)
    // ----------------------------------------------------
    FObjectProperty* MaterialProperty =
        FindFProperty<FObjectProperty>(GetClass(), TEXT("Planet Material"));

    if (MaterialProperty)
    {
        MaterialProperty->SetObjectPropertyValue_InContainer(this, Mat);
    }

    // ----------------------------------------------------
    // COMMON VALUES
    // ----------------------------------------------------
    const float BaseMeshRadius = 50.0f;
    const float NormalizedRadius = PlanetRadiusUnits / BaseMeshRadius;
    const FVector PlanetCenter = GetActorLocation();

    // ----------------------------------------------------
    // APPLY MATERIALS
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

        const bool bIsRing =
            MeshName.Contains(TEXT("Ring"), ESearchCase::IgnoreCase) ||
            MeshName.Contains(TEXT("Rings"), ESearchCase::IgnoreCase);

        // ==================================================
        // RINGS (FIXED: no MID stacking)
        // ==================================================
        if (bIsRing)
        {
            MeshComp->SetVisibility(true, true);
            MeshComp->SetHiddenInGame(false, true);
            MeshComp->SetCullDistance(0.0f);
            MeshComp->SetBoundsScale(10000.0f);

            UMaterialInterface* RingMat = MeshComp->GetMaterial(0);

            if (!RingMat)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("[GasGiantActor] Ring mesh has no material Mesh=%s"),
                    *MeshName);
            }
            else
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("[GasGiantActor] Ring visible/preserved Mesh=%s Mat=%s"),
                    *MeshName,
                    *GetNameSafe(RingMat));
            }

            continue;
        }

        // ==================================================
        // PLANET
        // ==================================================
        UMaterialInstanceDynamic* MID =
            UMaterialInstanceDynamic::Create(Mat, this);

        MeshComp->SetMaterial(0, MID);

        if (MID)
        {
            MID->SetScalarParameterValue(
                TEXT("PlanetRadius"),
                NormalizedRadius);

            MID->SetVectorParameterValue(
                TEXT("PlanetCenterWS"),
                FLinearColor(
                    PlanetCenter.X,
                    PlanetCenter.Y,
                    PlanetCenter.Z,
                    0.0f));

            MID->SetVectorParameterValue(
                TEXT("LightDirection"),
                FLinearColor(
                    LightDirection.X,
                    LightDirection.Y,
                    LightDirection.Z,
                    0.0f));

            MID->SetScalarParameterValue(TEXT("LightIntensity"), 1.0f);

            DynamicMaterial = MID;

            UE_LOG(LogTemp, Warning,
                TEXT("[GasGiantActor] PlanetMID applied Mesh=%s"),
                *MeshName);
        }
    }

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

void AGasGiantActor::UpdateRingParameters(float PlanetScale)
{
    TArray<UStaticMeshComponent*> Meshes;
    GetComponents<UStaticMeshComponent>(Meshes);

    for (UStaticMeshComponent* MeshComp : Meshes)
    {
        if (!MeshComp)
        {
            continue;
        }

        if (!MeshComp->GetName().Contains(TEXT("Ring"), ESearchCase::IgnoreCase))
        {
            continue;
        }

        UMaterialInstanceDynamic* MID =
            MeshComp->CreateAndSetMaterialInstanceDynamic(0);

        if (!MID)
        {
            continue;
        }

        const float Inner = PlanetScale * 1.2f;
        const float Outer = PlanetScale * 2.0f;

        MID->SetScalarParameterValue(TEXT("Inner_Radius"), Inner);
        MID->SetScalarParameterValue(TEXT("Outer_Radius"), Outer);

        MID->SetScalarParameterValue(TEXT("Position"), 0.0f);

        UE_LOG(LogTemp, Warning,
            TEXT("[GasGiantActor] RingParams Inner=%.2f Outer=%.2f Scale=%.2f"),
            Inner, Outer, PlanetScale);
    }
}

void AGasGiantActor::ApplyRingSettingsToBlueprint()
{
    auto SetFloat = [&](const FString& DisplayName, float Value)
        {
            for (TFieldIterator<FFloatProperty> It(GetClass()); It; ++It)
            {
                FFloatProperty* Prop = *It;

                if (!Prop)
                {
                    continue;
                }

                const FString Friendly =
                    Prop->GetMetaData(TEXT("DisplayName"));

                const FString Internal =
                    Prop->GetName();

                if (Friendly.Equals(DisplayName, ESearchCase::IgnoreCase) ||
                    Internal.Equals(DisplayName, ESearchCase::IgnoreCase))
                {
                    Prop->SetPropertyValue_InContainer(this, Value);

                    UE_LOG(LogTemp, Warning,
                        TEXT("[GasGiantActor] Set BP '%s' (Internal=%s) = %.3f Actor=%s"),
                        *DisplayName,
                        *Internal,
                        Value,
                        *GetName());

                    return;
                }
            }

            UE_LOG(LogTemp, Error,
                TEXT("[GasGiantActor] BP variable NOT FOUND: %s Actor=%s"),
                *DisplayName,
                *GetName());
        };

    // ----------------------------------------------------
    // APPLY VALUES
    // ----------------------------------------------------
    SetFloat(TEXT("Inner Radius"), InnerRingRadius);
    SetFloat(TEXT("Outer Radius"), OuterRingRadius);
    SetFloat(TEXT("Position"), RingPosition);

    // ----------------------------------------------------
    // NOTIFY BP TO REBUILD RINGS
    // ----------------------------------------------------
    OnGasGiantRadiusChanged();

    UE_LOG(LogTemp, Warning,
        TEXT("[GasGiantActor] ApplyRingSettingsToBlueprint Complete Inner=%.2f Outer=%.2f Position=%.2f"),
        InnerRingRadius,
        OuterRingRadius,
        RingPosition);
}

void AGasGiantActor::ApplyRingRadiusSettings()
{
    // These are normalized ring values used by the asset BP/material.
    // Do NOT use world radius here.
    const float InnerRadius = 1.2f;
    const float OuterRadius = 2.0f;
    const float Position = 0.0f;

    SetBPFloatProperty(this, TEXT("Inner Radius"), InnerRadius);
    SetBPFloatProperty(this, TEXT("Outer Radius"), OuterRadius);
    SetBPFloatProperty(this, TEXT("Position"), Position);

    OnGasGiantRadiusChanged();

    UE_LOG(LogTemp, Warning,
        TEXT("[GasGiantActor] ApplyRingRadiusSettings Inner=%.2f Outer=%.2f Position=%.2f"),
        InnerRadius,
        OuterRadius,
        Position);
}