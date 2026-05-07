#pragma once

#include "CoreMinimal.h"
#include "GameStructs_System.h"

class STARSHATTERWARS_API ShipUtils
{
public:

	// Legacy -> Unreal coordinate conversion
	static FORCEINLINE FVector LegacySimToUnreal(const FVector& V)
	{
		// legacy: X=right, Y=up, Z=forward
		// unreal: X=forward, Y=right, Z=up
		return FVector(V.Z, V.X, V.Y);
	}

	static FORCEINLINE FVector UnrealToLegacySim(const FVector& V)
	{
		// reverse mapping
		return FVector(V.Y, V.Z, V.X);
	}

	static EExplosionType ExplosionTypeFromInt(int32 Value);
	static int32 ExplosionTypeToInt(EExplosionType Type);
};
