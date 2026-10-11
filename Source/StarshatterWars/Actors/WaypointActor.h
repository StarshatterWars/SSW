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
    // Fixed legacy rule, not a BP-adjustable arrival distance.
    static double LegacyArrivalRadius(double HullRadius);
    void SetArrivalHullRadius(double HullRadius);
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") double ArrivalRadiusMeters = 0.0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") TObjectPtr<class USphereComponent> ArrivalSphere;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation|Appearance")
    TObjectPtr<class UStaticMeshComponent> WaypointMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") int32 ElementIdentity = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") int32 WaypointIndex = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") FString SectorName;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") FString Description;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") FVector LegacyLocation = FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Navigation") bool bNextWaypoint = false;
};
