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
    GasGiantRadius = FMath::Max(0.001f, InPlanetRadius);
    AtmosphereHeight = FMath::Max(0.0f, InAtmosphereHeight);
    AtmosphereSteps = FMath::Max(0.0f, InAtmosphereSteps);

    APlanetActor::SetPlanetRadius(GasGiantRadius);

    ApplyPlanetVisuals();
}

void AGasGiantActor::SetPlanetRadius(float InPlanetRadius)
{
    GasGiantRadius = FMath::Max(0.001f, InPlanetRadius);

    APlanetActor::SetPlanetRadius(GasGiantRadius);

    ApplyPlanetVisuals();
}