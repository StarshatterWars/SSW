#pragma once

#include "QuantumFlashExplosionActor.h"
#include "QuantumEffectsActor.generated.h"

class UMaterialBillboardComponent;

/** Native presentation of the legacy quantum flash at departure and arrival. */
UCLASS()
class STARSHATTERWARS_API AQuantumEffectsActor : public AQuantumFlashExplosionActor
{
    GENERATED_BODY()

public:
    AQuantumEffectsActor();

private:
    UPROPERTY(VisibleAnywhere, Category="Quantum")
    TObjectPtr<UMaterialBillboardComponent> Flash;
};
