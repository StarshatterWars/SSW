#pragma once
#include "CoreMinimal.h"
class AShipActor;
class UStaticMesh;
class SimModel;
class Camera;
class Weapon;
/** Uses cooked mesh triangles; no trace channels, physics simulation or asset mutation. */
class FNativeShipGeometry
{
public:
    // -1 means no native hull geometry; 0 means a verified mesh miss; 1 means hit.
    int32 Ray(const AShipActor& Actor,const Camera& Frame,const FVector& Origin,const FVector& Direction,double Length,FVector& Hit,Weapon*& HitWeapon);
    int32 Overlap(const AShipActor& Actor,const Camera& Frame,FNativeShipGeometry& Other,const AShipActor& OtherActor,const Camera& OtherFrame);
private:
    TMap<TWeakObjectPtr<UStaticMesh>,TSharedPtr<SimModel>> Models;
    TSharedPtr<SimModel> Model(UStaticMesh* Mesh);
};
