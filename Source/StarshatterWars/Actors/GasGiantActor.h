#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GasGiantActor.generated.h"

class UStaticMeshComponent;

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
    // Core planet-like behavior
    // -----------------------------

    UFUNCTION(BlueprintCallable)
    void SetLightDirection(const FVector& InDirection);

    UFUNCTION(BlueprintCallable)
    void SetAxialRotationDegreesPerSecond(float DegreesPerSecond);

    UFUNCTION(BlueprintCallable)
    void SetAxialRotationEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "GasGiant")
    void SetPlanetRadius(float InRadiusUnits);

    UFUNCTION(BlueprintPure, Category = "GasGiant")
    float GetPlanetRadius() const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GasGiant")
    float PlanetRadiusUnits = 1.0f;

protected:
    // Root
    UPROPERTY(VisibleAnywhere)
    USceneComponent* SceneRoot;

    // Optional base mesh (can be hidden if BP has its own)
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* CoreMesh;

    // -----------------------------
    // Visual parameters (for BP)
    // -----------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GasGiant")
    FVector LightDirection = FVector(1, 0, 0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GasGiant")
    float AxialRotationDegreesPerSecond = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GasGiant")
    bool bEnableAxialRotation = false;
};