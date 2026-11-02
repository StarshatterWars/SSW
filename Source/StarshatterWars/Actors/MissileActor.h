#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MissileActor.generated.h"

class UNiagaraComponent;

/** Missile visual shared by carried stores and independent simulation shots. */
UCLASS(Blueprintable, BlueprintType)
class STARSHATTERWARS_API AMissileActor : public AActor
{
    GENERATED_BODY()
public:
    AMissileActor();

    /** Release a world-spawned flight actor; SimShot owns movement and lifetime. */
    UFUNCTION(BlueprintCallable, Category="Missile")
    void BeginSimulationFlight();

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Missile")
    bool bInFlight = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Missile")
    TObjectPtr<USceneComponent> MissileRoot;

    /** Small engine exhaust, inherited by each missile Blueprint. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Missile|Trail")
    TObjectPtr<UNiagaraComponent> TrailComponent;

    /** Disable to position the trail manually in the Blueprint. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Missile|Trail")
    bool bAutoPositionTrail = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Missile|Trail")
    bool bEnableTrail = true;

    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void PositionTrail();
};
