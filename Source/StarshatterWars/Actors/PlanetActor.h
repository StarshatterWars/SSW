/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         PlanetActor.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Visual planet actor for runtime star system scenes.
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlanetActor.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTexture;

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
    void SetBaseTexture(UTexture* InTexture);

    UFUNCTION(BlueprintCallable, Category = "Planet")
    void SetGlossTexture(UTexture* InTexture);

    UFUNCTION(BlueprintCallable, Category = "Planet")
    void SetLightsTexture(UTexture* InTexture);

    UFUNCTION(BlueprintCallable, Category = "Planet")
    void SetPlanetTextures(
        UTexture* InBaseTexture,
        UTexture* InGlossTexture,
        UTexture* InLightsTexture);

    UFUNCTION(BlueprintCallable, Category = "Planet")
    void SetLightDirection(const FVector& InDirection);

    UFUNCTION(BlueprintCallable, Category = "Planet")
    void SetLightsIntensity(float InIntensity);

    UFUNCTION(BlueprintCallable, Category = "Planet")
    void SetNightFalloff(float InFalloff);

    UFUNCTION(BlueprintCallable, Category = "Planet")
    void SetAtmosphereColor(const FLinearColor& InColor);

    UFUNCTION(BlueprintCallable, Category = "Planet")
    void SetAtmosphereIntensity(float InIntensity);

    UFUNCTION(BlueprintCallable, Category = "Planet")
    void SetAxialRotationDegreesPerSecond(float DegreesPerSecond);

    UFUNCTION(BlueprintCallable, Category = "Planet")
    void SetAxialRotationEnabled(bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "Planet")
    UStaticMeshComponent* GetPlanetMeshComponent() const { return PlanetMesh; }

    UFUNCTION(BlueprintCallable, Category = "Planet|Debug")
    void DumpPlanetMaterialState(const FString& Context) const;

protected:
    void EnsureDynamicMaterial();
    void ApplyMaterialParameters();
    void DebugLogTextureState(const FString& Context) const;


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

    UPROPERTY(Transient)
    TObjectPtr<UTexture> BaseTexture = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTexture> GlossTexture = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTexture> LightsTexture = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Material")
    FVector LightDirection = FVector(0.0f, 0.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Material")
    float LightsIntensity = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Material")
    float NightFalloff = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Material")
    FLinearColor AtmosphereColor = FLinearColor(0.25f, 0.55f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Material")
    float AtmosphereIntensity = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Rotation")
    bool bEnableAxialRotation = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Rotation")
    float AxialRotationDegreesPerSecond = 0.0f;
};