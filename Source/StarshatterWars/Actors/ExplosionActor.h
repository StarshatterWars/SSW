#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameStructs_System.h"
#include "ExplosionActor.generated.h"
class UNiagaraComponent; class UNiagaraSystem; class UParticleSystemComponent; class UParticleSystem;
/** Cosmetic legacy explosion presentation. Never applies damage or collision. */
UCLASS(Blueprintable, BlueprintType)
class STARSHATTERWARS_API AExplosionActor : public AActor
{
 GENERATED_BODY()
public:
 AExplosionActor();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Explosion") TObjectPtr<USceneComponent> EffectRoot;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Explosion") TObjectPtr<UNiagaraComponent> NiagaraEffect;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Explosion") TObjectPtr<UParticleSystemComponent> ParticleEffect;
 UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Explosion") EExplosionType ExplosionType=EExplosionType::SMALL_EXPLOSION;
 UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Explosion") TObjectPtr<UNiagaraSystem> NiagaraSystem;
 UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Explosion") TObjectPtr<UParticleSystem> ParticleSystem;
 UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Explosion",meta=(ClampMin="0.001")) float VisualScale=1;
 UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Explosion",meta=(ClampMin="0.01")) float FallbackLifetime=3;
 UFUNCTION(BlueprintCallable, Category="Explosion") void PlayExplosion(float ExplosionScale=1,float ParticleScale=1,float SimulationLifetime=0);
 void AdvancePresentation(double Seconds);
 void SetPresentationClock(bool bPaused,float Dilation);
 void FinishSustained(float Seconds);
 void SyncSimulationLifetime(float Seconds);
 bool IsPersistentPresentation() const{return bPersistent;}
 static TSubclassOf<AExplosionActor> NativeClass(EExplosionType Type);
 static TSubclassOf<AExplosionActor> VisualClass(EExplosionType Type);
protected:
 virtual void OnConstruction(const FTransform& Transform) override;
private:
 double Remaining=0;bool bPersistent=false;
};
