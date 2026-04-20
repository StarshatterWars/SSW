#include "GasGiantActor.h"

AGasGiantActor::AGasGiantActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AGasGiantActor::SetPlanetVisualValues(
    float InPlanetRadius,
    float InAtmosphereHeight,
    float InAtmosphereSteps)
{
    PlanetRadius = FMath::Max(0.001f, InPlanetRadius);
    AtmosphereHeight = FMath::Max(0.0f, InAtmosphereHeight);
    AtmosphereSteps = FMath::Max(0.0f, InAtmosphereSteps);

    ApplyPlanetVisuals();
}

void AGasGiantActor::SetPlanetRadius(float InPlanetRadius)
{
    PlanetRadius = FMath::Max(0.001f, InPlanetRadius);
    ApplyPlanetVisuals();
}