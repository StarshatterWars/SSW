#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BeamWeaponActor.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/**
 * Replaces the legacy sprite/Bolt beam visual.
 * Subclass in Blueprint and assign a Niagara beam system
 * (e.g. one of the BP_ky_laser / BP_ky_beam assets) for a drop-in laser.
 *
 * The simulation drives it via SetBeamPoints / SetBeamActive.
 * Blueprint can also react through OnBeamUpdated.
 */
UCLASS(Blueprintable, BlueprintType)
class STARSHATTERWARS_API ABeamWeaponActor : public AActor
{
    GENERATED_BODY()

public:
    ABeamWeaponActor();

    /** Spawn a beam actor in the active game world. Returns null if no world. */
    UFUNCTION(BlueprintCallable, Category = "Beam", meta = (WorldContext = "WorldContextObject"))
    static ABeamWeaponActor* SpawnBeam(UObject* WorldContextObject, TSubclassOf<ABeamWeaponActor> BeamClass,
        FVector Start, FVector End, AActor* OwnerActor = nullptr);

    /** Update the beam endpoints. Called every frame by the weapon sim for continuous beams. */
    UFUNCTION(BlueprintCallable, Category = "Beam")
    void SetBeamPoints(FVector Start, FVector End);

    UFUNCTION(BlueprintCallable, Category = "Beam")
    void SetBeamActive(bool bActive);

    UFUNCTION(BlueprintCallable, Category = "Beam")
    void SetBeamColor(FLinearColor Color);

    UFUNCTION(BlueprintCallable, Category = "Beam")
    void SetBeamWidth(float Width);

    UFUNCTION(BlueprintPure, Category = "Beam")
    FVector GetBeamStart() const { return BeamStart; }

    UFUNCTION(BlueprintPure, Category = "Beam")
    FVector GetBeamEnd() const { return BeamEnd; }

    /** Fired whenever endpoints change. Override in Blueprint to drive a custom FX system. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Beam")
    void OnBeamUpdated(FVector Start, FVector End, float Length);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void ApplyVisual();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Beam")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Beam")
    TObjectPtr<UNiagaraComponent> BeamFX;

    /** Assign a Niagara beam system in the Blueprint child for the drop-in laser look. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beam")
    TObjectPtr<UNiagaraSystem> BeamSystem;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beam")
    FLinearColor BeamColor = FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beam", meta = (ClampMin = "0.1"))
    float BeamWidth = 8.0f;

    /** If > 0, actor auto-destroys after this many seconds. 0 = live until explicitly destroyed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beam", meta = (ClampMin = "0.0"))
    float LifeTime = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beam")
    bool bAutoDestroyOnDeactivate = true;

    UPROPERTY(BlueprintReadOnly, Category = "Beam")
    FVector BeamStart = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Beam")
    FVector BeamEnd = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Beam")
    bool bBeamActive = true;

    float Age = 0.0f;
};
