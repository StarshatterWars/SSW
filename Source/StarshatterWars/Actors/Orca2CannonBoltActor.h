#pragma once
#include "BoltWeaponActor.h"
#include "Orca2CannonBoltActor.generated.h"

/** Presentation class for Wep.def's Orca 2 Cannon (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Orca 2 Cannon Bolt"))
class STARSHATTERWARS_API AOrca2CannonBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    AOrca2CannonBoltActor();
};
