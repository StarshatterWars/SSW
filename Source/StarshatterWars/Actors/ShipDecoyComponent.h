#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShipDecoyComponent.generated.h"

// Blueprint bridge only: the legacy Ship/Weapon/Drone simulation owns deployment.
UCLASS(ClassGroup=(Ship), meta=(BlueprintSpawnableComponent))
class STARSHATTERWARS_API UShipDecoyComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UShipDecoyComponent();
    UFUNCTION(BlueprintCallable, Category="Ship|Decoy") bool Deploy();
    UFUNCTION(BlueprintPure, Category="Ship|Decoy") bool IsAvailable() const;
    UFUNCTION(BlueprintPure, Category="Ship|Decoy") int32 GetAmmo() const;
    UFUNCTION(BlueprintPure, Category="Ship|Decoy") int32 GetActiveCount() const;
};
