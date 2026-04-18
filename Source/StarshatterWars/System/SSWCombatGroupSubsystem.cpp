/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:      Combat Group Data
    FILE:           SSWCombatGroupSubsystem.cpp
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    Static combat-group ingestion and DataTable read subsystem.

    PHASE 1:
    Copy the old GameData combat-group methods first with minimal changes.
=============================================================================*/

#include "SSWCombatGroupSubsystem.h"

#include "StarshatterAssetRegistrySubsystem.h"
#include "SSWGameInstance.h"
#include "GameStructs.h"

#include "Engine/DataTable.h"
#include "FormattingUtils.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#include "Logging/LogMacros.h"
#include "CombatGroupRegistry.h"


// -----------------------------------------------------------------------------
// Local helpers
// -----------------------------------------------------------------------------

template<typename TEnum>
static bool SSWStringToEnum(const FString& InText, TEnum& OutValue, bool bLogFailure = false)
{
    const UEnum* EnumObj = StaticEnum<TEnum>();
    if (!EnumObj)
    {
        return false;
    }

    FString Candidate = InText;
    Candidate.TrimStartAndEndInline();

    for (int32 Index = 0; Index < EnumObj->NumEnums(); ++Index)
    {
        const FString ShortName = EnumObj->GetNameStringByIndex(Index);
        if (ShortName.Equals(Candidate, ESearchCase::IgnoreCase))
        {
            OutValue = static_cast<TEnum>(EnumObj->GetValueByIndex(Index));
            return true;
        }
    }

    for (int32 Index = 0; Index < EnumObj->NumEnums(); ++Index)
    {
        const FString FullName = EnumObj->GetNameByIndex(Index).ToString();
        if (FullName.Equals(Candidate, ESearchCase::IgnoreCase))
        {
            OutValue = static_cast<TEnum>(EnumObj->GetValueByIndex(Index));
            return true;
        }
    }

    if (bLogFailure)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CombatGroupSubsystem] Failed enum parse '%s'"), *InText);
    }

    return false;
}

static FString NormalizeCombatGroupTypeToken(FString InToken)
{
    InToken.TrimStartAndEndInline();
    InToken.ReplaceInline(TEXT("-"), TEXT("_"));
    InToken.ReplaceInline(TEXT(" "), TEXT("_"));
    InToken = InToken.ToUpper();

    if (InToken == TEXT("CARRIERGROUP"))
    {
        return TEXT("CARRIER_GROUP");
    }
    else if (InToken == TEXT("BATTLEGROUP"))
    {
        return TEXT("BATTLE_GROUP");
    }
    else if (InToken == TEXT("DESTROYERSQUADRON"))
    {
        return TEXT("DESTROYER_SQUADRON");
    }
    else if (InToken == TEXT("FIGHTERSQUADRON"))
    {
        return TEXT("FIGHTER_SQUADRON");
    }

    return InToken;
}

// -----------------------------------------------------------------------------
// Subsystem lifecycle
// -----------------------------------------------------------------------------

void USSWCombatGroupSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogTemp, Error, TEXT("[CombatGroupSubsystem] Initialize: GameInstance is null"));
        return;
    }

    GetSSWInstance();
    SetProjectPath();
    ResolveCombatGroupDataTable();
}

void USSWCombatGroupSubsystem::Deinitialize()
{
    ClearCombatGroupData();
    Super::Deinitialize();
}

void USSWCombatGroupSubsystem::GetSSWInstance()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        SSWInstance = Cast<USSWGameInstance>(GI);
    }
}

bool USSWCombatGroupSubsystem::ResolveCombatGroupDataTable()
{
    if (CombatGroupDataTable)
    {
        return true;
    }

    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogTemp, Error, TEXT("[CombatGroupSubsystem] ResolveCombatGroupDataTable: GameInstance is null"));
        return false;
    }

    UStarshatterAssetRegistrySubsystem* AssetRegistry =
        GI->GetSubsystem<UStarshatterAssetRegistrySubsystem>();

    if (!AssetRegistry)
    {
        UE_LOG(LogTemp, Error, TEXT("[CombatGroupSubsystem] ResolveCombatGroupDataTable: AssetRegistry subsystem missing"));
        return false;
    }

    UE_LOG(LogTemp, Log,
        TEXT("[CombatGroupSubsystem] Requesting DataTable key: Data.CombatGroupTable"));

    CombatGroupDataTable = AssetRegistry->GetDataTable(TEXT("Data.CombatGroupTable"), true);

    if (!CombatGroupDataTable)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CombatGroupSubsystem] ResolveCombatGroupDataTable: Data.CombatGroupTable not found"));
        return false;
    }

    UE_LOG(LogTemp, Log,
        TEXT("[CombatGroupSubsystem] ResolveCombatGroupDataTable: %s"),
        *GetNameSafe(CombatGroupDataTable));

    return true;
}

