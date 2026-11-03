#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BeamValidationRigActor.generated.h"
struct FBeamValidationState;
/** Isolated development-only live beam range; never spawned by normal gameplay. */
UCLASS()
class STARSHATTERWARS_API ABeamValidationRigActor : public AActor
{
 GENERATED_BODY()
public:
 ABeamValidationRigActor();virtual ~ABeamValidationRigActor();
 virtual void BeginPlay() override;virtual void Tick(float Seconds) override;
private:
 FBeamValidationState* State=nullptr;
};
