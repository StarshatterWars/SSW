#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BoltWeaponActor.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USphereComponent;

/**
 * Bolt-torpedo style energy projectile.
 * Replaces the legacy Bolt sprite for non-beam energy weapons.
 * Travels from spawn point along a direction for a fixed lifetime,
 * then destroys itself (impact handling is done by the simulation).
 *
 * Subclass in Blueprint and assign a Niagara system for the visual.
 */
UCLASS(Blueprintable, BlueprintType)
class STARSHATTERWARS_API ABoltWeaponActor : public AActor
{
    GENERATED_BODY()

public:
    ABoltWeaponActor();

    /** Spawn a bolt projectile. Speed is cm/s. Life is seconds. */
    UFUNCTION(BlueprintCallable, Category = "Bolt", meta = (WorldContext = "WorldContextObject"))
    static ABoltWeaponActor* SpawnBolt(UObject* WorldContextObject,
        TSubclassOf<ABoltWeaponActor> BoltClass,
        FVector Origin, FVector Direction, float Speed, float Life,
        AActor* OwnerActor = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Bolt")
    void Launch(FVector Origin, FVector Direction, float Speed, float Life);

    FVector GetVelocity() const { return MoveDirection * MoveSpeed; }

    /** Fired on impact or lifetime expiry. Override in Blueprint for explosion FX. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Bolt")
    void OnBoltExpired(FVector Location);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    void Expire();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bolt")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bolt")
    TObjectPtr<UNiagaraComponent> TrailFX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bolt")
    TObjectPtr<UNiagaraSystem> BoltSystem;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bolt")
    FLinearColor BoltColor = FLinearColor(0.3f, 0.7f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bolt", meta = (ClampMin = "1.0"))
    float BoltScale = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Bolt")
    FVector MoveDirection = FVector::ForwardVector;

    UPROPERTY(BlueprintReadOnly, Category = "Bolt")
    float MoveSpeed = 10000.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Bolt")
    float LifeRemaining = 2.0f;

    bool bLaunched = false;
};
