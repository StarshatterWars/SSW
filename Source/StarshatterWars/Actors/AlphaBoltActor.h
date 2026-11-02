#pragma once
#include "BoltWeaponActor.h"
#include "AlphaBoltActor.generated.h"

/** Presentation class for Wep.def's Alpha Cannon (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Alpha Cannon Bolt"))
class STARSHATTERWARS_API AAlphaBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    AAlphaBoltActor();
};