void USSWCombatGroupSubsystem::LoadAll(bool bFull /*= false*/)
{
    UE_LOG(LogTemp, Log, TEXT("[CombatGroupSubsystem] LoadAll (Full=%s)"),
        bFull ? TEXT("true") : TEXT("false"));

    if (!ResolveCombatGroupDataTable())
    {
        UE_LOG(LogTemp, Error, TEXT("[CombatGroupSubsystem] LoadAll: Failed to resolve CombatGroupDataTable"));
        return;
    }

    InitializeCombatRoster();
    //ReadCombatRosterData();

    const int32 TableRows = CombatGroupDataTable ? CombatGroupDataTable->GetRowNames().Num() : -1;
    const int32 CacheRows = CombatRosterData.Num();

    FString FirstRowName = TEXT("None");
    if (CombatGroupDataTable)
    {
        const TArray<FName> RowNames = CombatGroupDataTable->GetRowNames();
        if (RowNames.Num() > 0)
        {
            FirstRowName = RowNames[0].ToString();
        }
    }

    FString FirstCacheName = TEXT("None");
    if (CombatRosterData.Num() > 0)
    {
        FirstCacheName = CombatRosterData[0].Name;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[DBG CGS] After LoadAll: Table=%s TableRows=%d CacheRows=%d FirstTableRow=%s FirstCacheGroup=%s"),
        *GetNameSafe(CombatGroupDataTable),
        TableRows,
        CacheRows,
        *FirstRowName,
        *FirstCacheName);
}

// -----------------------------------------------------------------------------
// Paths
// -----------------------------------------------------------------------------

void USSWCombatGroupSubsystem::SetProjectPath()
{
    ProjectPath = FPaths::ProjectDir();
    ProjectPath.Append(TEXT("GameData/"));

    UE_LOG(LogTemp, Log, TEXT("Setting Game Data Directory %s"), *ProjectPath);
}

FString USSWCombatGroupSubsystem::GetProjectPath()
{
    return ProjectPath;
}

// -----------------------------------------------------------------------------
// Phase 1 combat-group ingestion
// -----------------------------------------------------------------------------

void USSWCombatGroupSubsystem::InitializeCombatRoster()
{
    UE_LOG(LogTemp, Log, TEXT("USSWCombatGroupSubsystem::InitializeCombatRoster()"));

    if (!ResolveCombatGroupDataTable())
    {
        UE_LOG(LogTemp, Error, TEXT("[CombatGroupSubsystem] InitializeCombatRoster: CombatGroupDataTable unresolved"));
        return;
    }

    CombatGroupDataTable->EmptyTable();

    ProjectPath = FPaths::ProjectContentDir();
    ProjectPath /= TEXT("GameData/Campaigns/");

    TArray<FString> Files;
    Files.Empty();

    const FString Wildcard = ProjectPath / TEXT("*.def");
    IFileManager::Get().FindFiles(Files, *Wildcard, true, false);

    UE_LOG(LogTemp, Warning,
        TEXT("[LoadCombatRoster] Found %d .def files in %s"),
        Files.Num(),
        *ProjectPath);

    for (const FString& File : Files)
    {
        const FString FullPath = ProjectPath / File;
        const FTCHARToUTF8 Utf8Path(*FullPath);
        LoadCombatRoster(Utf8Path.Get(), -1);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[LoadCombatRoster] DT row count after ingest: %d"),
        CombatGroupDataTable ? CombatGroupDataTable->GetRowNames().Num() : 0);
}

