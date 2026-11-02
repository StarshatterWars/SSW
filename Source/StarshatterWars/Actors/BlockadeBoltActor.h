#pragma once
#include "BoltWeaponActor.h"
#include "BlockadeBoltActor.generated.h"

/** Presentation class for Wep.def's Blockade Cannon (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Blockade Cannon Bolt"))
class STARSHATTERWARS_API ABlockadeBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    ABlockadeBoltActor();
};
