#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "MiningDroneAI.h"
#include "MiningDroneSubsystem.generated.h"
class Ship;
class Asteroid;
UCLASS()
class STARSHATTERWARS_API UMiningDroneSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    void Initialize(FSubsystemCollectionBase& Collection) override;
    void Deinitialize() override;
    void Tick(float DeltaTime) override;
    TStatId GetStatId() const override {RETURN_QUICK_DECLARE_CYCLE_STAT(UMiningDroneSubsystem,STATGROUP_Tickables);}
    bool IsTickable() const override {return !IsTemplate();}
    UWorld* GetTickableGameObjectWorld() const override {return GetWorld();}
    bool HasBay() const;
    bool PrepareBay(FText&);
    bool LoadCatalog();
    const TArray<FMiningDroneState>& GetDrones() const;
    bool Launch(FGuid,FText&);
    bool Mine(FGuid,FText&);
    bool MineRepeatedly(FGuid,FText&);
    bool Stop(FGuid,FText&);
    bool Return(FGuid,FText&);
    bool Dock(FGuid,FText&);
    bool CanLoadSpare(FText&) const;
    bool LoadSpare(FText&);
    FText GetStatusText() const {return LastStatus;}
private:
    TMap<FName,FMiningDroneBayDefinition> Bays;
    TMap<FGuid,TWeakObjectPtr<class AMiningDroneActor>> Visuals;
    TSet<FName> BoundFleets;
    FText LastStatus;
    double Accumulator=0;
    const FMiningDroneBayDefinition* GetBay(const Ship*) const;
    bool Authority(FText&) const;
    bool Command(FGuid,int32,FText&);
    FMiningDroneFrame Frame(Ship&,int32 Target,const FMiningDroneBayDefinition&,Asteroid*&) const;
    double Unload(Ship&,FName,double);
    void UpdateVisuals(Ship&,const FMiningDroneFleetState&);
};
