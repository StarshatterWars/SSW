/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         StarshatterSystemDesignSubsystem.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Parses Content/GameData/Systems/sys.def (SYSTEM) and fills:
    - SystemsByName runtime cache
    - Optional SystemDesignDataTable via AddRow/RemoveRow
    - Optional legacy List<> catalog mirror
*/

#include "StarshatterSystemDesignSubsystem.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"

#include "Engine/DataTable.h"
#include "FormattingUtils.h"
#include "DataTableUtils.h"
#include "StarshatterAssetRegistrySubsystem.h"


void UStarshatterSystemDesignSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Log, TEXT("[SYSTEM DESIGN] Initialize()"));

 /*------------------------------------------------------------------
     Resolve System Design DataTable via Asset Registry
 ------------------------------------------------------------------*/

    UStarshatterAssetRegistrySubsystem* Assets =
        GetGameInstance()->GetSubsystem<UStarshatterAssetRegistrySubsystem>();

    if (!Assets)
    {
        UE_LOG(LogTemp, Error, TEXT("[SYSTEMDESIGN] Asset Registry missing"));
        return;
    }

    SystemDesignDataTable =
        Assets->GetDataTable(TEXT("Data.SystemDesignTable"), true);

    if (!SystemDesignDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("[SYSTEMDESIGN] SystemDesignTable not found"));
        return;
    }

    if (bClearTables)
    {
        SystemDesignDataTable->EmptyTable();
    }
}

void UStarshatterSystemDesignSubsystem::Deinitialize()
{
    UE_LOG(LogTemp, Log, TEXT("[SYSTEM DESIGN] Deinitialize()"));
    DesignsByName.Empty();
    Super::Deinitialize();
}

void UStarshatterSystemDesignSubsystem::LoadAll(bool bLoaded)
{
    UE_LOG(LogTemp, Log, TEXT("UStarshatterShipDesignSubsystem::LoadAll()"));
    if (!bLoaded)
        return;

    LoadSystemDesigns();
    bLoaded = true;
}

void UStarshatterSystemDesignSubsystem::LoadSystemDesigns()
{
    const FString SysDefPath = FPaths::ProjectContentDir() / TEXT("GameData/Systems/sys.def");
    const FTCHARToUTF8 Utf8(*SysDefPath);
    LoadSystemDesign(Utf8.Get());
}

