// QANGA // IOLACORP. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WorldscapeSubsystem.h"
#include "WorldscapeCollisionInvoker.generated.h"

//forward declaration
class UWorldScapeRoot;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class WORLDSCAPECORE_API UWorldscapeCollisionInvoker : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UWorldscapeCollisionInvoker();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
		void AddWolrdScapeCollisionInvoker(UWorldscapeCollisionInvoker* WorldScapeCollisionInvoker);

	UFUNCTION()
		void RemoveWolrdScapeCollisionInvoker(UWorldscapeCollisionInvoker* WorldScapeCollisionInvoker);
		
};
