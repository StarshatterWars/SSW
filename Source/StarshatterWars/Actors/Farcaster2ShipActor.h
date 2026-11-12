#pragma once
#include "ShipActor.h"
#include "Farcaster2ShipActor.generated.h"

/** Presentation class for Farcaster2.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API AFarcaster2ShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    AFarcaster2ShipActor();
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Farcaster") TObjectPtr<UStaticMeshComponent> CenterMesh;
};