void USSWCombatGroupSubsystem::LoadCombatRoster(const char* InFilename, int32 Team)
{
    UE_LOG(LogTemp, Log, TEXT("USSWCombatGroupSubsystem::LoadCombatRoster"));

    if (!InFilename || !*InFilename)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LoadCombatRoster] null/empty filename"));
        return;
    }

    if (!ResolveCombatGroupDataTable())
    {
        UE_LOG(LogTemp, Error, TEXT("[LoadCombatRoster] CombatGroupDataTable is null"));
        return;
    }

    const FString OobFilePath = ANSI_TO_TCHAR(InFilename);
    UE_LOG(LogTemp, Log, TEXT("[LoadCombatRoster] Loading Data: %s"), *OobFilePath);

    if (!FPaths::FileExists(OobFilePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("[LoadCombatRoster] file not found: %s"), *OobFilePath);
        return;
    }

    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *OobFilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("[LoadCombatRoster] failed to read: %s"), *OobFilePath);
        return;
    }

    Bytes.Add(0);

    const FTCHARToUTF8 Utf8Path(*OobFilePath);
    const char* fn = Utf8Path.Get();

    Parser ParserObj(new BlockReader(reinterpret_cast<const char*>(Bytes.GetData())));
    Term* TermPtr = ParserObj.ParseTerm();

    if (!TermPtr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LoadCombatRoster] could not parse: %s"), *OobFilePath);
        return;
    }

    {
        TermText* FileType = TermPtr->isText();
        if (!FileType || FileType->value() != "ORDER_OF_BATTLE")
        {
            UE_LOG(LogTemp, Warning, TEXT("[LoadCombatRoster] Invalid Order of Battle File: %s"), *OobFilePath);
            delete TermPtr;
            return;
        }
    }

    delete TermPtr;
    TermPtr = nullptr;

    int32 ParsedGroupCount = 0;
    int32 AddedGroupCount = 0;
    int32 SkippedInvalidStructCount = 0;
    int32 SkippedTeamFilterCount = 0;
    int32 OverwrittenRowCount = 0;

    while ((TermPtr = ParserObj.ParseTerm()) != nullptr)
    {
        TermDef* Def = TermPtr->isDef();
        if (!Def || Def->name()->value() != "group")
        {
            delete TermPtr;
            TermPtr = nullptr;
            continue;
        }

        if (!Def->term() || !Def->term()->isStruct())
        {
            ++SkippedInvalidStructCount;
            UE_LOG(LogTemp, Warning, TEXT("WARNING: group struct missing in '%s'"), *OobFilePath);
            delete TermPtr;
            TermPtr = nullptr;
            continue;
        }

        ++ParsedGroupCount;

        TermStruct* GroupStruct = Def->term()->isStruct();

        FS_CombatGroup NewCombatGroup;
        decltype(FS_CombatGroup().Unit) NewCombatUnitArray;
        NewCombatUnitArray.Empty();

        Text LocalName = "";
        Text LocalType = "";
        Text LocalRegion = "";
        Text LocalSystem = "";
        Text LocalParentType = "";

        EINTEL_TYPE LocalIntelType = EINTEL_TYPE::KNOWN;
        ECOMBATGROUP_TYPE LocalGroupType = ECOMBATGROUP_TYPE::NONE;
        ECOMBATGROUP_TYPE LocalParentGroupType = ECOMBATGROUP_TYPE::NONE;

        int32 LocalParentId = 0;
        int32 LocalEmpireId = 0;
        int32 LocalIff = -1;
        int32 LocalUnitIndex = 0;

        Vec3 LocalLoc(1.0e9f, 0.0f, 0.0f);

        const int32 GroupElemCount = (int32)GroupStruct->elements()->size();
        for (int32 FieldIdx = 0; FieldIdx < GroupElemCount; ++FieldIdx)
        {
            TermDef* PDef = GroupStruct->elements()->at(FieldIdx)->isDef();
            if (!PDef)
            {
                continue;
            }

            const Text& Key = PDef->name()->value();

            if (Key == "name")
            {
                GetDefText(LocalName, PDef, fn);
                NewCombatGroup.Name = FString(LocalName);
            }
            else if (Key == "intel")
            {
                Text Intel = "";
                GetDefText(Intel, PDef, fn);

                if (!SSWStringToEnum<EINTEL_TYPE>(FString(Intel).ToUpper(), LocalIntelType, false))
                {
                    LocalIntelType = EINTEL_TYPE::KNOWN;
                }

                NewCombatGroup.Intel = LocalIntelType;
            }
            else if (Key == "region")
            {
                GetDefText(LocalRegion, PDef, fn);
                NewCombatGroup.Region = FString(LocalRegion);
            }
            else if (Key == "system")
            {
                GetDefText(LocalSystem, PDef, fn);
                NewCombatGroup.System = FString(LocalSystem);
            }
            else if (Key == "loc")
            {
                GetDefVec(LocalLoc, PDef, fn);
                NewCombatGroup.Location = FVector(LocalLoc.X, LocalLoc.Y, LocalLoc.Z);
            }
            else if (Key == "parent_type")
            {
                GetDefText(LocalParentType, PDef, fn);

                const FString NormalizedParentType =
                    NormalizeCombatGroupTypeToken(FString(LocalParentType));

                if (!SSWStringToEnum<ECOMBATGROUP_TYPE>(NormalizedParentType, LocalParentGroupType, false))
                {
                    LocalParentGroupType = ECOMBATGROUP_TYPE::NONE;

                    UE_LOG(LogTemp, Warning,
                        TEXT("[LoadCombatRoster] unknown parent_type '%s' normalized to '%s' in '%s'"),
                        *FString(LocalParentType),
                        *NormalizedParentType,
                        *OobFilePath);
                }

                NewCombatGroup.ParentType = LocalParentGroupType;
            }
            else if (Key == "parent_id")
            {
                GetDefNumber(LocalParentId, PDef, fn);
                NewCombatGroup.ParentId = LocalParentId;
            }
            else if (Key == "empire_id")
            {
                GetDefNumber(LocalEmpireId, PDef, fn);
                NewCombatGroup.EmpireId = UFormattingUtils::GetEmpireTypeFromIndex(LocalEmpireId);
            }
            else if (Key == "iff")
            {
                GetDefNumber(LocalIff, PDef, fn);
                NewCombatGroup.Iff = LocalIff;
            }
            else if (Key == "id")
            {
                int32 LocalId = 0;
                GetDefNumber(LocalId, PDef, fn);
                NewCombatGroup.Id = LocalId;
            }
            else if (Key == "unit_index")
            {
                GetDefNumber(LocalUnitIndex, PDef, fn);
                NewCombatGroup.UnitIndex = LocalUnitIndex;
            }
            else if (Key == "type")
            {
                GetDefText(LocalType, PDef, fn);

                const FString NormalizedType =
                    NormalizeCombatGroupTypeToken(FString(LocalType));

                if (!SSWStringToEnum<ECOMBATGROUP_TYPE>(NormalizedType, LocalGroupType, false))
                {
                    LocalGroupType = ECOMBATGROUP_TYPE::NONE;

                    UE_LOG(LogTemp, Warning,
                        TEXT("[LoadCombatRoster] unknown type '%s' normalized to '%s' in '%s'"),
                        *FString(LocalType),
                        *NormalizedType,
                        *OobFilePath);
                }

                NewCombatGroup.Type = LocalGroupType;
            }
            else if (Key == "unit")
            {
                TermStruct* UnitStruct = (PDef->term() ? PDef->term()->isStruct() : nullptr);
                if (!UnitStruct)
                {
                    continue;
                }

                FS_CombatGroupUnit NewUnit;

                Text LocalUnitName = "";
                Text LocalUnitRegnum = "";
                Text LocalUnitRegion = "";
                Text LocalUnitClass = "";
                Text LocalUnitDesign = "";
                Text LocalUnitSkin = "";

                int32 LocalUnitCount = 1;
                int32 LocalUnitDamage = 0;
                int32 LocalUnitDead = 0;
                int32 LocalUnitHeading = 0;
                Vec3  LocalUnitLoc(1.0e9f, 0.0f, 0.0f);

                const int32 UnitElemCount = (int32)UnitStruct->elements()->size();
                for (int32 UnitFieldIdx = 0; UnitFieldIdx < UnitElemCount; ++UnitFieldIdx)
                {
                    TermDef* UDef = UnitStruct->elements()->at(UnitFieldIdx)->isDef();
                    if (!UDef)
                    {
                        continue;
                    }

                    const Text& UKey = UDef->name()->value();

                    if (UKey == "name")
                    {
                        GetDefText(LocalUnitName, UDef, fn);
                        NewUnit.UnitName = FString(LocalUnitName);
                    }
                    else if (UKey == "regnum")
                    {
                        GetDefText(LocalUnitRegnum, UDef, fn);
                    }
                    else if (UKey == "region")
                    {
                        GetDefText(LocalUnitRegion, UDef, fn);
                    }
                    else if (UKey == "loc")
                    {
                        GetDefVec(LocalUnitLoc, UDef, fn);
                    }
                    else if (UKey == "type")
                    {
                        GetDefText(LocalUnitClass, UDef, fn);
                    }
                    else if (UKey == "design")
                    {
                        GetDefText(LocalUnitDesign, UDef, fn);
                    }
                    else if (UKey == "skin")
                    {
                        GetDefText(LocalUnitSkin, UDef, fn);
                    }
                    else if (UKey == "count")
                    {
                        GetDefNumber(LocalUnitCount, UDef, fn);
                    }
                    else if (UKey == "dead_count")
                    {
                        GetDefNumber(LocalUnitDead, UDef, fn);
                    }
                    else if (UKey == "damage")
                    {
                        GetDefNumber(LocalUnitDamage, UDef, fn);
                    }
                    else if (UKey == "heading")
                    {
                        GetDefNumber(LocalUnitHeading, UDef, fn);
                    }
                }

                NewUnit.UnitName = FString(LocalUnitName);
                NewUnit.UnitRegnum = FString(LocalUnitRegnum);
                NewUnit.UnitRegion = FString(LocalUnitRegion);
                NewUnit.UnitLoc = FVector(LocalUnitLoc.X, LocalUnitLoc.Y, LocalUnitLoc.Z);
                NewUnit.UnitClass = FString(LocalUnitClass);
                NewUnit.UnitDesign = FString(LocalUnitDesign);
                NewUnit.UnitSkin = FString(LocalUnitSkin);
                NewUnit.UnitCount = LocalUnitCount;
                NewUnit.UnitDead = LocalUnitDead;
                NewUnit.UnitDamage = LocalUnitDamage;
                NewUnit.UnitHeading = LocalUnitHeading;

                NewCombatUnitArray.Add(NewUnit);
                NewCombatGroup.Unit = NewCombatUnitArray;
            }
        }

        const bool bPassTeam = (Team < 0) || (NewCombatGroup.Iff == Team);

        if (NewCombatGroup.Iff > -1 && bPassTeam)
        {
            const FName RowName(*(
                UFormattingUtils::GetOrdinal(NewCombatGroup.Id) + TEXT(" ") +
                FString(UFormattingUtils::GetGroupTypeDisplayName(NewCombatGroup.Type)) +
                TEXT(" [") + NewCombatGroup.Name + TEXT("]")
                ));

            if (CombatGroupDataTable->FindRow<FS_CombatGroup>(RowName, TEXT("DuplicateCheck"), false))
            {
                ++OverwrittenRowCount;
                UE_LOG(LogTemp, Warning,
                    TEXT("[LoadCombatRoster] DUPLICATE ROW NAME '%s' file=%s Empire=%d Type=%d Id=%d Iff=%d Region=%s"),
                    *RowName.ToString(),
                    *OobFilePath,
                    (int32)NewCombatGroup.EmpireId,
                    (int32)NewCombatGroup.Type,
                    NewCombatGroup.Id,
                    NewCombatGroup.Iff,
                    *NewCombatGroup.Region);
            }

            NewCombatGroup.DisplayName = RowName.ToString();
            CombatGroupDataTable->AddRow(RowName, NewCombatGroup);
            ++AddedGroupCount;
        }
        else
        {
            ++SkippedTeamFilterCount;
            UE_LOG(LogTemp, Warning,
                TEXT("[LoadCombatRoster] SKIPPED GROUP file=%s Name=%s Type=%d Id=%d Iff=%d Team=%d"),
                *OobFilePath,
                *NewCombatGroup.Name,
                (int32)NewCombatGroup.Type,
                NewCombatGroup.Id,
                NewCombatGroup.Iff,
                Team);
        }

        delete TermPtr;
        TermPtr = nullptr;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[LoadCombatRoster] SUMMARY file=%s Parsed=%d Added=%d SkippedInvalid=%d SkippedFilter=%d Overwritten=%d"),
        *OobFilePath,
        ParsedGroupCount,
        AddedGroupCount,
        SkippedInvalidStructCount,
        SkippedTeamFilterCount,
        OverwrittenRowCount);

    UE_LOG(LogTemp, Log, TEXT("[LoadCombatRoster] complete: %s"), *OobFilePath);
}

