#include "BoltVisualPoolSubsystem.h"
#include "BoltWeaponActor.h"
#include "Engine/World.h"
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "HAL/IConsoleManager.h"
namespace
{
TAutoConsoleVariable<int32> CVarBoltVisualPooling(TEXT("ssw.BoltVisualPooling"),1,
    TEXT("Reuse dormant client bolt visuals. Native shots and damage remain unchanged."));
}
bool UBoltVisualPoolSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    return !IsRunningDedicatedServer() && Super::ShouldCreateSubsystem(Outer);
}
ABoltWeaponActor* UBoltVisualPoolSubsystem::Acquire(TSubclassOf<ABoltWeaponActor> Class,const FVector& Position,const FRotator& Rotation)
{
    auto* World=GetWorld();if(!World || !Class) return nullptr;
    ABoltWeaponActor* Actor=nullptr;
    if(CVarBoltVisualPooling.GetValueOnGameThread()!=0)
        if(auto* Bucket=Buckets.Find(Class.Get()))
            while(!Bucket->Idle.IsEmpty() && !Actor)
            {
                auto* Candidate=Bucket->Idle.Pop(EAllowShrinking::No).Get();
                IdleActors.Remove(Candidate);--IdleCount;
                if(IsValid(Candidate) && Candidate->GetWorld()==World) Actor=Candidate;
            }
    if(Actor) ++ReuseCount;
    else
    {
        FActorSpawnParameters Spawn;
        Spawn.ObjectFlags|=RF_Transient;
        Spawn.OverrideLevel=World->PersistentLevel;
        Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Actor=World->SpawnActor<ABoltWeaponActor>(Class,Position,Rotation,Spawn);
        if(Actor) ++SpawnCount;
    }
    if(Actor)
    {
        Actor->SetLifeSpan(0);Actor->SetActorEnableCollision(false);
        Actor->SetActorLocationAndRotation(Position,Rotation,false,nullptr,ETeleportType::TeleportPhysics);
        // BeginSimulationFlight remains the caller's activation boundary.
    }
    return Actor;
}
void UBoltVisualPoolSubsystem::Release(ABoltWeaponActor* Actor)
{
    if(!IsValid(Actor) || IdleActors.Contains(Actor)) return;
    auto* World=GetWorld();
    if(!World || World->bIsTearingDown || Actor->GetWorld()!=World || CVarBoltVisualPooling.GetValueOnGameThread()==0)
    {Actor->Destroy();return;}
    auto& Bucket=Buckets.FindOrAdd(Actor->GetClass());
    if(IdleCount>=MaximumIdleTotal || Bucket.Idle.Num()>=MaximumIdlePerClass)
    {Actor->Destroy();return;}
    Actor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Actor->SetLifeSpan(0);Actor->SetActorTickEnabled(false);
    Actor->SetActorEnableCollision(false);Actor->SetActorHiddenInGame(true);
    Actor->SetOwner(nullptr);Actor->SetInstigator(nullptr);
    TArray<UNiagaraComponent*> Effects;Actor->GetComponents(Effects);
    for(auto* Effect:Effects) if(Effect) Effect->DeactivateImmediate();
    TArray<UAudioComponent*> Audio;Actor->GetComponents(Audio);
    for(auto* Sound:Audio) if(Sound) Sound->Stop();
    Bucket.Idle.Add(Actor);IdleActors.Add(Actor);++IdleCount;
}
void UBoltVisualPoolSubsystem::Deinitialize()
{
    for(auto& Pair:Buckets)
        for(const auto& Pointer:Pair.Value.Idle) if(auto* Actor=Pointer.Get();IsValid(Actor)) Actor->Destroy();
    Buckets.Empty();IdleActors.Empty();IdleCount=0;
    Super::Deinitialize();
}
