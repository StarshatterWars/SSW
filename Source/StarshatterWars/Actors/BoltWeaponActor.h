#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BoltWeaponActor.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UStaticMeshComponent;
class UMaterialInterface;

/**
 * Presentation for graphic_type 3 projectiles, including energy bolts and railgun rounds.
 * Replaces the legacy Bolt sprite; graphic type does not determine damage behavior.
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

    /** Exact Wep.def identity; gameplay values continue to come from WeaponDesign. */
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Bolt")
    FString WeaponDefinitionName;

    /** Native presentation class for an exact Wep.def name, or the generic bolt fallback. */
    static TSubclassOf<ABoltWeaponActor> ClassForWeapon(const FString& WeaponName);
    static TSubclassOf<ABoltWeaponActor> BlueprintClassForWeapon(const FString& WeaponName);

    /** Activate a world-space visual whose movement and lifetime belong to SimShot. */
    void BeginSimulationFlight();

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
    virtual void OnConstruction(const FTransform& Transform) override;
    void ApplyPresentationDefaults();
    void ActivateVisuals();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    void Expire();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bolt")
    TObjectPtr<USceneComponent> Root;

    /** Small visible projectile core; never participates in damage collision. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bolt")
    TObjectPtr<UStaticMeshComponent> ProjectileMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bolt")
    TObjectPtr<UNiagaraComponent> CoreFX;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bolt")
    TObjectPtr<UNiagaraComponent> TrailFX;

    /** Per-weapon MI, applied to the inherited sphere in editor and game. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bolt")
    TObjectPtr<UMaterialInterface> BoltMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bolt")
    TObjectPtr<UNiagaraSystem> CoreSystem;

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
