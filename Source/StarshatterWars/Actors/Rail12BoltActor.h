#pragma once
#include "BoltWeaponActor.h"
#include "Rail12BoltActor.generated.h"

/** Presentation class for Wep.def's Rail-12 (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Rail-12 Bolt"))
class STARSHATTERWARS_API ARail12BoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    ARail12BoltActor();
};
