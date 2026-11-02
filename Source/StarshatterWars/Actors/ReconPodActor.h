#pragma once
#include "MissileActor.h"
#include "ReconPodActor.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable, BlueprintType)
class STARSHATTERWARS_API AReconPodActor : public AMissileActor
{
    GENERATED_BODY()
public:
    AReconPodActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Missile|Mesh")
    TObjectPtr<UStaticMeshComponent> MeshComponent;
};