// -----------------------------------------------------------------------------
// Readback
// -----------------------------------------------------------------------------

void USSWCombatGroupSubsystem::ReadCombatRosterData()
{
    UE_LOG(LogTemp, Warning, TEXT("[LoadCombatRoster] ReadCombatRosterTable: BEGIN"));

    if (!ResolveCombatGroupDataTable())
    {
        UE_LOG(LogTemp, Error, TEXT("[LoadCombatRoster] ReadCombatRosterTable: CombatGroupDataTable is NULL"));
        return;
    }

    CombatRosterData.Reset();
    CombatGroupMapByRow.Reset();
    CombatGroupRowByName.Reset();
    CombatGroupRowByKey.Reset();
    CombatGroupRegistry::Clear();

    static const FString ContextString(TEXT("ReadCombatRosterData"));
    const TArray<FName> RowNames = CombatGroupDataTable->GetRowNames();

    UE_LOG(LogTemp, Warning,
        TEXT("[LoadCombatRoster] ReadCombatRosterTable: Found %d rows"),
        RowNames.Num());

    for (const FName& RowName : RowNames)
    {
        const FS_CombatGroup* Row =
            CombatGroupDataTable->FindRow<FS_CombatGroup>(RowName, ContextString);

        if (!Row)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[LoadCombatRoster] ReadCombatRosterTable: Failed to read row '%s'"),
                *RowName.ToString());
            continue;
        }

        FS_CombatGroup Group = *Row;

        if (Group.DisplayName.IsEmpty())
        {
            Group.DisplayName = RowName.ToString();
        }

        CombatRosterData.Add(Group);
        CombatGroupMapByRow.Add(RowName, Group);

        if (!Group.Name.IsEmpty())
        {
            CombatGroupRowByName.Add(Group.Name.ToLower(), RowName);
        }

        CombatGroupRowByKey.Add(
            FSSWCombatGroupKey(Group.EmpireId, Group.Type, Group.Id),
            RowName);

        CombatGroupRegistry::RegisterGroup(RowName, Group);

        UE_LOG(LogTemp, Log,
            TEXT("[LoadCombatRoster] CombatGroup Table Loaded: Row=%s Type=%d Id=%d ParentType=%d ParentId=%d Empire=%d Iff=%d Name=%s Region=%s"),
            *RowName.ToString(),
            (int32)Group.Type,
            Group.Id,
            (int32)Group.ParentType,
            Group.ParentId,
            (int32)Group.EmpireId,
            Group.Iff,
            *Group.DisplayName,
            *Group.Region);
    }

    BuildIndexes();

    UE_LOG(LogTemp, Warning,
        TEXT("1: COMPLETE (%d groups, registry=%d)"),
        CombatRosterData.Num(),
        CombatGroupRegistry::Num());
}

