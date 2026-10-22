#pragma once
#include "CoreMinimal.h"
#include "GasGiantActor.h"
#include "TrellisPlanetActor.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API ATrellisPlanetActor : public AGasGiantActor
{
    GENERATED_BODY()
public:
    ATrellisPlanetActor();
    virtual void OnConstruction(const FTransform& Transform) override;
    UFUNCTION(BlueprintCallable, Category="Trellis|Rings")
    void RefreshTrellisRings();
    UFUNCTION(BlueprintPure, Category="Trellis|Rings")
    bool HasSpawnedRings() const;
protected:
    virtual void BeginPlay() override;
};
