#include "SSWRuntimeSubsystem.h"
#include "ExplosionActor.h"
#include "MuzzleFlashExplosionActor.h"
#include "ShipActor.h"
#include "Ship.h"
#include "WeaponPresentationFrame.h"
#include "Explosion.h"
#include "Sim.h"
#include "SimRegion.h"
#include "Engine/World.h"
void USSWRuntimeSubsystem::ClearExplosionVisuals()
{
 for(auto& P:ExplosionVisuals)if(P.Value.IsValid())P.Value->Destroy();
 ExplosionVisuals.Empty();ExplosionVisualRegion=nullptr;Explosion::ClearPresentationEvents();
}
void USSWRuntimeSubsystem::SyncExplosionClock()
{
 for(auto& P:ExplosionVisuals)if(auto* A=P.Value.Get())A->SetPresentationClock(bPaused,TimeCompression);
}
void USSWRuntimeSubsystem::UpdateExplosionVisuals(double Seconds)
{
 auto Events=Explosion::ConsumePresentationEvents();
 SimRegion* Region=SimInstance?SimInstance->GetActiveRegion():nullptr;
 if(!GetWorld() || !Region || !MissionPresentationBuilder.IsValid()){ClearExplosionVisuals();return;}
 if(ExplosionVisualRegion!=Region){for(auto& P:ExplosionVisuals)if(P.Value.IsValid())P.Value->Destroy();ExplosionVisuals.Empty();ExplosionVisualRegion=Region;}
 TMap<uint64,FExplosionPresentation> Current;TSet<uint64> Live;
 for(auto& E:Events)if(E.Region==Region)Current.Add(E.Serial,E);
 auto& Explosions=Region->GetExplosions();
 for(int I=0;I<Explosions.size();++I)if(auto* E=Explosions[I]){
  const auto State=E->GetPresentation();Current.Add(State.Serial,State);Live.Add(State.Serial);
 }
 for(const auto& P:Current){
  auto* Actor=ExplosionVisuals.FindRef(P.Key).Get();const auto& State=P.Value;
  if(!Actor){
   UClass* Class=State.bMuzzleFlash?AMuzzleFlashExplosionActor::MuzzleClass().Get():AExplosionActor::VisualClass(State.Type).Get();if(!Class)continue;
   FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
   Actor=GetWorld()->SpawnActor<AExplosionActor>(Class,GetVisualLocationForRegion(Region,State.Location),FRotator::ZeroRotator,Params);
   if(!Actor)continue;if(auto* M=Cast<AMuzzleFlashExplosionActor>(Actor))M->SetFlashAppearance(State.FlashId,State.ExplosionScale);ExplosionVisuals.Add(P.Key,Actor);Actor->PlayExplosion(State.ExplosionScale,State.ParticleScale,State.Lifetime);
  }
  if(Live.Contains(P.Key))Actor->SyncSimulationLifetime(State.Lifetime);
  Actor->SetPresentationClock(bPaused,TimeCompression);
  Actor->SetActorLocation(GetVisualLocationForRegion(Region,State.Location));Actor->SetActorHiddenInGame(State.bHidden);
  if(State.bMuzzleFlash){
   const FVector Forward=ConvertLegacyShipLocationToUE(State.Direction).GetSafeNormal();
   if(!Forward.IsNearlyZero())Actor->SetActorRotation(Forward.Rotation());
   // Validate non-owning shooter identity against the live region before lookup.
   Ship* Shooter=const_cast<Ship*>(State.Shooter);
   if(Shooter && Region->GetShips().contains(Shooter))
    if(auto* Visual=FindRegionActorForRuntimeShip(Shooter))Actor->AttachToActor(Visual,FAttachmentTransformRules::KeepWorldTransform);
  }
 }
 for(auto It=ExplosionVisuals.CreateIterator();It;++It){
  auto* A=It.Value().Get();
  if(!A){It.RemoveCurrent();continue;}
  if(A->IsPersistentPresentation() && !Live.Contains(It.Key())){A->Destroy();It.RemoveCurrent();continue;}
  if(!Current.Contains(It.Key()))A->AdvancePresentation(Seconds);
 }
}
