#pragma once
#include "BoltWeaponActor.h"
#include "PhalanxCannonBoltActor.generated.h"

/** Presentation class for Wep.def's Phalanx Cannon (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Phalanx Cannon Bolt"))
class STARSHATTERWARS_API APhalanxCannonBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    APhalanxCannonBoltActor();
};
