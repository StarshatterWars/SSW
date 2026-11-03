#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BoltTurretValidationRigActor.generated.h"
struct FBoltTurretValidationState;
/** Isolated development-only live bolt and turret range; never spawned by normal gameplay. */
UCLASS()
class STARSHATTERWARS_API ABoltTurretValidationRigActor : public AActor
{
 GENERATED_BODY()
public:
 ABoltTurretValidationRigActor();virtual ~ABoltTurretValidationRigActor();
 virtual void BeginPlay() override;virtual void Tick(float Seconds) override;
private:
 FBoltTurretValidationState* State=nullptr;
};
