#pragma once
#include "GameFramework/Actor.h"
#include "MiningDroneActor.generated.h"
UCLASS()
class STARSHATTERWARS_API AMiningDroneActor : public AActor
{
    GENERATED_BODY()
public:
    AMiningDroneActor();
    void UpdateVisual(FVector Position,FVector Heading,bool Mining,FVector BeamEnd);
    void Tick(float DeltaSeconds) override;
private:
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Body;
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Arms;
    FVector Destination=FVector::ZeroVector;
    FVector BeamTarget=FVector::ZeroVector;
    bool bFirstPosition=true;
    bool bMining=false;
};
