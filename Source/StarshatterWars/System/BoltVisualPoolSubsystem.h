#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BoltVisualPoolSubsystem.generated.h"
class ABoltWeaponActor;
USTRUCT()
struct FBoltVisualPoolBucket
{
    GENERATED_BODY()
    UPROPERTY() TArray<TObjectPtr<ABoltWeaponActor>> Idle;
};
/** Presentation only. Native SimShot ownership and damage never enter the pool. */
UCLASS()
class STARSHATTERWARS_API UBoltVisualPoolSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Deinitialize() override;
    ABoltWeaponActor* Acquire(TSubclassOf<ABoltWeaponActor> Class,const FVector& Position,const FRotator& Rotation);
    void Release(ABoltWeaponActor* Actor);
    int32 GetIdleCount() const{return IdleCount;}
    uint64 GetSpawnCount() const{return SpawnCount;}
    uint64 GetReuseCount() const{return ReuseCount;}
private:
    UPROPERTY(Transient) TMap<TObjectPtr<UClass>,FBoltVisualPoolBucket> Buckets;
    UPROPERTY(Transient) TSet<TObjectPtr<ABoltWeaponActor>> IdleActors;
    int32 IdleCount=0;
    uint64 SpawnCount=0,ReuseCount=0;
    static constexpr int32 MaximumIdlePerClass=32;
    static constexpr int32 MaximumIdleTotal=256;
};
