#pragma once
#include "ExplosionActor.h"
#include "MuzzleFlashExplosionActor.generated.h"
/** Brief weapon muzzle blast, distinct from a damage impact. */
UCLASS(Blueprintable, BlueprintType)
class STARSHATTERWARS_API AMuzzleFlashExplosionActor : public AExplosionActor
{
 GENERATED_BODY()
public:
 AMuzzleFlashExplosionActor();
 static TSubclassOf<AExplosionActor> MuzzleClass();
};
