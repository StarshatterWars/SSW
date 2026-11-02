#pragma once
#include "BoltWeaponActor.h"
#include "UrsaCannonBoltActor.generated.h"

/** Presentation class for Wep.def's Ursa Cannon (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Ursa Cannon Bolt"))
class STARSHATTERWARS_API AUrsaCannonBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    AUrsaCannonBoltActor();
};
