#pragma once
#include "BoltWeaponActor.h"
#include "VikingCannonBoltActor.generated.h"

/** Presentation class for Wep.def's Viking Cannon (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Viking Cannon Bolt"))
class STARSHATTERWARS_API AVikingCannonBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    AVikingCannonBoltActor();
};
