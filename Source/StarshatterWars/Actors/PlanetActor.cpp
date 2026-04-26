/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         PlanetActor.cpp
    AUTHOR:       Carlos Bott
*/

#include "PlanetActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

APlanetActor::APlanetActor()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    PlanetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlanetMesh"));
    PlanetMesh->SetupAttachment(SceneRoot);
    PlanetMesh->SetMobility(EComponentMobility::Movable);
    PlanetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PlanetMesh->SetGenerateOverlapEvents(false);
    PlanetMesh->SetCastShadow(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(
        TEXT("/Script/Engine.StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"));

    if (SphereMeshFinder.Succeeded())
    {
        DefaultSphereMesh = SphereMeshFinder.Object;
        PlanetMesh->SetStaticMesh(DefaultSphereMesh);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlanetMaterialFinder(
        TEXT("/Script/Engine.Material'/Game/GameData/Galaxy/PlanetMaterials/M_Planet.M_Planet'"));

    if (PlanetMaterialFinder.Succeeded())
    {
        DefaultPlanetMaterial = PlanetMaterialFinder.Object;
        PlanetMesh->SetMaterial(0, DefaultPlanetMaterial);
    }
}

void APlanetActor::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] BeginPlay: Actor=%s Class=%s"),
        *GetName(),
        *GetClass()->GetName());

    TArray<UStaticMeshComponent*> Meshes;
    GetComponents<UStaticMeshComponent>(Meshes);

    for (UStaticMeshComponent* MeshComp : Meshes)
    {
        if (!MeshComp)
        {
            continue;
        }

        const bool bIsMainPlanetMesh = (MeshComp == PlanetMesh);

        MeshComp->SetVisibility(bIsMainPlanetMesh, true);
        MeshComp->SetHiddenInGame(!bIsMainPlanetMesh, true);
        MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        MeshComp->SetGenerateOverlapEvents(false);
        MeshComp->SetCastShadow(false);

        UE_LOG(LogTemp, Warning,
            TEXT("[PlanetActor] MeshVisibility Actor=%s Mesh=%s IsPlanetMesh=%d Visible=%d Hidden=%d"),
            *GetName(),
            *MeshComp->GetName(),
            bIsMainPlanetMesh ? 1 : 0,
            MeshComp->IsVisible() ? 1 : 0,
            MeshComp->bHiddenInGame ? 1 : 0);
    }

    if (PlanetMesh)
    {
        PlanetMesh->SetVisibility(true, true);
        PlanetMesh->SetHiddenInGame(false, true);
        PlanetMesh->SetCullDistance(0.0f);
        PlanetMesh->SetBoundsScale(10000.0f);

        UE_LOG(LogTemp, Error,
            TEXT("[PlanetActor] FORCE VISIBLE Actor=%s ActorLoc=%s ActorScale=%s MeshScale=%s MeshWorldScale=%s Mesh=%s Mat=%s"),
            *GetName(),
            *GetActorLocation().ToString(),
            *GetActorScale3D().ToString(),
            *PlanetMesh->GetRelativeScale3D().ToString(),
            *PlanetMesh->GetComponentScale().ToString(),
            *GetNameSafe(PlanetMesh->GetStaticMesh()),
            *GetNameSafe(PlanetMesh->GetMaterial(0)));
    }

    EnsureDynamicMaterial();
    ApplyMaterialParameters();

    DumpPlanetMaterialState(TEXT("BeginPlay"));
}

void APlanetActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bEnableAxialRotation || FMath::IsNearlyZero(AxialRotationDegreesPerSecond))
    {
        return;
    }

    if (PlanetMesh)
    {
        PlanetMesh->AddLocalRotation(
            FRotator(0.0f, AxialRotationDegreesPerSecond * DeltaTime, 0.0f));
    }
}

void APlanetActor::SetPlanetRadius(float InRadiusUnits)
{
    if (!PlanetMesh)
    {
        return;
    }

    const float MeshRadius = 50.0f;
    const float Scale = FMath::Max(InRadiusUnits, 1.0f) / MeshRadius;

    PlanetMesh->SetRelativeScale3D(FVector(Scale));

    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] SetPlanetRadius: Actor=%s RadiusUnits=%.2f MeshScale=%.3f"),
        *GetName(),
        InRadiusUnits,
        Scale);
}

void APlanetActor::SetPlanetMaterial(UMaterialInterface* InMaterial)
{
    if (!PlanetMesh || !InMaterial)
    {
        return;
    }

    if (DynamicPlanetMaterial)
    {
        PlanetMesh->SetMaterial(0, DynamicPlanetMaterial);
        ApplyMaterialParameters();
        return;
    }

    PlanetMesh->SetMaterial(0, InMaterial);
    DynamicPlanetMaterial = PlanetMesh->CreateAndSetMaterialInstanceDynamic(0);

    ApplyMaterialParameters();
}

void APlanetActor::SetBaseTexture(UTexture* InTexture)
{
    BaseTexture = InTexture;
    ApplyMaterialParameters();
}

void APlanetActor::SetGlossTexture(UTexture* InTexture)
{
    GlossTexture = InTexture;
    ApplyMaterialParameters();
}

void APlanetActor::SetLightsTexture(UTexture* InTexture)
{
    LightsTexture = InTexture;
    ApplyMaterialParameters();
}

void APlanetActor::SetPlanetTextures(
    UTexture* InBaseTexture,
    UTexture* InGlossTexture,
    UTexture* InLightsTexture)
{
    BaseTexture = InBaseTexture;
    GlossTexture = InGlossTexture;
    LightsTexture = InLightsTexture;

    UE_LOG(LogTemp, Error,
        TEXT("[PlanetActor] SetPlanetTextures Actor=%s Base=%s Gloss=%s Lights=%s"),
        *GetName(),
        *GetNameSafe(BaseTexture),
        *GetNameSafe(GlossTexture),
        *GetNameSafe(LightsTexture));

    ApplyMaterialParameters();
}

