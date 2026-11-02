#pragma once
#include "BoltWeaponActor.h"
#include "VulcanCannonBoltActor.generated.h"

/** Presentation class for Wep.def's Vulcan Cannon (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Vulcan Cannon Bolt"))
class STARSHATTERWARS_API AVulcanCannonBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    AVulcanCannonBoltActor();
};