// -----------------------------------------------------------------------------
// Queries
// -----------------------------------------------------------------------------

const FS_CombatGroup* USSWCombatGroupSubsystem::FindCombatGroupByRowName(FName RowName) const
{
    return CombatGroupMapByRow.Find(RowName);
}

const FS_CombatGroup* USSWCombatGroupSubsystem::FindCombatGroupByKey(
    EEMPIRE_NAME EmpireId,
    ECOMBATGROUP_TYPE Type,
    int32 Id) const
{
    const FName* FoundRow =
        CombatGroupRowByKey.Find(FSSWCombatGroupKey(EmpireId, Type, Id));

    return FoundRow ? CombatGroupMapByRow.Find(*FoundRow) : nullptr;
}

const FS_CombatGroup* USSWCombatGroupSubsystem::FindCombatGroupByName(const FString& GroupName) const
{
    const FName* FoundRow = CombatGroupRowByName.Find(GroupName.ToLower());
    return FoundRow ? CombatGroupMapByRow.Find(*FoundRow) : nullptr;
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

void USSWCombatGroupSubsystem::BuildIndexes()
{
    // Keep phase 1 light.
    // Add parent/child indexes here later if needed.
}

void USSWCombatGroupSubsystem::ClearCombatGroupData()
{
    CombatRosterData.Reset();
    CombatGroupMapByRow.Reset();
    CombatGroupRowByName.Reset();
    CombatGroupRowByKey.Reset();
}