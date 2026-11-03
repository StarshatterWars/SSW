#pragma once
#include "BoltWeaponActor.h"
#include "ZolonBoltActor.generated.h"

/** Presentation class for Wep.def's Zolon Cannon (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Zolon Cannon Bolt"))
class STARSHATTERWARS_API AZolonBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    AZolonBoltActor();
};
