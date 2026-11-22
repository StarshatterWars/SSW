#pragma once
#include "Engine/World.h"

// The simulation owns vessels across streamed scenery changes. Their UE proxies
// must have the same lifetime, rather than belonging to a departing system map.
namespace RuntimeShipVisualLifetime
{
    inline void ConfigureSpawn(FActorSpawnParameters& Params,UWorld& World)
    {
        Params.OverrideLevel=World.PersistentLevel;
        Params.Owner=nullptr;
    }
}
