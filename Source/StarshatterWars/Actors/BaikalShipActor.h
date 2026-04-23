#pragma once
#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "BaikalShipActor.generated.h"

UCLASS()
class STARSHATTERWARS_API ABaikalShipActor : public AShipActor
{
    GENERATED_BODY()

public:
    ABaikalShipActor();

    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    void ApplyBaikalDefaults();
    void ApplyBaikalFixedPoints();
    void BuildBaikalMainEnginePoints();
    void BuildBaikalThrusterPoints();
    void BuildBaikalWeaponMountPoints();
    void BuildBaikalTurretBasePoints();
    void BuildBaikalDockPoints();
    void BuildBaikalLandingPoints();
    void BuildBaikalNavLights();
};