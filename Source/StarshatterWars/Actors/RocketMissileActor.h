#pragma once
#include "MissileActor.h"
#include "RocketMissileActor.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable, BlueprintType)
class STARSHATTERWARS_API ARocketMissileActor : public AMissileActor
{
    GENERATED_BODY()
public:
    ARocketMissileActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Missile|Mesh")
    TObjectPtr<UStaticMeshComponent> MeshComponent;
};
