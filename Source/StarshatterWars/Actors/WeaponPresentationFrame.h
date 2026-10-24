#pragma once
#include "CoreMinimal.h"
#include "Camera.h"
#include "Math/RotationMatrix.h"

namespace WeaponPresentationFrame
{
    inline FQuat Rotation(const Camera& Frame)
    {
        const FVector F = Frame.vpn(), U = Frame.vup();
        return FRotationMatrix::MakeFromXZ(FVector(F.Z,F.X,F.Y),FVector(U.Z,U.X,U.Y)).ToQuat();
    }
    inline FQuat Display(const Camera& Frame, const Camera& SimShip, const FQuat& DisplayShip)
    {
        return (DisplayShip * Rotation(SimShip).Inverse() * Rotation(Frame)).GetNormalized();
    }
}
