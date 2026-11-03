#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShipProbeComponent.generated.h"

// Blueprint bridge only: the legacy Ship/Weapon/Drone simulation owns deployment.
UCLASS(ClassGroup=(Ship), meta=(BlueprintSpawnableComponent))
class STARSHATTERWARS_API UShipProbeComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UShipProbeComponent();
    UFUNCTION(BlueprintCallable, Category="Ship|Probe") bool Deploy();
    UFUNCTION(BlueprintPure, Category="Ship|Probe") bool IsAvailable() const;
    UFUNCTION(BlueprintPure, Category="Ship|Probe") int32 GetAmmo() const;
    UFUNCTION(BlueprintPure, Category="Ship|Probe") int32 GetActiveCount() const;
};
