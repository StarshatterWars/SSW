#pragma once
#include "CoreMinimal.h"
#include "SimRegion.h"
#include "SimShot.h"
#include "Drone.h"
namespace ProjectilePresentationList {
inline TArray<SimShot*> Collect(SimRegion& Region){
 return Region.GetProjectiles();
}
}
