#pragma once

#include "FreightXferShipActor.h"
#include "AsteroidBaseActor.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable)
class STARSHATTERWARS_API AAsteroidBaseActor : public AFreightXferShipActor
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Flight Deck")
    bool bBayDoorsOpen = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Flight Deck")
    float BayDoorTravelSeconds = 1.0f;

    // Travel from the closed pose to the open pose authored in the Blueprint.
    UPROPERTY(EditDefaultsOnly, Category="Flight Deck")
    float UpperDoorTravel = 900.0f;

    UPROPERTY(EditDefaultsOnly, Category="Flight Deck")
    float LowerDoorTravel = 650.0f;

private:
    void UpdateDoorPositions();

    TWeakObjectPtr<UStaticMeshComponent> UpperDoor;
    TWeakObjectPtr<UStaticMeshComponent> LowerDoor;
    FVector UpperOpenLocation = FVector::ZeroVector;
    FVector LowerOpenLocation = FVector::ZeroVector;
    float DoorOpening = 0.0f;
};