void APlanetActor::SetLightDirection(const FVector& InDirection)
{
    FVector SafeDirection = InDirection;

    if (SafeDirection.IsNearlyZero())
    {
        SafeDirection = FVector(1.0f, 0.0f, 0.0f);
    }

    LightDirection = SafeDirection.GetSafeNormal();
    ApplyMaterialParameters();
}

void APlanetActor::SetLightsIntensity(float InIntensity)
{
    LightsIntensity = FMath::Max(0.0f, InIntensity);
    ApplyMaterialParameters();
}

void APlanetActor::SetNightFalloff(float InFalloff)
{
    NightFalloff = FMath::Max(0.01f, InFalloff);
    ApplyMaterialParameters();
}

void APlanetActor::SetAtmosphereColor(const FLinearColor& InColor)
{
    AtmosphereColor = InColor;
    ApplyMaterialParameters();
}

void APlanetActor::SetAtmosphereIntensity(float InIntensity)
{
    AtmosphereIntensity = FMath::Max(0.0f, InIntensity);
    ApplyMaterialParameters();
}

void APlanetActor::SetAxialRotationDegreesPerSecond(float DegreesPerSecond)
{
    AxialRotationDegreesPerSecond = DegreesPerSecond;
    bEnableAxialRotation = !FMath::IsNearlyZero(AxialRotationDegreesPerSecond);
}

void APlanetActor::SetAxialRotationEnabled(bool bEnabled)
{
    bEnableAxialRotation = bEnabled;
}

void APlanetActor::EnsureDynamicMaterial()
{
    if (!PlanetMesh)
    {
        return;
    }

    if (DynamicPlanetMaterial)
    {
        PlanetMesh->SetMaterial(0, DynamicPlanetMaterial);
        return;
    }

    UMaterialInterface* CurrentMaterial = PlanetMesh->GetMaterial(0);

    if (!CurrentMaterial && DefaultPlanetMaterial)
    {
        PlanetMesh->SetMaterial(0, DefaultPlanetMaterial);
        CurrentMaterial = DefaultPlanetMaterial;
    }

    if (CurrentMaterial)
    {
        DynamicPlanetMaterial = PlanetMesh->CreateAndSetMaterialInstanceDynamic(0);
    }
}

void APlanetActor::ApplyMaterialParameters()
{
    EnsureDynamicMaterial();

    if (!DynamicPlanetMaterial)
    {
        return;
    }

    if (PlanetMesh)
    {
        PlanetMesh->SetMaterial(0, DynamicPlanetMaterial);
    }

    if (BaseTexture)
    {
        DynamicPlanetMaterial->SetTextureParameterValue(TEXT("BaseTexture"), BaseTexture);
        DynamicPlanetMaterial->SetTextureParameterValue(TEXT("PlanetTexture"), BaseTexture);
        DynamicPlanetMaterial->SetTextureParameterValue(TEXT("AlbedoTexture"), BaseTexture);
    }

    if (GlossTexture)
    {
        DynamicPlanetMaterial->SetTextureParameterValue(TEXT("GlossTexture"), GlossTexture);
        DynamicPlanetMaterial->SetTextureParameterValue(TEXT("RoughnessTexture"), GlossTexture);
    }

    if (LightsTexture)
    {
        DynamicPlanetMaterial->SetTextureParameterValue(TEXT("LightsTexture"), LightsTexture);
        DynamicPlanetMaterial->SetTextureParameterValue(TEXT("CityLightsTexture"), LightsTexture);
    }

    DynamicPlanetMaterial->SetVectorParameterValue(
        TEXT("LightDirection"),
        FLinearColor(LightDirection.X, LightDirection.Y, LightDirection.Z, 0.0f));

    DynamicPlanetMaterial->SetScalarParameterValue(TEXT("LightsIntensity"), LightsIntensity);
    DynamicPlanetMaterial->SetScalarParameterValue(TEXT("NightFalloff"), NightFalloff);
    DynamicPlanetMaterial->SetVectorParameterValue(TEXT("AtmosColor"), AtmosphereColor);
    DynamicPlanetMaterial->SetScalarParameterValue(TEXT("AtmosIntensity"), AtmosphereIntensity);
}

void APlanetActor::DumpPlanetMaterialState(const FString& Context) const
{
    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] DumpPlanetMaterialState Context=%s Actor=%s Class=%s"),
        *Context,
        *GetName(),
        *GetClass()->GetName());

    if (!PlanetMesh)
    {
        return;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] MainMesh Mesh=%s Visible=%d Hidden=%d RelativeScale=%s WorldScale=%s Mat0=%s MID=%s"),
        *GetNameSafe(PlanetMesh->GetStaticMesh()),
        PlanetMesh->IsVisible() ? 1 : 0,
        PlanetMesh->bHiddenInGame ? 1 : 0,
        *PlanetMesh->GetRelativeScale3D().ToString(),
        *PlanetMesh->GetComponentScale().ToString(),
        *GetNameSafe(PlanetMesh->GetMaterial(0)),
        *GetNameSafe(DynamicPlanetMaterial));
}

void APlanetActor::DebugLogTextureState(const FString& Context) const
{
    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] TextureState Context=%s Actor=%s Base=%s Gloss=%s Lights=%s"),
        *Context,
        *GetName(),
        *GetNameSafe(BaseTexture),
        *GetNameSafe(GlossTexture),
        *GetNameSafe(LightsTexture));
}