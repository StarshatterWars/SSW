#pragma once
#include "PlanetActor.h"
#include "GasGiantActor.h"
#include "WorldScapeRoot.h"
#include "NativeTerrainActor.h"

namespace MissionSceneIsolation
{
    inline bool IsWorldSurface(const AActor* Actor)
    {
        return Actor && (Actor->IsA<APlanetActor>() || Actor->IsA<AGasGiantActor>() ||
            Actor->IsA<AWorldScapeRoot>() || Actor->IsA<ANativeTerrainActor>());
    }
    inline bool BelongsToLevel(const AActor* Actor,const ULevel* Level)
    {
        if(!Level)return false;
        for(const AActor* Owner=Actor;Owner;Owner=Owner->GetOwner())
            if(Owner->GetLevel()==Level)return true;
        return false;
    }
}
