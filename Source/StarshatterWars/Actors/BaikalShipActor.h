#pragma once

#include "CoreMinimal.h"
#include "CapitalShipActor.h"
#include "BaikalShipActor.generated.h"

// Reparenting only: existing model and .def-driven defaults stay in the .cpp.
UCLASS(Blueprintable)
class STARSHATTERWARS_API ABaikalShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    ABaikalShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyBaikalDefaults();
    void ApplyBaikalFixedPoints();
    void BuildBaikalWeaponMountPoints();
    void BuildBaikalTurretBasePoints();
    void BuildBaikalDockPoints();
    void BuildBaikalLandingPoints();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
