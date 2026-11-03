#pragma once
#include "BoltWeaponActor.h"
#include "PlasmaBoltActor.generated.h"

/** Presentation class for Wep.def's Plasma Cannon (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Plasma Cannon Bolt"))
class STARSHATTERWARS_API APlasmaBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    APlasmaBoltActor();
};