void UStarshatterSystemDesignSubsystem::LoadSystemDesign(const char* Filename)
{
    if (!Filename || !*Filename)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SYSTEM] LoadSystemDesign: null/empty filename"));
        return;
    }

    const FString FilePath = ANSI_TO_TCHAR(Filename);
    UE_LOG(LogTemp, Log, TEXT("[SYSTEM] Loading System Designs '%s'"), *FilePath);

    if (!FPaths::FileExists(FilePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("[SYSTEM] file not found: %s"), *FilePath);
        return;
    }

    // ------------------------------------------------------------------
    // DataTable safety: ensure DT exists and row struct matches
    // ------------------------------------------------------------------
    if (SystemDesignDataTable)
    {
        const UScriptStruct* Actual = SystemDesignDataTable->GetRowStruct();
        const UScriptStruct* Expected = FSystemDesign::StaticStruct();

        if (!ensureMsgf(Actual == Expected,
            TEXT("[SYSTEM] DT row struct mismatch. Expected=%s Actual=%s  (DT=%s)"),
            *GetNameSafe(Expected), *GetNameSafe(Actual), *GetNameSafe(SystemDesignDataTable)))
        {
            // Disable DT writes to avoid crashes
            SystemDesignDataTable = nullptr;
        }
    }

    // Optional clear (cache always safe):
    if (bClearTables)
    {
        DesignsByName.Empty();
        UE_LOG(LogTemp, Log, TEXT("[SYSTEM] bClearTables=true: cleared in-memory cache"));
        // If you want to also clear DT in editor-only workflows, do it explicitly elsewhere.
    }

    // ------------------------------------------------------------------
    // Load file bytes for legacy parser
    // ------------------------------------------------------------------
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("[SYSTEM] failed to read: %s"), *FilePath);
        return;
    }
    Bytes.Add(0);

    // Stable UTF-8 filename for legacy helpers:
    const FTCHARToUTF8 Utf8Path(*FilePath);
    const char* fn = Utf8Path.Get();

    Parser ParserObj(new BlockReader(reinterpret_cast<const char*>(Bytes.GetData())));
    Term* TermObj = ParserObj.ParseTerm();

    if (!TermObj)
    {
        UE_LOG(LogTemp, Error, TEXT("[SYSTEM] ERROR: could not parse '%s'"), *FilePath);
        return;
    }

    // ------------------------------------------------------------------
    // Header check
    // ------------------------------------------------------------------
    {
        TermText* FileType = TermObj->isText();
        if (!FileType || FileType->value() != "SYSTEM")
        {
            UE_LOG(LogTemp, Error, TEXT("[SYSTEM] ERROR: invalid system design file '%s' (expected SYSTEM)"), *FilePath);
            delete TermObj;
            return;
        }
    }

    int32 ParsedSystems = 0;

    while (true)
    {
        delete TermObj;
        TermObj = ParserObj.ParseTerm();

        if (!TermObj)
            break;

        TermDef* Def = TermObj->isDef();
        if (!Def)
            continue;

        const Text& DefName = Def->name()->value();
        if (DefName != "system")
            continue;

        TermStruct* SystemStruct = (Def->term() ? Def->term()->isStruct() : nullptr);
        if (!SystemStruct)
        {
            UE_LOG(LogTemp, Warning, TEXT("[SYSTEM] WARNING: system structure missing in '%s'"), *FilePath);
            continue;
        }

        FSystemDesign NewSystem;
        NewSystem.SourceFile = FilePath;
        NewSystem.Components.Reset();

        Text SystemNameText = "";

        // Parse system { ... }
        for (int32 i = 0; i < (int32)SystemStruct->elements()->size(); ++i)
        {
            TermDef* ParamDef = SystemStruct->elements()->at(i)->isDef();
            if (!ParamDef)
                continue;

            const Text& Key = ParamDef->name()->value();

            if (Key == "name")
            {
                GetDefText(SystemNameText, ParamDef, fn);
                NewSystem.Name = FString(ANSI_TO_TCHAR(SystemNameText.data())).TrimStartAndEnd();
            }
            else if (Key == "component")
            {
                TermStruct* CompStruct = (ParamDef->term() ? ParamDef->term()->isStruct() : nullptr);
                if (!CompStruct)
                {
                    UE_LOG(LogTemp, Warning,
                        TEXT("[SYSTEM] WARNING: component struct missing in system '%s' in '%s'"),
                        *NewSystem.Name, *FilePath);
                    continue;
                }

                FComponentDesign NewComp;

                Text  CompNameText = "";
                Text  CompAbrvText = "";
                double RepairTime = 0.0;
                double ReplaceTime = 0.0;
                int   Spares = 0;
                int   Affects = 0;

                for (int32 j = 0; j < (int32)CompStruct->elements()->size(); ++j)
                {
                    TermDef* CDef = CompStruct->elements()->at(j)->isDef();
                    if (!CDef)
                        continue;

                    const Text& CKey = CDef->name()->value();

                    if (CKey == "name")
                    {
                        GetDefText(CompNameText, CDef, fn);
                        NewComp.Name = FString(ANSI_TO_TCHAR(CompNameText.data())).TrimStartAndEnd();
                    }
                    else if (CKey == "abrv")
                    {
                        GetDefText(CompAbrvText, CDef, fn);
                        NewComp.Abbrev = FString(ANSI_TO_TCHAR(CompAbrvText.data())).TrimStartAndEnd();
                    }
                    else if (CKey == "repair_time")
                    {
                        GetDefNumber(RepairTime, CDef, fn);
                        NewComp.RepairTime = (float)RepairTime;
                    }
                    else if (CKey == "replace_time")
                    {
                        GetDefNumber(ReplaceTime, CDef, fn);
                        NewComp.ReplaceTime = (float)ReplaceTime;
                    }
                    else if (CKey == "spares")
                    {
                        GetDefNumber(Spares, CDef, fn);
                        NewComp.Spares = (int32)Spares;
                    }
                    else if (CKey == "affects")
                    {
                        GetDefNumber(Affects, CDef, fn);
                        NewComp.Affects = (int32)Affects;
                    }
                }

                // Optional: skip empty components
                if (!NewComp.Name.IsEmpty())
                {
                    NewSystem.Components.Add(NewComp);
                }
            }
        }

        if (NewSystem.Name.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("[SYSTEM] WARNING: system with missing name ignored in '%s'"), *FilePath);
            continue;
        }

        const FName RowName(*NewSystem.Name);

        // Cache (authoritative)
        DesignsByName.Add(RowName, NewSystem);

        // DT write (optional)
        if (SystemDesignDataTable)
        {
            if (FSystemDesign* Existing = SystemDesignDataTable->FindRow<FSystemDesign>(RowName, TEXT("LoadSystemDesign"), false))
            {
                *Existing = NewSystem;   // overwrite row data in place
            }
            else
            {
                SystemDesignDataTable->AddRow(RowName, NewSystem);
            }
        }

        ++ParsedSystems;
    }

    delete TermObj;

    UE_LOG(LogTemp, Log, TEXT("[SYSTEM] Loaded %d system designs (cache=%d) from '%s'"),
        ParsedSystems, DesignsByName.Num(), *FilePath);
}
