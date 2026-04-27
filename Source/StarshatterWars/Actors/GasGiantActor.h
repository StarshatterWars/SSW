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

    UPROPERTY(Transient, BlueprintReadOnly, Category = "GasGiant|Rings")
    UMaterialInstance* CurrentRingMaterial = nullptr;

    UFUNCTION(BlueprintImplementableEvent, Category = "GasGiant|Rings")
    void OnRingMaterialChanged(UMaterialInstance* NewRingMaterial);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GasGiant")
    FVector InitialActorScale = FVector::OneVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GasGiant")
    float GasGiantBaseMeshRadius = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GasGiant|Rings")
    FString RingMaterialName;

    // -----------------------------
    // Visual controls (optional)
    // -----------------------------
    UFUNCTION(BlueprintCallable, Category = "GasGiant")
    void SetBandSpeed(float InSpeed);

    UFUNCTION(BlueprintCallable, Category = "GasGiant")
    void SetStormIntensity(float InIntensity);

    UFUNCTION(BlueprintCallable, Category = "GasGiant|Rings")
    void SetRingMaterialByName(const FString& InRingName);

    void UpdateRingParameters(float PlanetScale);
    void ApplyRingSettingsToBlueprint();
    void ApplyRingRadiusSettings();

    UMaterialInterface* LoadRingMaterialByName(const FString& RingName);
    void DrawDebugRingOutline(float InnerRadius, float OuterRadius) const;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GasGiant|Rings")
    float InnerRingRadius = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GasGiant|Rings")
    float OuterRingRadius = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GasGiant|Rings")
    float RingPosition = 0.0f;

    UPROPERTY()
    UMaterialInstanceDynamic* RingDynamicMaterial = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GasGiant|Rings")
    TObjectPtr<UMaterialInterface> RingBaseMaterial = nullptr;


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

    UPROPERTY(EditAnywhere, Category = "GasGiant|Rings")
    FString RingMaterialBasePath = TEXT("/Game/GameData/Galaxy/GasGiants/");

    UPROPERTY(EditAnywhere, Category = "GasGiant|Rings")
    FString RingMaterialPrefix = TEXT("MI_");

    // -----------------------------
    // Internal
    // -----------------------------
    void CreateMID();

    // -----------------------------
    // Debug
    // -----------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bDebugDrawRings = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
    float DebugInnerRadius = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
    float DebugOuterRadius = 0.0f;
};