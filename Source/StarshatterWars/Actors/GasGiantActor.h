#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"
#include "GasGiantActor.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class STARSHATTERWARS_API AGasGiantActor : public AActor
{
    GENERATED_BODY()

public:
    AGasGiantActor();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // -----------------------------
    // Core
    // -----------------------------
    UFUNCTION(BlueprintCallable, Category = "GasGiant")
    void SetPlanetRadius(float InRadiusUnits);

    UFUNCTION(BlueprintPure, Category = "GasGiant")
    float GetPlanetRadius() const;

    UFUNCTION(BlueprintCallable, Category = "GasGiant")
    void SetLightDirection(const FVector& InDirection);

    UFUNCTION(BlueprintCallable, Category = "GasGiant")
    void SetGasGiantMaterialByName(const FString& MaterialName);

    UFUNCTION(BlueprintImplementableEvent, Category = "GasGiant|Material")
    void OnGasGiantMaterialChanged();

    UFUNCTION(BlueprintImplementableEvent, Category = "GasGiant")
    void OnGasGiantRadiusChanged();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GasGiant")
    FVector InitialActorScale = FVector::OneVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GasGiant")
    float GasGiantBaseMeshRadius = 50.0f;

    // -----------------------------
    // Visual controls (optional)
    // -----------------------------
    UFUNCTION(BlueprintCallable, Category = "GasGiant")
    void SetBandSpeed(float InSpeed);

    UFUNCTION(BlueprintCallable, Category = "GasGiant")
    void SetStormIntensity(float InIntensity);

    void UpdateRingParameters(float PlanetScale);
    void ApplyRingSettingsToBlueprint();
    void ApplyRingRadiusSettings();

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GasGiant|Rings")
    float InnerRingRadius = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GasGiant|Rings")
    float OuterRingRadius = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GasGiant|Rings")
    float RingPosition = 0.0f;

protected:

    // Root
    UPROPERTY(VisibleAnywhere)
    USceneComponent* SceneRoot;

    // Core sphere (gas giant body)
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* CoreMesh;

    // Dynamic material
    UPROPERTY()
    UMaterialInstanceDynamic* DynamicMaterial;

    // -----------------------------
    // Runtime data
    // -----------------------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GasGiant")
    float PlanetRadiusUnits = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GasGiant")
    FVector LightDirection = FVector(1, 0, 0);

    // -----------------------------
    // Config
    // -----------------------------
    UPROPERTY(EditAnywhere, Category = "GasGiant|Material")
    FString MaterialBasePath = TEXT("/Game/GameData/Galaxy/GasGiants/");

    UPROPERTY(EditAnywhere, Category = "GasGiant|Material")
    FString MaterialPrefix = TEXT("MI_");

    // -----------------------------
    // Internal
    // -----------------------------
    void CreateMID();
};