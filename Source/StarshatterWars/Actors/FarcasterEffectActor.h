#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "FarcasterEffectActor.generated.h"

class AShipActor;
class Ship;
class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;

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
    void FlashArrival();
    virtual void Tick(float DeltaSeconds) override;
    bool IsArrivalEffect() const { return bArrivalEffect; }
    void ClosePortal();
    void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    void ConfigureAppearance(bool bMarakan);
    void SetPortalScale(float Fraction);
    void SetArrivalAppearance(bool bArrival);

    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> PortalLayers;

    UPROPERTY() TArray<TObjectPtr<UNiagaraComponent>> VortexRings;
    UPROPERTY() TObjectPtr<UNiagaraSystem> DepartureSystem;
    UPROPERTY() TObjectPtr<UNiagaraSystem> ArrivalSystem;
    bool bArrivalEffect = false;
    float OpeningFraction = 0.f;
    float EffectAge = 0.f;
    TArray<FVector> AuthoredScales;
    TWeakObjectPtr<class ATravelVortexActor> ProceduralTunnel;
    FTimerHandle CloseTimer;
};
