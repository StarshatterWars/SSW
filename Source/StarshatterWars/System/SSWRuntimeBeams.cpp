#include "SSWRuntimeSubsystem.h"
#include "BeamWeaponActor.h"
void USSWRuntimeSubsystem::SyncBeamPresentationClock()
{
 for(auto& P:ProjectileVisuals)if(auto* B=Cast<ABeamWeaponActor>(P.Value.Get()))B->SetPresentationClock(bPaused,TimeCompression);
}
