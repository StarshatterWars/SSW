#pragma once
#include "BoltWeaponActor.h"
#include "HeliconBoltActor.generated.h"

/** Presentation class for Wep.def's Helicon (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Helicon Bolt"))
class STARSHATTERWARS_API AHeliconBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    AHeliconBoltActor();
};
