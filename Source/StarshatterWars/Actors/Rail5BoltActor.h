#pragma once
#include "BoltWeaponActor.h"
#include "Rail5BoltActor.generated.h"

/** Presentation class for Wep.def's Rail-5 (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Rail-5 Bolt"))
class STARSHATTERWARS_API ARail5BoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    ARail5BoltActor();
};
