#pragma once
#include "BoltWeaponActor.h"
#include "FighterBoltActor.generated.h"

/** Presentation class for Wep.def's Fighter Turret (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Fighter Turret Bolt"))
class STARSHATTERWARS_API AFighterBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    AFighterBoltActor();
};
