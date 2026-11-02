#pragma once
#include "BoltWeaponActor.h"
#include "VulcanBoltActor.generated.h"

/** Presentation class for Wep.def's Vulcan Cannon (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Vulcan Cannon Bolt"))
class STARSHATTERWARS_API AVulcanBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    AVulcanBoltActor();
};
