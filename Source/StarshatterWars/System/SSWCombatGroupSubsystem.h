#pragma once
/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:      Combat Group Data
    FILE:           SSWCombatGroupSubsystem.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    Static combat-group ingestion and DataTable read subsystem.

    PHASE 1 GOAL:
    Copy the existing combat-group ingestion / table-read logic out of
    UStarshatterGameDataSubsystem with minimal behavioral change.

    THIS SUBSYSTEM SHOULD OWN:
    - CombatGroupDataTable
    - CombatRosterData / static combat-group row cache
    - combat-group table ingestion
    - combat-group table reads
    - combat-group lookup/index helpers

    THIS SUBSYSTEM SHOULD NOT YET OWN:
    - campaign combatants
    - OOB runtime assembly
    - runtime CombatGroup* trees
    - save/load overlays
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameStructs.h"

// Legacy parsing / registry headers (kept for now)
#include "DataLoader.h"
#include "ParseUtil.h"
#include "Random.h"
#include "FormatUtil.h"
#include "Text.h"
#include "Term.h"

#include "SSWCombatGroupSubsystem.generated.h"

class UDataTable;
class UStarshatterAssetRegistrySubsystem;
class USSWGameInstance;

USTRUCT()
struct FSSWCombatGroupKey
{
    GENERATED_BODY()

    UPROPERTY()
    EEMPIRE_NAME EmpireId = EEMPIRE_NAME::Unknown;

    UPROPERTY()
    ECOMBATGROUP_TYPE Type = ECOMBATGROUP_TYPE::NONE;

    UPROPERTY()
    int32 Id = 0;

    FSSWCombatGroupKey() = default;

    FSSWCombatGroupKey(EEMPIRE_NAME InEmpireId, ECOMBATGROUP_TYPE InType, int32 InId)
        : EmpireId(InEmpireId)
        , Type(InType)
        , Id(InId)
    {
    }

    bool operator==(const FSSWCombatGroupKey& Other) const
    {
        return EmpireId == Other.EmpireId
            && Type == Other.Type
            && Id == Other.Id;
    }
};

FORCEINLINE uint32 GetTypeHash(const FSSWCombatGroupKey& Key)
{
    uint32 Hash = ::GetTypeHash((int32)Key.EmpireId);
    Hash = HashCombine(Hash, ::GetTypeHash((int32)Key.Type));
    Hash = HashCombine(Hash, ::GetTypeHash(Key.Id));
    return Hash;
}

UCLASS()
class STARSHATTERWARS_API USSWCombatGroupSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // =====================================================================
    // Subsystem lifecycle
    // =====================================================================
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void GetSSWInstance();

    bool ResolveCombatGroupDataTable();

    // =====================================================================
    // Primary entry point
    // =====================================================================
    void LoadAll(bool bFull = false);

    // =====================================================================
    // Project path / utility
    // =====================================================================
    void SetProjectPath();
    FString GetProjectPath();

    // =====================================================================
    // Combat groups (copy from GameData first)
    // =====================================================================
    void InitializeCombatRoster();
    void LoadCombatRoster(const char* InFilename, int32 Team);
    void ReadCombatRosterData();

    // =====================================================================
    // Queries
    // =====================================================================
    UDataTable* GetCombatGroupDataTable() const { return CombatGroupDataTable; }
    const TArray<FS_CombatGroup>& GetCombatRosterData() const { return CombatRosterData; }

    const FS_CombatGroup* FindCombatGroupByRowName(FName RowName) const;
    const FS_CombatGroup* FindCombatGroupByKey(EEMPIRE_NAME EmpireId, ECOMBATGROUP_TYPE Type, int32 Id) const;
    const FS_CombatGroup* FindCombatGroupByName(const FString& GroupName) const;

private:
    void BuildIndexes();
    void ClearCombatGroupData();

private:
    // Cached GI (same pattern as GameData)
    USSWGameInstance* SSWInstance = nullptr;

    // Paths
    FString ProjectPath;
    FString FilePath;

    // Static combat-group table
    UPROPERTY(Transient)
    TObjectPtr<UDataTable> CombatGroupDataTable = nullptr;

    // Static combat-group cache
    UPROPERTY()
    TArray<FS_CombatGroup> CombatRosterData;

    UPROPERTY()
    TMap<FName, FS_CombatGroup> CombatGroupMapByRow;

    UPROPERTY()
    TMap<FString, FName> CombatGroupRowByName;

    TMap<FSSWCombatGroupKey, FName> CombatGroupRowByKey;
};