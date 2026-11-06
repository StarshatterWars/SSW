// QANGA // IOLACORP. All Rights Reserved


#include "WorldscapeCollisionInvoker.h"
#include "WorldScapeRoot.h"

// Sets default values for this component's properties
UWorldscapeCollisionInvoker::UWorldscapeCollisionInvoker()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// ...
}


// Called when the game starts
void UWorldscapeCollisionInvoker::BeginPlay()
{
	Super::BeginPlay();
	AddWolrdScapeCollisionInvoker(this);
	// ...
	
}

void UWorldscapeCollisionInvoker::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveWolrdScapeCollisionInvoker(this);
	Super::EndPlay(EndPlayReason);
}


// Called every frame
void UWorldscapeCollisionInvoker::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UWorldscapeCollisionInvoker::AddWolrdScapeCollisionInvoker(UWorldscapeCollisionInvoker* WorldScapeCollisionInvoker)
{
	if (IsValid(WorldScapeCollisionInvoker) && IsValid(WorldScapeCollisionInvoker->GetOwner())) {
		AActor* TargetOwnerActor = WorldScapeCollisionInvoker->GetOwner();
		GetOwner()->GetWorld()->GetSubsystem<UWorldscapeSubsystem>()->WorldScapeCollisionInvokers.Add(WorldScapeCollisionInvoker);
		for (auto& Elem : GetOwner()->GetWorld()->GetSubsystem<UWorldscapeSubsystem>()->WorldScapeActors)
		{
			if (IsValid(Elem)) {
				Elem->CollisionDependantActor.Add(TargetOwnerActor);
				Elem->UpdatePosition();
			}
		}

	}
}

void UWorldscapeCollisionInvoker::RemoveWolrdScapeCollisionInvoker(UWorldscapeCollisionInvoker* WorldScapeCollisionInvoker)
{
	if (IsValid(WorldScapeCollisionInvoker) && IsValid(WorldScapeCollisionInvoker->GetOwner())) {
		AActor* TargetOwnerActor = WorldScapeCollisionInvoker->GetOwner();
		GetOwner()->GetWorld()->GetSubsystem<UWorldscapeSubsystem>()->WorldScapeCollisionInvokers.Remove(WorldScapeCollisionInvoker);
		for (auto& Elem : GetOwner()->GetWorld()->GetSubsystem<UWorldscapeSubsystem>()->WorldScapeActors)
		{
			if (IsValid(Elem)) {
				Elem->CollisionDependantActor.Remove(TargetOwnerActor);
			}
		}

	}
}

