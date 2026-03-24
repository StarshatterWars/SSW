/*=============================================================================
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025–2026.

    SUBSYSTEM:    StarshatterWeaponDesignSubsystem (Unreal Engine)
    FILE:         StarshatterWeaponDesignSubsystem.h / .cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Authoritative WEAPON design ingestion subsystem (wep.def).

    - Parses Content/GameData/Weapons/wep.def (legacy "WEAPON" file)
    - Extracts:
        primary { ... }
        missile { ... }
        drone   { ... }
        beam    { ... }

    - Emits Unreal-native:
        FWeaponDesign rows (DataTable + in-memory cache)

    WRITE-ON-INGEST ONLY:
    - This runs only when data changes / you re-import.
    - Runtime should read from the DataTable / cache; no parsing during gameplay.

    NO EDITOR-ONLY DEPENDENCIES:
    - Uses only UDataTable::RemoveRow / AddRow (no FDataTableEditorUtils, no GEditor)

=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

// Legacy parser deps:
#include "ParseUtil.h"
#include "Text.h"
#include "Term.h"

// Your structs:
#include "GameStructs.h"
#include "GameStructs_System.h"

#include "StarshatterWeaponDesignSubsystem.generated.h"

UCLASS()
class STARSHATTERWARS_API UStarshatterWeaponDesignSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Convenience wrapper for the one legacy file:
    UFUNCTION(BlueprintCallable, Category = "Starshatter|WeaponDesign")
    void LoadWeaponDesigns();

    // Monolithic parse method (legacy-style):
    void LoadWeaponDesign(const char* Filename);
    void LoadAll(bool bFull = false);

    // Authoritative cache
    const TMap<FName, FWeaponDesign>& GetDesignsByName() const { return DesignsByName; }
    const FWeaponDesign* FindDesign(const FName Name) const { return DesignsByName.Find(Name); }

public:
    // Target DT (optional; can be null if you only want in-memory cache)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Starshatter|WeaponDesign")
    UDataTable* WeaponDesignDataTable = nullptr;

    // If true, clear in-memory cache (DT rows overwritten as parsed)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Starshatter|WeaponDesign")
    bool bClearTables = false;

private:
    UPROPERTY()
    TMap<FName, FWeaponDesign> DesignsByName;
};
