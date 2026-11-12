#include "ExplosionActor.h"
#include "Components/SceneComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Misc/PackageName.h"
#include "ShieldFlashExplosionActor.h"
#include "HullFlashExplosionActor.h"
#include "BeamFlashExplosionActor.h"
#include "ShotBlastExplosionActor.h"
#include "HullBurstExplosionActor.h"
#include "HullFireExplosionActor.h"
#include "PlasmaLeakExplosionActor.h"
#include "SmokeTrailExplosionActor.h"
#include "SmallFireExplosionActor.h"
#include "SmallExplosionActor.h"
#include "LargeExplosionActor.h"
#include "LargeBurstExplosionActor.h"
#include "NukeExplosionActor.h"
#include "QuantumEffectsActor.h"
#include "HyperFlashExplosionActor.h"

AExplosionActor::AExplosionActor()
{
 PrimaryActorTick.bCanEverTick=false;SetActorEnableCollision(false);
 EffectRoot=CreateDefaultSubobject<USceneComponent>(TEXT("EffectRoot"));SetRootComponent(EffectRoot);
 NiagaraEffect=CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraEffect"));NiagaraEffect->SetupAttachment(EffectRoot);NiagaraEffect->SetAutoActivate(false);NiagaraEffect->SetAutoDestroy(false);
 ParticleEffect=CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleEffect"));ParticleEffect->SetupAttachment(EffectRoot);ParticleEffect->SetAutoActivate(false);ParticleEffect->bAutoDestroy=false;
 NiagaraEffect->SetCollisionEnabled(ECollisionEnabled::NoCollision);ParticleEffect->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void AExplosionActor::OnConstruction(const FTransform& Transform)
{
 Super::OnConstruction(Transform);NiagaraEffect->SetAsset(NiagaraSystem);ParticleEffect->SetTemplate(ParticleSystem);
}
void AExplosionActor::PlayExplosion(float ExplosionScale,float ParticleScale,float SimulationLifetime)
{
 bPersistent=SimulationLifetime<0;Remaining=SimulationLifetime>0?SimulationLifetime:FallbackLifetime;
 // Pack-specific scale calibration is cosmetic; legacy simulation scale is unchanged.
 NiagaraEffect->SetRelativeScale3D(FVector(FMath::Max(.001f,VisualScale*FMath::Abs(ExplosionScale))));
 ParticleEffect->SetRelativeScale3D(FVector(FMath::Max(.001f,VisualScale*FMath::Abs(ExplosionScale))));
 NiagaraEffect->SetVariableFloat(TEXT("User.LegacyParticleScale"),ParticleScale);
 ParticleEffect->SetFloatParameter(TEXT("LegacyParticleScale"),ParticleScale);
 if(NiagaraSystem){NiagaraEffect->SetAsset(NiagaraSystem);NiagaraEffect->Activate(true);}
 if(ParticleSystem){ParticleEffect->SetTemplate(ParticleSystem);ParticleEffect->ActivateSystem(true);}
}
void AExplosionActor::SetPresentationClock(bool bPaused,float Dilation)
{
 NiagaraEffect->SetPaused(bPaused);NiagaraEffect->SetCustomTimeDilation(Dilation);
 ParticleEffect->CustomTimeDilation=bPaused?0:Dilation;
}
void AExplosionActor::SyncSimulationLifetime(float Seconds)
{
 if(bPersistent && Seconds>=0)FinishSustained(Seconds);
 else if(Seconds>0)Remaining=Seconds;
}
void AExplosionActor::FinishSustained(float Seconds)
{
 bPersistent=false;Remaining=FMath::Max(.01f,Seconds);NiagaraEffect->Deactivate();ParticleEffect->DeactivateSystem();
}
void AExplosionActor::AdvancePresentation(double Seconds)
{
 if(!bPersistent){Remaining-=FMath::Max(0.0,Seconds);if(Remaining<=0)Destroy();}
}
TSubclassOf<AExplosionActor> AExplosionActor::NativeClass(EExplosionType Type)
{
 switch(Type){
 case EExplosionType::SHIELD_FLASH:return AShieldFlashExplosionActor::StaticClass();
 case EExplosionType::HULL_FLASH:return AHullFlashExplosionActor::StaticClass();
 case EExplosionType::BEAM_FLASH:return ABeamFlashExplosionActor::StaticClass();
 case EExplosionType::SHOT_BLAST:return AShotBlastExplosionActor::StaticClass();
 case EExplosionType::HULL_BURST:return AHullBurstExplosionActor::StaticClass();
 case EExplosionType::HULL_FIRE:return AHullFireExplosionActor::StaticClass();
 case EExplosionType::PLASMA_LEAK:return APlasmaLeakExplosionActor::StaticClass();
 case EExplosionType::SMOKE_TRAIL:return ASmokeTrailExplosionActor::StaticClass();
 case EExplosionType::SMALL_FIRE:return ASmallFireExplosionActor::StaticClass();
 case EExplosionType::SMALL_EXPLOSION:return ASmallExplosionActor::StaticClass();
 case EExplosionType::LARGE_EXPLOSION:return ALargeExplosionActor::StaticClass();
 case EExplosionType::LARGE_BURST:return ALargeBurstExplosionActor::StaticClass();
 case EExplosionType::NUKE_EXPLOSION:return ANukeExplosionActor::StaticClass();
 case EExplosionType::QUANTUM_FLASH:return AQuantumEffectsActor::StaticClass();
 case EExplosionType::HYPER_FLASH:return AHyperFlashExplosionActor::StaticClass();
 case EExplosionType::LARGE_EXP:return ALargeExplosionActor::StaticClass();
 default:return nullptr;
 }
}
TSubclassOf<AExplosionActor> AExplosionActor::VisualClass(EExplosionType Type)
{
 UClass* Native=NativeClass(Type).Get();if(!Native)return nullptr;
 const FString Name=Native->GetName().LeftChop(5);
 const FString Package=FString::Printf(TEXT("/Game/Effects/Explosions/%s/BP_%s"),*Name,*Name);
 if(FPackageName::DoesPackageExist(Package))
  if(UClass* BP=LoadClass<AExplosionActor>(nullptr,*(Package+TEXT(".BP_")+Name+TEXT("_C"))))return BP;
 return Native;
}
