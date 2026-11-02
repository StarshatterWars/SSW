#pragma once
#include "BoltWeaponActor.h"
#include "LightPDBBoltActor.generated.h"

/** Presentation class for Wep.def's Light-PDB (graphic_type 3).
 * Simulation owns ammunition/charge, trajectory, collision and damage.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Light-PDB Bolt"))
class STARSHATTERWARS_API ALightPDBBoltActor : public ABoltWeaponActor
{
    GENERATED_BODY()
public:
    ALightPDBBoltActor();
};
