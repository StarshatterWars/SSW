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
    PlanetMesh->SetCastShadow(true);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(
        TEXT("/Script/Engine.StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"));

    if (SphereMeshFinder.Succeeded())
    {
        DefaultSphereMesh = SphereMeshFinder.Object;
        PlanetMesh->SetStaticMesh(DefaultSphereMesh);

        UE_LOG(LogTemp, Warning,
            TEXT("[PlanetActor] Constructor: Sphere mesh assigned: %s"),
            *DefaultSphereMesh->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("[PlanetActor] Constructor: FAILED to load sphere mesh"));
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlanetMaterialFinder(
        TEXT("/Script/Engine.Material'/Game/GameData/Galaxy/PlanetMaterials/M_Planet.M_Planet'"));

    if (PlanetMaterialFinder.Succeeded())
    {
        DefaultPlanetMaterial = PlanetMaterialFinder.Object;
        PlanetMesh->SetMaterial(0, DefaultPlanetMaterial);

        UE_LOG(LogTemp, Warning,
            TEXT("[PlanetActor] Constructor: Default planet material assigned: %s"),
            *DefaultPlanetMaterial->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("[PlanetActor] Constructor: FAILED to load M_Planet"));
    }
}

void APlanetActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bEnableAxialRotation)
    {
        return;
    }

    if (FMath::IsNearlyZero(AxialRotationDegreesPerSecond))
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
        UE_LOG(LogTemp, Warning,
            TEXT("[PlanetActor] SetPlanetRadius: PlanetMesh is null"));
        return;
    }

    const float ScaleMultiplier = 100.0f; 

    const float Scale = InRadiusUnits * ScaleMultiplier;

    PlanetMesh->SetWorldScale3D(FVector(Scale));

    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] SetPlanetRadius: Actor=%s RadiusUnits=%.2f FinalScale=%.2f"),
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

    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] Created MID: %s"),
        *GetNameSafe(DynamicPlanetMaterial));

    ApplyMaterialParameters();
}

void APlanetActor::SetBaseTexture(UTexture* InTexture)
{
    BaseTexture = InTexture;

    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] SetBaseTexture: Actor=%s Base=%s"),
        *GetName(),
        BaseTexture ? *BaseTexture->GetName() : TEXT("NULL"));

    ApplyMaterialParameters();
}

void APlanetActor::SetGlossTexture(UTexture* InTexture)
{
    GlossTexture = InTexture;

    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] SetGlossTexture: Actor=%s Gloss=%s"),
        *GetName(),
        GlossTexture ? *GlossTexture->GetName() : TEXT("NULL"));

    ApplyMaterialParameters();
}

void APlanetActor::SetLightsTexture(UTexture* InTexture)
{
    LightsTexture = InTexture;

    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] SetLightsTexture: Actor=%s Lights=%s"),
        *GetName(),
        LightsTexture ? *LightsTexture->GetName() : TEXT("NULL"));

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
        TEXT("[PlanetActor] SetPlanetTextures CALLED: Actor=%s Base=%s Gloss=%s Lights=%s"),
        *GetName(),
        BaseTexture ? *BaseTexture->GetName() : TEXT("NULL"),
        GlossTexture ? *GlossTexture->GetName() : TEXT("NULL"),
        LightsTexture ? *LightsTexture->GetName() : TEXT("NULL"));

    ApplyMaterialParameters();
    DumpPlanetMaterialState(TEXT("After SetPlanetTextures"));
}

void APlanetActor::SetLightDirection(const FVector& InDirection)
{
    FVector SafeDirection = InDirection;

    if (SafeDirection.IsNearlyZero())
    {
        SafeDirection = FVector(0.0f, 0.0f, 1.0f);
    }

    LightDirection = SafeDirection.GetSafeNormal();

    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] SetLightDirection: Actor=%s Direction=%s"),
        *GetName(),
        *LightDirection.ToString());

    ApplyMaterialParameters();
}

void APlanetActor::SetLightsIntensity(float InIntensity)
{
    LightsIntensity = FMath::Max(0.0f, InIntensity);

    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] SetLightsIntensity: Actor=%s Intensity=%.3f"),
        *GetName(),
        LightsIntensity);

    ApplyMaterialParameters();
}

void APlanetActor::SetNightFalloff(float InFalloff)
{
    NightFalloff = FMath::Max(0.01f, InFalloff);

    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] SetNightFalloff: Actor=%s Falloff=%.3f"),
        *GetName(),
        NightFalloff);

    ApplyMaterialParameters();
}

