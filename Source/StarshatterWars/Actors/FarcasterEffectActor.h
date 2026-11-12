#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "FarcasterEffectActor.generated.h"

class AShipActor;
class Ship;
class UStaticMeshComponent;

/** Owns and drives the portal layers from the legacy Farcaster sequence. */
UCLASS()
class STARSHATTERWARS_API AFarcasterEffectActor : public AActor
{
    GENERATED_BODY()

public:
    AFarcasterEffectActor();

    static void InstallOn(AShipActor& Gate, bool bMarakan);
    static AFarcasterEffectActor* FindFor(const Ship* Gate);

    void SetOpening(float Progress);
    void FlashDeparture();
    void ClosePortal();

private:
    void ConfigureAppearance(bool bMarakan);
    void SetPortalScale(float Fraction);

    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> PortalLayers;

    TArray<FVector> AuthoredScales;
    FTimerHandle CloseTimer;
};
