#pragma once

#include "CoreMinimal.h"
#include "PlanetActor.h"
#include "GasGiantActor.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API AGasGiantActor : public APlanetActor
{
    GENERATED_BODY()

public:
    AGasGiantActor();

    UFUNCTION(BlueprintCallable, Category = "Planet")
    virtual void SetPlanetVisualValues(
        float InPlanetRadius,
        float InAtmosphereHeight,
        float InAtmosphereSteps);

    virtual void SetPlanetRadius(float InPlanetRadius) override;

    UFUNCTION(BlueprintImplementableEvent, Category = "Planet")
    void ApplyPlanetVisuals();

    UFUNCTION(BlueprintPure, Category = "Planet")
    float GetPlanetRadius() const
    {
        return GasGiantRadius;
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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|GasGiant")
    float GasGiantRadius = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|GasGiant")
    float AtmosphereHeight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|GasGiant")
    float AtmosphereSteps = 8.0f;
};