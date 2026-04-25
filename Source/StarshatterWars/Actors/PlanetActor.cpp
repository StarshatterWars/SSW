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
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture.h"

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
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));

    if (SphereMeshFinder.Succeeded())
    {
        DefaultSphereMesh = SphereMeshFinder.Object;
        PlanetMesh->SetStaticMesh(DefaultSphereMesh);
    }
}

void APlanetActor::BeginPlay()
{
    Super::BeginPlay();

    if (DefaultPlanetMaterial)
    {
        SetPlanetMaterial(DefaultPlanetMaterial);
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
    const float SafeRadius = FMath::Max(InRadiusUnits, 1.0f);

    /*
     * Engine sphere diameter is 100 units.
     * Scale = diameter / 100 = radius * 2 / 100.
     */
    const float Scale = (SafeRadius * 2.0f) / 100.0f;

    SetActorScale3D(FVector(Scale));
}

void APlanetActor::SetPlanetMaterial(UMaterialInterface* InMaterial)
{
    if (!PlanetMesh || !InMaterial)
    {
        return;
    }

    PlanetMesh->SetMaterial(0, InMaterial);
    DynamicPlanetMaterial = PlanetMesh->CreateAndSetMaterialInstanceDynamic(0);
}

void APlanetActor::SetPlanetTexture(UTexture* InTexture)
{
    if (!InTexture)
    {
        return;
    }

    EnsureDynamicMaterial();

    if (DynamicPlanetMaterial)
    {
        DynamicPlanetMaterial->SetTextureParameterValue(TEXT("PlanetTexture"), InTexture);
        DynamicPlanetMaterial->SetTextureParameterValue(TEXT("BaseTexture"), InTexture);
        DynamicPlanetMaterial->SetTextureParameterValue(TEXT("AlbedoTexture"), InTexture);
    }
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
    if (DynamicPlanetMaterial)
    {
        return;
    }

    if (!PlanetMesh)
    {
        return;
    }

    UMaterialInterface* CurrentMaterial = PlanetMesh->GetMaterial(0);

    if (!CurrentMaterial && DefaultPlanetMaterial)
    {
        CurrentMaterial = DefaultPlanetMaterial;
        PlanetMesh->SetMaterial(0, DefaultPlanetMaterial);
    }

    if (CurrentMaterial)
    {
        DynamicPlanetMaterial = PlanetMesh->CreateAndSetMaterialInstanceDynamic(0);
    }
}