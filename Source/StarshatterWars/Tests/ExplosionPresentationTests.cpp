#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "ExplosionActor.h"
#include "QuantumEffectsActor.h"
#include "Components/MaterialBillboardComponent.h"
#include "MuzzleFlashExplosionActor.h"
#include "WeaponDesignRegistry.h"
#include "WeaponDesign.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "Engine/GameInstance.h"
#include "Explosion.h"
#include "NiagaraComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleEmitter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
namespace {
class FExplosionProbe : public Explosion {
public:
 FExplosionProbe(EExplosionType T,const FVector& P,float E,float S,SimObject* Src=nullptr):Explosion(T,P,FVector::ZeroVector,E,S,nullptr,Src){}
 void ForceLife(double L){life=L;}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExplosionPresentationTest,"Starshatter.Effects.ExplosionPresentation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FExplosionPresentationTest::RunTest(const FString&)
{
 Explosion::ClearPresentationEvents();ON_SCOPE_EXIT{Explosion::ClearPresentationEvents();};
 uint64 First=0;
 {
  FExplosionProbe Short(EExplosionType::HULL_FLASH,FVector(10,20,30),.3f,2);
  Short.ForceLife(.001);Short.QueuePresentationEvent();First=Short.GetPresentation().Serial;
 }
 auto Events=Explosion::ConsumePresentationEvents();
 TestEqual(TEXT("Flash survives simulation deletion"),Events.Num(),1);
 if(Events.Num()){
  TestEqual(TEXT("Exact legacy type"),Events[0].Type,EExplosionType::HULL_FLASH);
  TestTrue(TEXT("Position unchanged in simulation space"),Events[0].Location.Equals(FVector(10,20,30)));
  TestEqual(TEXT("Legacy sprite scale retained"),Events[0].ExplosionScale,.3f);
  TestEqual(TEXT("Legacy particle scale retained"),Events[0].ParticleScale,2.f);
 }
 TestEqual(TEXT("Events consumed once"),Explosion::ConsumePresentationEvents().Num(),0);
 Explosion::QueueMuzzleFlash(FVector(4,5,6),FVector(0,0,1),.5f,nullptr,nullptr);
 auto MuzzleEvents=Explosion::ConsumePresentationEvents();TestEqual(TEXT("One flash per successful fire event"),MuzzleEvents.Num(),1);
 if(MuzzleEvents.Num()){
  TestTrue(TEXT("Muzzle distinct from impacts"),MuzzleEvents[0].bMuzzleFlash);
  TestTrue(TEXT("Hardpoint position retained"),MuzzleEvents[0].Location.Equals(FVector(4,5,6)));
  TestTrue(TEXT("Firing direction retained"),MuzzleEvents[0].Direction.Equals(FVector(0,0,1)));
  TestEqual(TEXT("Flash scale retained"),MuzzleEvents[0].ExplosionScale,.5f);
 }
 Explosion::QueueMuzzleFlash(FVector::ZeroVector,FVector::ZeroVector,0,nullptr,nullptr);
 TestEqual(TEXT("Disabled flash never emitted"),Explosion::ConsumePresentationEvents().Num(),0);
 const auto RegistryBefore=WeaponDesignRegistry::GetAll();
 ON_SCOPE_EXIT{WeaponDesignRegistry::Clear();for(const auto& P:RegistryBefore)WeaponDesignRegistry::RegisterDesign(P.Key,P.Value);};
 auto* Importer=NewObject<UStarshatterWeaponDesignSubsystem>(NewObject<UGameInstance>());
 const FString WepFile=FPaths::ProjectContentDir()/TEXT("GameData/Weapons/Wep.def");const auto WepAnsi=StringCast<ANSICHAR>(*WepFile);Importer->LoadWeaponDesign(WepAnsi.Get());
 const auto* Delta=WeaponDesignRegistry::Find(FName(TEXT("Delta Cannon")));const auto* DeltaLegacy=WeaponDesignRegistry::FindLegacy(FName(TEXT("Delta Cannon")));
 if(TestNotNull(TEXT("Delta definition"),Delta) && TestNotNull(TEXT("Delta runtime design"),DeltaLegacy)){
  TestEqual(TEXT("Flash identity reaches runtime"),FString(ANSI_TO_TCHAR(DeltaLegacy->flash.data())),Delta->Flash);
  TestEqual(TEXT("Flash scale reaches runtime"),DeltaLegacy->flash_scale,Delta->FlashScale);
  TestTrue(TEXT("Delta muzzle flash enabled"),DeltaLegacy->flash.length()>0 && DeltaLegacy->flash_scale>0);
 }

 FExplosionProbe Next(EExplosionType::SMALL_EXPLOSION,FVector::ZeroVector,1,1);
 TestTrue(TEXT("Stable serial prevents pointer-reuse collisions"),Next.GetPresentation().Serial>First);
 SimObject Source("Attached explosion fixture");Source.MoveTo(FVector(100,200,300));
 FExplosionProbe Attached(EExplosionType::HULL_FIRE,FVector(101,202,303),1,1,&Source);Attached.ForceLife(-1);
 Source.MoveTo(FVector(110,220,330));Attached.ExecFrame(.1);
 TestTrue(TEXT("Source-mounted effect follows legacy attachment"),Attached.GetLocation().Equals(FVector(111,222,333),.001));
 Attached.Update(&Source);TestEqual(TEXT("Legacy source destruction retains four-second tail"),Attached.GetLife(),4.0);
 const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
 if(!TestNotNull(TEXT("Preview world"),World))return false;ON_SCOPE_EXIT{World->DestroyWorld(false);};
 for(int I=1;I<=15;++I){
  const auto Type=static_cast<EExplosionType>(I);UClass* Native=AExplosionActor::NativeClass(Type).Get();UClass* Class=AExplosionActor::VisualClass(Type).Get();
  if(!TestNotNull(TEXT("Native subtype"),Native)||!TestNotNull(TEXT("Blueprint subtype"),Class))continue;
  const bool bQuantum = Type == EExplosionType::QUANTUM_FLASH;
  if(bQuantum) TestTrue(TEXT("Legacy quantum flash resolves to the native pack effect"),Class==AQuantumEffectsActor::StaticClass());
  else TestTrue(TEXT("Dedicated Blueprint resolves"),Class!=Native && Class->IsChildOf(Native));
  auto* A=World->SpawnActor<AExplosionActor>(Class);if(!TestNotNull(TEXT("Spawn visual"),A))continue;
  TestEqual(TEXT("Legacy enum unchanged"),A->ExplosionType,Type);
  TestFalse(TEXT("Explosion is cosmetic, no collision"),A->GetActorEnableCollision());
  if(bQuantum){
   const auto* Flash=A->FindComponentByClass<UMaterialBillboardComponent>();
   if(TestNotNull(TEXT("Quantum has a camera-facing flash"),Flash))
    TestNotNull(TEXT("Quantum flash has the pack material"),Flash->GetMaterial(0));
  } else TestTrue(TEXT("Pack effect assigned"),A->NiagaraSystem!=nullptr || A->ParticleSystem!=nullptr);
  A->PlayExplosion(2,3,.2f);TestTrue(TEXT("Cosmetic scale applied once"),A->NiagaraEffect->GetRelativeScale3D().Equals(FVector(2*A->VisualScale)));
  A->SetPresentationClock(true,2);TestTrue(TEXT("Niagara follows runtime pause"),A->NiagaraEffect->IsPaused());A->SetPresentationClock(false,2);
  A->AdvancePresentation(.3);TestTrue(TEXT("Transient visual expires"),A->IsActorBeingDestroyed());
 }
 auto* Muzzle=World->SpawnActor<AExplosionActor>(AMuzzleFlashExplosionActor::MuzzleClass());
 if(TestNotNull(TEXT("Dedicated muzzle Blueprint"),Muzzle)){
  TestNotNull(TEXT("Tiny blast from Space Explosion Pack"),Muzzle->ParticleSystem.Get());
  TestTrue(TEXT("Muzzle flash brief"),Muzzle->FallbackLifetime<=.15f);Muzzle->PlayExplosion(.5f,.5f,0);Muzzle->AdvancePresentation(.2);TestTrue(TEXT("Flash expires"),Muzzle->IsActorBeingDestroyed());
 }
 TestNull(TEXT("NONE creates no fake blast"),AExplosionActor::NativeClass(EExplosionType::NONE).Get());
 TestNull(TEXT("Unknown type creates no fake blast"),AExplosionActor::NativeClass(static_cast<EExplosionType>(31)).Get());
 auto* Persistent=World->SpawnActor<AExplosionActor>();Persistent->PlayExplosion(1,1,-1);Persistent->AdvancePresentation(100);
 TestFalse(TEXT("Persistent effect retained"),Persistent->IsActorBeingDestroyed());Persistent->FinishSustained(.2);Persistent->AdvancePresentation(.3);
 TestTrue(TEXT("Detached persistent effect expires"),Persistent->IsActorBeingDestroyed());
 FString Inventory;
 for(int I=1;I<=15;++I){const FString Path=FString::Printf(TEXT("/Game/Space_Explosion_Pack/Particles/P_Explosion%d.P_Explosion%d"),I,I);
  if(auto* FX=LoadObject<UParticleSystem>(nullptr,*Path)){Inventory+=Path+TEXT(": ");for(const auto& E:FX->Emitters)if(E)Inventory+=E->GetEmitterName().ToString()+TEXT(", ");Inventory+=TEXT("\n");}
 }
 FFileHelper::SaveStringToFile(Inventory,*(FPaths::ProjectSavedDir()/TEXT("SpaceExplosionEmitterInventory.txt")));
 return true;
}
#endif