void APlanetActor::SetAtmosphereColor(const FLinearColor& InColor)
{
    AtmosphereColor = InColor;

    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] SetAtmosphereColor: Actor=%s Color=%s"),
        *GetName(),
        *AtmosphereColor.ToString());

    ApplyMaterialParameters();
}

void APlanetActor::SetAtmosphereIntensity(float InIntensity)
{
    AtmosphereIntensity = FMath::Max(0.0f, InIntensity);

    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] SetAtmosphereIntensity: Actor=%s Intensity=%.3f"),
        *GetName(),
        AtmosphereIntensity);

    ApplyMaterialParameters();
}

void APlanetActor::SetAxialRotationDegreesPerSecond(float DegreesPerSecond)
{
    AxialRotationDegreesPerSecond = DegreesPerSecond;
    bEnableAxialRotation = !FMath::IsNearlyZero(AxialRotationDegreesPerSecond);

    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] SetAxialRotationDegreesPerSecond: Actor=%s DegreesPerSecond=%.3f Enabled=%d"),
        *GetName(),
        AxialRotationDegreesPerSecond,
        bEnableAxialRotation ? 1 : 0);
}

void APlanetActor::SetAxialRotationEnabled(bool bEnabled)
{
    bEnableAxialRotation = bEnabled;

    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] SetAxialRotationEnabled: Actor=%s Enabled=%d"),
        *GetName(),
        bEnableAxialRotation ? 1 : 0);
}

void APlanetActor::EnsureDynamicMaterial()
{
    if (!PlanetMesh)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[PlanetActor] EnsureDynamicMaterial FAILED: PlanetMesh is NULL on %s"),
            *GetName());
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

        UE_LOG(LogTemp, Warning,
            TEXT("[PlanetActor] EnsureDynamicMaterial: Actor=%s Created MID=%s From=%s"),
            *GetName(),
            DynamicPlanetMaterial ? *DynamicPlanetMaterial->GetName() : TEXT("NULL"),
            *CurrentMaterial->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("[PlanetActor] EnsureDynamicMaterial FAILED: No material available on %s"),
            *GetName());
    }
}

void APlanetActor::ApplyMaterialParameters()
{
    EnsureDynamicMaterial();

    if (!DynamicPlanetMaterial)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[PlanetActor] ApplyMaterialParameters FAILED: DynamicPlanetMaterial is NULL on %s"),
            *GetName());
        return;
    }

    if (PlanetMesh)
    {
        PlanetMesh->SetMaterial(0, DynamicPlanetMaterial);
    }

    DebugLogTextureState(TEXT("ApplyMaterialParameters"));

    if (BaseTexture)
    {
        DynamicPlanetMaterial->SetTextureParameterValue(TEXT("BaseTexture"), BaseTexture);
        DynamicPlanetMaterial->SetTextureParameterValue(TEXT("PlanetTexture"), BaseTexture);
        DynamicPlanetMaterial->SetTextureParameterValue(TEXT("AlbedoTexture"), BaseTexture);

        UTexture* VerifyTexture = nullptr;

        const bool bGotTexture = DynamicPlanetMaterial->GetTextureParameterValue(
            TEXT("BaseTexture"),
            VerifyTexture);

        UE_LOG(LogTemp, Error,
            TEXT("[PlanetActor] VERIFY BaseTexture Param Got=%d Input=%s ReadBack=%s"),
            bGotTexture ? 1 : 0,
            *GetNameSafe(BaseTexture),
            *GetNameSafe(VerifyTexture));
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("[PlanetActor] ApplyMaterialParameters: BaseTexture is NULL on %s"),
            *GetName());
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
        FLinearColor(
            LightDirection.X,
            LightDirection.Y,
            LightDirection.Z,
            0.0f));

    DynamicPlanetMaterial->SetScalarParameterValue(
        TEXT("LightsIntensity"),
        LightsIntensity);

    DynamicPlanetMaterial->SetScalarParameterValue(
        TEXT("NightFalloff"),
        NightFalloff);

    DynamicPlanetMaterial->SetVectorParameterValue(
        TEXT("AtmosColor"),
        AtmosphereColor);

    DynamicPlanetMaterial->SetScalarParameterValue(
        TEXT("AtmosIntensity"),
        AtmosphereIntensity);

    FLinearColor VerifyLightDir;
    const bool bGotLightDir = DynamicPlanetMaterial->GetVectorParameterValue(
        TEXT("LightDirection"),
        VerifyLightDir);

    UE_LOG(LogTemp, Error,
        TEXT("[PlanetActor] VERIFY LightDirection Got=%d Stored=%s Param=%s"),
        bGotLightDir ? 1 : 0,
        *LightDirection.ToString(),
        *VerifyLightDir.ToString());
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

        if (MeshComp != PlanetMesh)
        {
            MeshComp->SetVisibility(false, true);
            MeshComp->SetHiddenInGame(true, true);
            MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            MeshComp->SetGenerateOverlapEvents(false);

            UE_LOG(LogTemp, Warning,
                TEXT("[PlanetActor] Hiding BP mesh '%s'"),
                *MeshComp->GetName());
        }

        if (PlanetMesh)
        {
            PlanetMesh->SetVisibility(true, true);
            PlanetMesh->SetHiddenInGame(false, true);
            PlanetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            PlanetMesh->SetCastShadow(false);

            // Stop distance/frustum culling from hiding the tiny planet mesh.
            PlanetMesh->SetCullDistance(0.0f);
            PlanetMesh->SetBoundsScale(10000.0f);

            UE_LOG(LogTemp, Error,
                TEXT("[PlanetActor] FORCE VISIBLE Actor=%s Loc=%s Scale=%s BoundsScale=10000"),
                *GetName(),
                *GetActorLocation().ToString(),
                *GetActorScale3D().ToString());
        }
    }

    DrawDebugSphere(
        GetWorld(),
        GetActorLocation(),
        100000.0f, // BIG
        32,
        FColor::Green,
        true,
        10.0f,
        0,
        100.0f);

    EnsureDynamicMaterial();
    ApplyMaterialParameters();

    DumpPlanetMaterialState(TEXT("BeginPlay"));
}

