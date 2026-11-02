#pragma once
#include "BoltWeaponActor.h"
#include "OrcaBoltActor.generated.h"

/** Presentation class for Wep.def's Orca Cannon (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Orca Cannon Bolt"))
class STARSHATTERWARS_API AOrcaBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    AOrcaBoltActor();
};
