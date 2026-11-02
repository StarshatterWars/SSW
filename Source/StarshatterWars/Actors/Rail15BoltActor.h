#pragma once
#include "BoltWeaponActor.h"
#include "Rail15BoltActor.generated.h"

/** Presentation class for Wep.def's Rail-15 (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Rail-15 Bolt"))
class STARSHATTERWARS_API ARail15BoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    ARail15BoltActor();
};
