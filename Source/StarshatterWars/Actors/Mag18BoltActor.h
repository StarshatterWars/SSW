#pragma once
#include "BoltWeaponActor.h"
#include "Mag18BoltActor.generated.h"

/** Presentation class for Wep.def's Mag-18 (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Mag-18 Bolt"))
class STARSHATTERWARS_API AMag18BoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    AMag18BoltActor();
};
