#pragma once
#include "BoltWeaponActor.h"
#include "FighterTurretBoltActor.generated.h"

/** Presentation class for Wep.def's Fighter Turret (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Fighter Turret Bolt"))
class STARSHATTERWARS_API AFighterTurretBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    AFighterTurretBoltActor();
};
