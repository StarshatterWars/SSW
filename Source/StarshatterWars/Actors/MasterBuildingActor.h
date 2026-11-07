#pragma once
#include "ShipActor.h"
#include "MasterBuildingActor.generated.h"

/** Ground presentation uses ordinary Unreal centimetres; legacy Ship owns combat/state. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API AMasterBuildingActor : public AShipActor
{
    GENERATED_BODY()
public:
    AMasterBuildingActor();
    virtual void OnConstruction(const FTransform& Transform) override;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ground|Definition") FName GroundDefinition;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ground|Placeholder", meta=(ClampMin="1"))
    FVector PlaceholderSizeCm = FVector(1000.f,1000.f,1000.f);
protected:
    UPROPERTY(VisibleAnywhere, Category="Ground|Placeholder") TObjectPtr<UStaticMeshComponent> PlaceholderMesh;
    void UpdateGroundPlaceholder();
};
