/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         StarshatterFormSubsystem.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Loads and parses legacy Starshatter .frm UI definitions into Unreal-native
    structs (FS_FormDesign, FS_UIControlDef, FS_LayoutDef).

    This subsystem performs NO widget creation and NO rendering.
*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "GameStructs_UI.h"

#include "StarshatterFormSubsystem.generated.h"

class UDataTable;

UCLASS()
class STARSHATTERWARS_API UStarshatterFormSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Boot entrypoint for BootSubsystem (public on purpose)
    void BootLoadForms();

    // Public read API (optional but useful)
    const FS_FormDesign* FindFormByName(const FName& FormName) const;

    // Public load (BootLoadForms calls this)
    void LoadForms();

    UFUNCTION(CallInEditor)
    void RebuildFormDefDataTable_EditorOnly();

private:
    // Private per-file loader
    void LoadForm(const char* InFilename);

private:
    // Authoritative parsed forms cache
    TMap<FName, FS_FormDesign> FormsByName;

    // Optional: DT destination (if you want to write rows)
    UDataTable* FormDefDataTable = nullptr;

    // Guard so LoadForms is idempotent if you want it:
    bool bFormsLoaded = false;
};
