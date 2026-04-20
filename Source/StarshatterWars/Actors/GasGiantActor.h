#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GasGiantActor.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API AGasGiantActor : public AActor
{
    GENERATED_BODY()

public:
    AGasGiantActor();

    UFUNCTION(BlueprintCallable, Category = "Planet")
    virtual void SetPlanetVisualValues(
        float InPlanetRadius,
        float InAtmosphereHeight,
        float InAtmosphereSteps);

    UFUNCTION(BlueprintCallable, Category = "Planet")
    virtual void SetPlanetRadius(float InPlanetRadius);

    UFUNCTION(BlueprintImplementableEvent, Category = "Planet")
    void ApplyPlanetVisuals();

    UFUNCTION(BlueprintPure, Category = "Planet")
    float GetPlanetRadius() const
    {
        return PlanetRadius;
    }

    UFUNCTION(BlueprintPure, Category = "Planet")
    float GetAtmosphereHeight() const
    {
        return AtmosphereHeight;
    }

    UFUNCTION(BlueprintPure, Category = "Planet")
    float GetAtmosphereSteps() const
    {
        return AtmosphereSteps;
    }

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
    float PlanetRadius = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
    float AtmosphereHeight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
    float AtmosphereSteps = 8.0f;
};