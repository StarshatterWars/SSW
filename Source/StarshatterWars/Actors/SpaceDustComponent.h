#pragma once
#include "Components/InstancedStaticMeshComponent.h"
#include "SpaceDustComponent.generated.h"
/** Bounded, camera-relative visual dust; never participates in legacy physics. */
UCLASS(ClassGroup=Rendering,meta=(BlueprintSpawnableComponent))
class STARSHATTERWARS_API USpaceDustComponent : public UInstancedStaticMeshComponent
{
    GENERATED_BODY()
public:
    USpaceDustComponent();
    void SetPlayerActive(bool Active);
    virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* TickFunction) override;
    UPROPERTY(EditAnywhere,Category="Space Dust",meta=(ClampMin="10")) float DustRadius=300.f;
private:
    bool bPlayerActive=false,bHaveCamera=false;
    FVector PreviousCamera=FVector::ZeroVector;
    TArray<FVector> Specks;
    TArray<FTransform> DustTransforms;
    FRandomStream Random{45045};
};