void APlanetActor::DumpPlanetMaterialState(const FString& Context) const
{
    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] DumpPlanetMaterialState: Context=%s Actor=%s Class=%s"),
        *Context,
        *GetName(),
        *GetClass()->GetName());

    if (!PlanetMesh)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[PlanetActor] DumpPlanetMaterialState: PlanetMesh is NULL"));
        return;
    }

    UStaticMesh* Mesh = PlanetMesh->GetStaticMesh();
    UMaterialInterface* SlotMaterial = PlanetMesh->GetMaterial(0);

    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] MainPlanetMesh: Name=%s Mesh=%s Visible=%d HiddenInGame=%d Slot0=%s DefaultMat=%s MID=%s"),
        *PlanetMesh->GetName(),
        Mesh ? *Mesh->GetName() : TEXT("NULL"),
        PlanetMesh->IsVisible() ? 1 : 0,
        PlanetMesh->bHiddenInGame ? 1 : 0,
        SlotMaterial ? *SlotMaterial->GetName() : TEXT("NULL"),
        DefaultPlanetMaterial ? *DefaultPlanetMaterial->GetName() : TEXT("NULL"),
        DynamicPlanetMaterial ? *DynamicPlanetMaterial->GetName() : TEXT("NULL"));

    TArray<UStaticMeshComponent*> Meshes;
    GetComponents<UStaticMeshComponent>(Meshes);

    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] StaticMeshComponent count=%d"),
        Meshes.Num());

    for (int32 Index = 0; Index < Meshes.Num(); ++Index)
    {
        UStaticMeshComponent* MeshComp = Meshes[Index];

        if (!MeshComp)
        {
            continue;
        }

        UStaticMesh* CompMesh = MeshComp->GetStaticMesh();
        UMaterialInterface* CompMaterial = MeshComp->GetMaterial(0);

        UE_LOG(LogTemp, Warning,
            TEXT("[PlanetActor] MeshComp[%d]: Name=%s IsPlanetMesh=%d Visible=%d HiddenInGame=%d Mesh=%s Mat0=%s"),
            Index,
            *MeshComp->GetName(),
            MeshComp == PlanetMesh ? 1 : 0,
            MeshComp->IsVisible() ? 1 : 0,
            MeshComp->bHiddenInGame ? 1 : 0,
            CompMesh ? *CompMesh->GetName() : TEXT("NULL"),
            CompMaterial ? *CompMaterial->GetName() : TEXT("NULL"));
    }
}
void APlanetActor::DebugLogTextureState(const FString& Context) const
{
    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetActor] TextureState: Context=%s Actor=%s Base=%s Gloss=%s Lights=%s"),
        *Context,
        *GetName(),
        BaseTexture ? *BaseTexture->GetName() : TEXT("NULL"),
        GlossTexture ? *GlossTexture->GetName() : TEXT("NULL"),
        LightsTexture ? *LightsTexture->GetName() : TEXT("NULL"));
}
