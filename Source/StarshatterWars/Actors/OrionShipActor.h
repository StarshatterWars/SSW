#pragma once

#include "CoreMinimal.h"
#include "CarrierActor.h"
#include "OrionShipActor.generated.h"

class UCarrierInteriorBuilderComponent;

// Structural/model defaults from Orion.def. Simulation remains data-driven.
UCLASS(Blueprintable)
class STARSHATTERWARS_API AOrionShipActor : public ACarrierActor
{
    GENERATED_BODY()
public:
    AOrionShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Carrier|Interior")
    TObjectPtr<UCarrierInteriorBuilderComponent> InteriorBuilder;
protected:
    void ApplyOrionDefaults();
    void ApplyOrionFixedPoints();
};
