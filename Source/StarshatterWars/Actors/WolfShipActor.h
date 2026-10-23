#pragma once

#include "CoreMinimal.h"
#include "CapitalShipActor.h"
#include "WolfShipActor.generated.h"

// Reparenting only: existing model and .def-driven defaults stay in the .cpp.
UCLASS(Blueprintable)
class STARSHATTERWARS_API AWolfShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    AWolfShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyWolfDefaults();
    void ApplyWolfFixedPoints();
};
