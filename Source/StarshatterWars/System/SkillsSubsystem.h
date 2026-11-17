#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameStructs_skills.h"
#include "GameStructs_economy.h"
#include "SkillsSubsystem.generated.h"

class FSkillRules
{
public:
    static bool CanPurchase(const FSkillDefinition&, const TMap<FName, FSkillProgress>&, int32 Rank, FName Empire, FText& Reason);
    static bool Purchase(FEconomyState&, TMap<FName, FSkillProgress>&, const FSkillDefinition&, int32 Rank, FName Empire, int64 Now, FText& Reason);
};
UCLASS()
class STARSHATTERWARS_API USkillsSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    void Initialize(FSubsystemCollectionBase& Collection) override;
    bool LoadCatalog();
    const TMap<FName, FSkillDefinition>& GetCatalog() const { return Definitions; }
    bool HasSkill(FName Id, int32 RequiredLevel = 1) const;
    bool CanLearn(FName Id, FText& Reason) const;
    static FName RequiredHullSkill(uint32 Classification);
    bool CanPurchaseBook(FName Id, FText& Reason) const;
    bool PurchaseBook(FName Id, FText& Reason);
private:
    TMap<FName, FSkillDefinition> Definitions;
};
