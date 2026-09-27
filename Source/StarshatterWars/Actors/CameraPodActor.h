#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "CameraPodActor.generated.h"

UCLASS()
class STARSHATTERWARS_API ACameraPodActor : public AShipActor
{
    GENERATED_BODY()

public:
    ACameraPodActor();

    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    virtual void BeginPlay() override;

private:
    void ApplyCameraPodDefaults();
    void ApplyCameraPodVisibility();
};