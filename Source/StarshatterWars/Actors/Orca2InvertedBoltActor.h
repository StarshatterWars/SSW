#pragma once
#include "BoltWeaponActor.h"
#include "Orca2InvertedBoltActor.generated.h"

/** Presentation class for Wep.def's Orca 2 Inverted (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Orca 2 Inverted Bolt"))
class STARSHATTERWARS_API AOrca2InvertedBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    AOrca2InvertedBoltActor();
};
