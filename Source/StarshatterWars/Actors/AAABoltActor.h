#pragma once
#include "BoltWeaponActor.h"
#include "AAABoltActor.generated.h"

/** Presentation class for Wep.def's AAA Cannon (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "AAA Cannon Bolt"))
class STARSHATTERWARS_API AAAABoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    AAAABoltActor();
};
