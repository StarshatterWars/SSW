#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaypointActor.generated.h"

// Blueprint parent for BP_Waypoint. Data marker only; the Nav map draws its icon.
UCLASS(Blueprintable)
class STARSHATTERWARS_API AWaypointActor : public AActor
{
    GENERATED_BODY()
public:
    AWaypointActor();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") int32 ElementIdentity = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") int32 WaypointIndex = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") FString SectorName;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") FString Description;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") FVector LegacyLocation = FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") bool bNextWaypoint = false;
};
