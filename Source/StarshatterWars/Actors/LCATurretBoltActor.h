#pragma once
#include "BoltWeaponActor.h"
#include "LCATurretBoltActor.generated.h"

/** Presentation class for Wep.def's LCA Turret (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "LCA Turret Bolt"))
class STARSHATTERWARS_API ALCATurretBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    ALCATurretBoltActor();
};
