#include "SSWRuntimeSubsystem.h"
#include "BeamWeaponActor.h"
#include "NiagaraComponent.h"
void USSWRuntimeSubsystem::SyncBeamPresentationClock()
{
 for(auto& P:ProjectileVisuals)if(auto* Actor=P.Value.Get()){
  if(auto* B=Cast<ABeamWeaponActor>(Actor))B->SetPresentationClock(bPaused,TimeCompression);
  TArray<UNiagaraComponent*> Effects;Actor->GetComponents(Effects);
  for(auto* FX:Effects){FX->SetPaused(bPaused);FX->SetCustomTimeDilation(TimeCompression);}
 }
}
