#pragma once
#include "CoreMinimal.h"
class Surface;
class SimModel;
struct Poly;
namespace MeshCollision
{
    bool RaySphere(const FVector& Origin, const FVector& UnitDirection, double Length,
        const FVector& Center, double Radius, FVector& Hit);
    bool Ray(SimModel& Model, const FTransform& Pose, const FVector& Origin,
        const FVector& UnitDirection, double Length, bool Translucent, FVector& Hit, Poly*& HitPoly);
    bool HasGeometry(SimModel& Model);
    bool Overlap(SimModel& A, const FTransform& APose, SimModel& B, const FTransform& BPose);
}
