/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.
    All Rights Reserved.

    FILE:         StarshatterDesignSubsystem.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UStarshatterDesignSubsystem

    Unreal-native design registry for ship/system/weapon catalogs.

    Responsibilities:
    - DataTable row lookup (by RowName, by Name field)
    - Listing / filtering / existence checks
    - Optional: indexing caches (Name->RowName) for fast lookups

    Non-responsibilities:
    - DEF parsing (ingestion happens elsewhere; this is a registry)
    - Asset loading / content streaming
    - Runtime simulation behavior
*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"

#include "GameStructs_System.h" // FWeaponDesign, FSystemDesign, FShipDesign
#include "StarshatterDesignSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogStarshatterDesign, Log, All);

UCLASS()
class STARSHATTERWARS_API UStarshatterDesignSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UStarshatterDesignSubsystem();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ---------------------------------------------------------------------
    // Table registration (call from your boot / content init)
    // ---------------------------------------------------------------------
    void SetWeaponTable(UDataTable* InTable);
    void SetSystemTable(UDataTable* InTable);
    void SetShipTable(UDataTable* InTable);

    UDataTable* GetWeaponTable() const { return WeaponTable; }
    UDataTable* GetSystemTable() const { return SystemTable; }
    UDataTable* GetShipTable()   const { return ShipTable; }

    // ---------------------------------------------------------------------
    // Weapon designs
    // ---------------------------------------------------------------------
    const FWeaponDesign* GetWeaponByRow(FName RowName) const;
    const FWeaponDesign* FindWeaponByName(const FString& Name) const;
    bool HasWeaponRow(FName RowName) const;

    void GetWeaponRowNames(TArray<FName>& OutRows) const;
    void GetWeapons(TArray<const FWeaponDesign*>& OutWeapons) const;

    // ---------------------------------------------------------------------
    // System designs
    // ---------------------------------------------------------------------
    const FSystemDesign* GetSystemByRow(FName RowName) const;
    const FSystemDesign* FindSystemByName(const FString& Name) const;
    bool HasSystemRow(FName RowName) const;

    void GetSystemRowNames(TArray<FName>& OutRows) const;
    void GetSystems(TArray<const FSystemDesign*>& OutSystems) const;

    // ---------------------------------------------------------------------
    // Ship designs
    // ---------------------------------------------------------------------
    const FShipDesign* GetShipByRow(FName RowName) const;
    const FShipDesign* FindShipByName(const FString& Name) const;
    bool HasShipRow(FName RowName) const;

    void GetShipRowNames(TArray<FName>& OutRows) const;
    void GetShips(TArray<const FShipDesign*>& OutShips) const;

    void GetShipsByClass(const FString& ShipClass, TArray<const FShipDesign*>& OutShips) const;

    // ---------------------------------------------------------------------
    // Optional: build caches for fast lookups
    // ---------------------------------------------------------------------
    void RebuildAllIndexes();

private:
    UPROPERTY()
    UDataTable* WeaponTable = nullptr;

    UPROPERTY()
    UDataTable* SystemTable = nullptr;

    UPROPERTY()
    UDataTable* ShipTable = nullptr;

    UPROPERTY()
    TMap<FString, FName> WeaponNameIndex;

    UPROPERTY()
    TMap<FString, FName> SystemNameIndex;

    UPROPERTY()
    TMap<FString, FName> ShipNameIndex;

private:
    void RebuildWeaponIndex();
    void RebuildSystemIndex();
    void RebuildShipIndex();

    static FString NormalizeKey(const FString& In);
};
