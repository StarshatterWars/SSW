#pragma once
#include "ExplosionActor.h"
#include "MuzzleFlashExplosionActor.generated.h"
class UMaterialBillboardComponent;class UMaterialInterface;class UMaterialInstanceDynamic;
/** Brief weapon muzzle blast, distinct from a damage impact. */
UCLASS(Blueprintable, BlueprintType)
class STARSHATTERWARS_API AMuzzleFlashExplosionActor : public AExplosionActor
{
 GENERATED_BODY()
public:
 AMuzzleFlashExplosionActor();
 static TSubclassOf<AExplosionActor> MuzzleClass();
 void SetFlashAppearance(const FString& FlashId,float Scale);
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Muzzle") TObjectPtr<UMaterialBillboardComponent> FlashGlow;
 UPROPERTY() TObjectPtr<UMaterialInterface> GlowMaterial;
 UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> GlowMID;

};
