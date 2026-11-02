#pragma once
#include "BoltWeaponActor.h"
#include "Rail12InvertedBoltActor.generated.h"

/** Presentation class for Wep.def's Rail-12 Inverted (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Rail-12 Inverted Bolt"))
class STARSHATTERWARS_API ARail12InvertedBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    ARail12InvertedBoltActor();
};
