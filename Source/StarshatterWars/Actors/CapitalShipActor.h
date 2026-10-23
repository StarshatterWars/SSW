#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "CapitalShipActor.generated.h"

/** Shared capital-ship presentation features. The runtime Ship remains authoritative. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API ACapitalShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    ACapitalShipActor();

    /** Capital-specific component group; no duplicate runtime combat systems. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Capital Ship|Components")
    TObjectPtr<USceneComponent> CapitalSubsystemRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Capital Ship|Components")
    TObjectPtr<USceneComponent> CapitalDefenseRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Capital Ship|Components")
    TObjectPtr<USceneComponent> CapitalCommandRoot;
};
