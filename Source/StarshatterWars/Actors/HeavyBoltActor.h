#pragma once
#include "BoltWeaponActor.h"
#include "HeavyBoltActor.generated.h"

/** Presentation class for Wep.def's Heavy Cannon (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Heavy Cannon Bolt"))
class STARSHATTERWARS_API AHeavyBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    AHeavyBoltActor();
};
