#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "WeaponDesignRegistry.h"
#include "WeaponDesign.h"
#include "WeaponBlueprintResolver.h"
#include "BeamWeaponActor.h"
#include "SimShot.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeamWeaponPresentationTest,"Starshatter.Weapons.BeamPresentation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBeamWeaponPresentationTest::RunTest(const FString&)
{
 const auto Before=WeaponDesignRegistry::GetAll();ON_SCOPE_EXIT{WeaponDesignRegistry::Clear();for(const auto& P:Before)WeaponDesignRegistry::RegisterDesign(P.Key,P.Value);};
 auto* Importer=NewObject<UStarshatterWeaponDesignSubsystem>(NewObject<UGameInstance>());const FString File=FPaths::ProjectContentDir()/TEXT("GameData/Weapons/Wep.def");const auto Ansi=StringCast<ANSICHAR>(*File);Importer->LoadWeaponDesign(Ansi.Get());
 const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);if(!TestNotNull(TEXT("World"),World))return false;ON_SCOPE_EXIT{World->DestroyWorld(false);};
 const TCHAR* Names[]={TEXT("XRay Laser"),TEXT("Gamma Laser"),TEXT("Graser Turret"),TEXT("Zolon Laser")};
 const double Damages[]={3,4,6,30};const double Lives[]={1.5,3,3,3};const double Charges[]={1000,1500,1500,1500};
 for(int I=0;I<4;++I){
  const FString Name=Names[I];const auto* Row=WeaponDesignRegistry::Find(Name);auto* Design=WeaponDesignRegistry::FindLegacy(Name);if(!TestNotNull(TEXT("Definition"),Row)||!TestNotNull(TEXT("Legacy design"),Design))continue;
  TestTrue(TEXT("Still legacy beam"),Row->bBeam && Design->beam);TestEqual(TEXT("Damage unchanged"),double(Design->damage),Damages[I]);TestEqual(TEXT("Lifetime unchanged"),double(Design->life),Lives[I]);TestEqual(TEXT("Charge unchanged"),double(Design->charge),Charges[I]);
  TestEqual(TEXT("Min range unchanged"),double(Design->min_range),10000.0);TestEqual(TEXT("Max range unchanged"),double(Design->max_range),50000.0);TestEqual(TEXT("Minimum firing charge unchanged"),double(Design->min_charge),500.0);
  const FString Path=WeaponBlueprintResolver::ProjectileClassPath(Row->Projectile);TestEqual(TEXT("New field resolves organized beam path"),Path,ABeamWeaponActor::BlueprintPathForWeapon(Name));
  UClass* Class=WeaponBlueprintResolver::ResolveProjectile(Row->Projectile);if(!TestNotNull(TEXT("Per-weapon Blueprint"),Class))continue;TestTrue(TEXT("Correct native child"),Class->IsChildOf(ABeamWeaponActor::ClassForWeapon(Name)));
  auto* Beam=World->SpawnActor<ABeamWeaponActor>(Class);Beam->BeginSimulationBeam(Name);TestFalse(TEXT("Noncolliding presentation"),Beam->GetActorEnableCollision());TestFalse(TEXT("Simulation owns lifetime"),Beam->IsActorTickEnabled());
  auto* FX=Beam->FindComponentByClass<UNiagaraComponent>();if(!TestNotNull(TEXT("Niagara"),FX))continue;TestNotNull(TEXT("Pack system assigned"),FX->GetAsset());
  Beam->SetActorRotation(FRotator(10,45,15));const FVector Start(1000,-2000,3000),Impact(4000,5000,-1000);Beam->SetBeamPoints(Start,Impact);
  TestTrue(TEXT("Muzzle start exact"),Beam->GetBeamStart().Equals(Start));TestTrue(TEXT("Impact end exact"),Beam->GetBeamEnd().Equals(Impact));
  const uint8* Data=FX->GetOverrideParameters().GetParameterData(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),TEXT("User.beamEnd")));
  if(TestNotNull(TEXT("Correct case/type endpoint parameter"),Data)){
   const FVector Local(*reinterpret_cast<const FVector3f*>(Data));TestTrue(TEXT("Bounds contain full beam, not only pack default length"),FX->GetSystemFixedBounds().IsInsideOrOn(Local));TestTrue(TEXT("Pack local endpoint reconstructs exact world impact"),FX->GetComponentTransform().TransformPosition(Local).Equals(Impact,.01));
  }
  const uint8* Color=FX->GetOverrideParameters().GetParameterData(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),TEXT("User.color")));
  if(TestNotNull(TEXT("Pack Vector3 color parameter"),Color)){
   const auto C=*reinterpret_cast<const FVector3f*>(Color);
   TestTrue(TEXT("Legacy dominant color retained"),I==0?(C.X>C.Z && C.Y>C.Z):(I==3?C.Y>C.X && C.Y>C.Z:C.Z>C.X && C.Z>C.Y));
  }
  Beam->SetPresentationClock(true,2);TestTrue(TEXT("Runtime pause freezes beam"),FX->IsPaused());Beam->SetPresentationClock(false,2);TestFalse(TEXT("Runtime resume"),FX->IsPaused());
  Camera Aim;SimShot Shot(FVector::ZeroVector,Aim,Design,nullptr);Shot.SetCharge(Design->charge*.5f);TestEqual(TEXT("Half energy retains legacy half duration"),Shot.GetLife(),Lives[I]*.5);
  Shot.SetBeamPoints(FVector::ZeroVector,FVector(0,0,5000));Shot.ExecFrame(.1);const double D=Shot.Damage();TestTrue(TEXT("Beam damage is nonzero despite Game::FrameTime stub"),D>0);TestTrue(TEXT("Legacy damage equation uses actual step"),FMath::IsNearlyEqual(D,Damages[I]*Charges[I]*.5*.1,.001));
  Shot.ExecFrame(.05);TestTrue(TEXT("Damage scales with simulation step"),FMath::IsNearlyEqual(Shot.Damage(),D*.5,.001));
  const double Life=Shot.GetLife();Beam->SetBeamPoints(Start,Impact);TestEqual(TEXT("FX updates do not drain energy or shorten life"),Shot.GetLife(),Life);TestNull(TEXT("No raw-coordinate fallback actor created"),Shot.GetBeamActor());
  Beam->SetBeamActive(false);TestTrue(TEXT("Beam cleanup"),Beam->IsActorBeingDestroyed());
 }
 return true;
}
#endif
