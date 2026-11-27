#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "GameStructs_skills.h"
#include "GameStructs_economy.h"
#include "SkillCommandAI.h"
#include "SkillsSubsystem.generated.h"

class FSkillRules
{
public:
    static bool CanPurchase(const FSkillDefinition&, const TMap<FName, FSkillProgress>&, int32 Rank, FName Empire, FText& Reason);
    static bool Purchase(FEconomyState&, TMap<FName, FSkillProgress>&, const FSkillDefinition&, int32 Rank, FName Empire, int64 Now, FText& Reason);
};
class FSkillTrainingRules
{
public:
    static int64 Duration(int32 Level);
    static int64 InstantCost(const FSkillTrainingEntry&,int64 Now);
    static bool InstantTrain(FEconomyState&,const TMap<FName,FSkillDefinition>&,TMap<FName,FSkillProgress>&,TArray<FSkillTrainingEntry>&,FGuid Entry,int64 Now,FText& Reason);
    static int32 Level(const FSkillProgress*,int32 Maximum=10);
    static bool CanQueue(const TMap<FName,FSkillDefinition>&,const TMap<FName,FSkillProgress>&,const TArray<FSkillTrainingEntry>&,FName Skill,int32 Rank,FName Empire,FText& Reason);
    static bool Queue(const TMap<FName,FSkillDefinition>&,TMap<FName,FSkillProgress>&,TArray<FSkillTrainingEntry>&,FName Skill,int32 Rank,FName Empire,int64 Now,FText& Reason);
    static bool Advance(const TMap<FName,FSkillDefinition>&,TMap<FName,FSkillProgress>&,TArray<FSkillTrainingEntry>&,int64 Now);
    static bool Cancel(TMap<FName,FSkillProgress>&,TArray<FSkillTrainingEntry>&,FGuid Entry,int64 Now);
};
UCLASS()
class STARSHATTERWARS_API USkillsSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    void Initialize(FSubsystemCollectionBase& Collection) override;
    bool LoadCatalog();
    const TMap<FName, FSkillDefinition>& GetCatalog() const { return Definitions; }
    void Tick(float DeltaTime) override;
    bool IsTickable() const override { return !IsTemplate() && GetGameInstance()!=nullptr; }
    UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(USkillsSubsystem,STATGROUP_Tickables); }
    int32 GetLevel(FName Id) const;
    bool CanQueueTraining(FName Id,FText& Reason) const;
    bool QueueTraining(FName Id,FText& Reason);
    int32 TrainingCap(FName Id) const;
    bool QueueToLevel(FName Id,int32 Target,FText& Reason,TFunction<void(bool,const FText&)> Completed={});
    bool CancelTraining(FGuid Entry,FText& Reason,TFunction<void(bool,const FText&)> Completed={});
    bool InstantTrain(FGuid Entry,FText& Reason,TFunction<void(bool,const FText&)> Completed={});
    TConstArrayView<FSkillTrainingEntry> GetQueue() const;
    FText TrainingStatus(FName Id) const;
    bool HasSkill(FName Id, int32 RequiredLevel = 1) const;
    bool CanLearn(FName Id, FText& Reason) const;
    static FName RequiredHullSkill(uint32 Classification);
    bool CanPurchaseBook(FName Id, FText& Reason) const;
    bool PurchaseBook(FName Id, FText& Reason,TFunction<void(bool,const FText&)> Completed={});
    bool CanPurchaseAdvancedBook(FName Id,FText& Reason) const;
    bool PurchaseAdvancedBook(FName Id,FText& Reason,TFunction<void(bool,const FText&)> Completed={});
private:
    bool RequestAction(ESkillCommand,FName,FGuid,int32,TFunction<void(bool,const FText&)>&&,FText&);
    bool bActionPending=false;
    uint64 CatalogRevision=0;
    double LastTrainingCheck=0;
    TMap<FName, FSkillDefinition> Definitions;
};
