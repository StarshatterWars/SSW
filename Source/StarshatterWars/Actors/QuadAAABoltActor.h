#pragma once
#include "BoltWeaponActor.h"
#include "QuadAAABoltActor.generated.h"

/** Presentation class for Wep.def's Quad AAA (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Quad AAA Bolt"))
class STARSHATTERWARS_API AQuadAAABoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    AQuadAAABoltActor();
};
