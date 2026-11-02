#pragma once
#include "BoltWeaponActor.h"
#include "VikingBoltActor.generated.h"

/** Presentation class for Wep.def's Viking Cannon (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Viking Cannon Bolt"))
class STARSHATTERWARS_API AVikingBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    AVikingBoltActor();
};
