/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         PlanetActor.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Visual planet actor for runtime star system scenes.

    This actor owns only the planet visual mesh and material behavior.
    Region anchors, ships, stations, and mission actors should not attach
    to the spinning mesh. They should attach to the stable system/region
    actor hierarchy instead.
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlanetActor.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS()
class STARSHATTERWARS_API APlanetActor : public AActor
{
    GENERATED_BODY()

public:
    APlanetActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

public:
    UFUNCTION(BlueprintCallable, Category = "Planet")
    void SetPlanetRadius(float InRadiusUnits);

    UFUNCTION(BlueprintCallable, Category = "Planet")
    void SetPlanetMaterial(UMaterialInterface* InMaterial);

    UFUNCTION(BlueprintCallable, Category = "Planet")
    void SetPlanetTexture(UTexture* InTexture);

    UFUNCTION(BlueprintCallable, Category = "Planet")
    void SetAxialRotationDegreesPerSecond(float DegreesPerSecond);

    UFUNCTION(BlueprintCallable, Category = "Planet")
    void SetAxialRotationEnabled(bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "Planet")
    UStaticMeshComponent* GetPlanetMeshComponent() const { return PlanetMesh; }

protected:
    void EnsureDynamicMaterial();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
    TObjectPtr<USceneComponent> SceneRoot = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
    TObjectPtr<UStaticMeshComponent> PlanetMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet|Mesh")
    TObjectPtr<UStaticMesh> DefaultSphereMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet|Material")
    TObjectPtr<UMaterialInterface> DefaultPlanetMaterial = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> DynamicPlanetMaterial = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Rotation")
    bool bEnableAxialRotation = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Rotation")
    float AxialRotationDegreesPerSecond = 0.0f;
};