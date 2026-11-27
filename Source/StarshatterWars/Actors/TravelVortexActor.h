#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TravelVortexActor.generated.h"
class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
UCLASS()
class STARSHATTERWARS_API ATravelVortexActor : public AActor
{
    GENERATED_BODY()
public:
    ATravelVortexActor();
    bool Configure(float Radius, float Length);
    void SetEnvelope(bool bArrival, float Strength);
    UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> TunnelMesh;
private:
    UPROPERTY() TObjectPtr<class UMaterialInterface> PackMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Material;
    bool bBuilt=false;
};
