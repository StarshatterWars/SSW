#pragma once
#include "BoltWeaponActor.h"
#include "VanguardBoltActor.generated.h"

/** Presentation class for Wep.def's Vanguard (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Vanguard Bolt"))
class STARSHATTERWARS_API AVanguardBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    AVanguardBoltActor();
};
