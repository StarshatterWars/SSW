#pragma once
#include "CoreMinimal.h"
#include "SimRegion.h"
#include "SimShot.h"
#include "Drone.h"
namespace ProjectilePresentationList {
inline TArray<SimShot*> Collect(SimRegion& Region){
 TArray<SimShot*> Result;
 for(int I=0;I<Region.GetShots().size();++I)if(auto* S=Region.GetShots()[I])Result.AddUnique(S);
 for(int I=0;I<Region.GetDrones().size();++I)if(auto* D=Region.GetDrones()[I])Result.AddUnique(D);
 return Result;
}
}
