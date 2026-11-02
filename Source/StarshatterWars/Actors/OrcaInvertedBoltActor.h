#pragma once
#include "BoltWeaponActor.h"
#include "OrcaInvertedBoltActor.generated.h"

/** Presentation class for Wep.def's Orca Inverted (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Orca Inverted Bolt"))
class STARSHATTERWARS_API AOrcaInvertedBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    AOrcaInvertedBoltActor();
};
