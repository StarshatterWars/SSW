#pragma once
#include "BoltWeaponActor.h"
#include "DeltaCannonBoltActor.generated.h"

/** Presentation class for Wep.def's Delta Cannon (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Delta Cannon Bolt"))
class STARSHATTERWARS_API ADeltaCannonBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    ADeltaCannonBoltActor();
};
