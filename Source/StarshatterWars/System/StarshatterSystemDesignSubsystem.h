/*=============================================================================
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025–2026.

    SUBSYSTEM:    StarshatterSystemDesignSubsystem (Unreal Engine)
    FILE:         StarshatterSystemDesignSubsystem.h / .cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Authoritative SYSTEM design ingestion subsystem (sys.def).

    - Parses Content/GameData/Systems/sys.def (legacy "SYSTEM" file)
    - Extracts:
        system { name: "..." component { ... } component { ... } }
    - Emits Unreal-native:
        FSystemDesign (Name, Components[], SourceFile)
        FComponentDesign (Name, Abbrev, RepairTime, ReplaceTime, Spares, Affects)

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

#include "DataLoader.h"
#include "ParseUtil.h"
#include "Random.h"
#include "FormatUtil.h"
#include "Text.h"
#include "Term.h"
#include "List.h"
#include "GameLoader.h"

#include "GameStructs_System.h"
#include "GameStructs.h"

#include "StarshatterSystemDesignSubsystem.generated.h"

UCLASS()
class STARSHATTERWARS_API UStarshatterSystemDesignSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Convenience wrapper for the one legacy file:
    UFUNCTION(BlueprintCallable, Category = "Starshatter|SystemDesign")
    void LoadSystemDesigns();

    // Monolithic parse method (legacy-style):
    void LoadSystemDesign(const char* Filename);
    void LoadAll(bool bFull = false);
    bool                 bClearTables;

    // Authoritative cache
    const TMap<FName, FSystemDesign>& GetDesignsByName() const { return DesignsByName; }
    const FSystemDesign* FindDesign(const FName Name) const { return DesignsByName.Find(Name); }

public:
    // Target DT (optional; can be null if you only want in-memory cache)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Starshatter|SystemDesign")
    UDataTable* SystemDesignDataTable = nullptr;

    TArray<FSystemDesign> NewSystemArray;
    TArray<FComponentDesign> NewComponentArray;

private:
    UPROPERTY()
    TMap<FName, FSystemDesign> DesignsByName;

  
};
