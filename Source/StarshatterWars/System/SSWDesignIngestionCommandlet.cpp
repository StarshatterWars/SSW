#include "SSWDesignIngestionCommandlet.h"

#if WITH_EDITOR
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "StarshatterShipDesignSubsystem.h"
#include "StarshatterSystemDesignSubsystem.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "UObject/SavePackage.h"
#include "SSWGameInstance.h"
#include "StarshatterGameDataSubsystem.h"
#include "StarshatterEnvironmentSubsystem.h"
#include "SSWCombatGroupSubsystem.h"
#include "ShipVisualCatalog.h"
#include "Farcaster.h"
#include "FlightDeck.h"
#include "LandingGear.h"
#include "Weapon.h"

namespace
{
bool LoadOrderOfBattleOverview(UDataTable& Table)
{
    const FString Path = FPaths::ProjectContentDir() / TEXT("GameData/Systems/OrderOfBattleOverview.def");
    TArray<FString> Lines;
    if (!FFileHelper::LoadFileToStringArray(Lines, *Path) || Lines.IsEmpty() ||
        Lines[0].TrimStartAndEnd() != TEXT("ORDER_OF_BATTLE_OVERVIEW"))
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid OOB overview DEF: %s"), *Path);
        return false;
    }

    FName Key;
    FString RowText;
    const auto AddForce = [&]()
    {
        if (Key.IsNone()) return RowText.IsEmpty();
        FS_OOBForce Force;
        const TCHAR* End = FS_OOBForce::StaticStruct()->ImportText(*RowText, &Force, &Table, PPF_None,
            GWarn, TEXT("OrderOfBattleOverview"));
        if (!End || *End || Force.Name != Key.ToString() || Table.GetRowMap().Contains(Key))
        {
            UE_LOG(LogTemp, Error, TEXT("Invalid or duplicate OOB overview force %s."), *Key.ToString());
            return false;
        }
        Table.AddRow(Key, Force);
        return true;
    };

    for (int32 Index = 1; Index < Lines.Num(); ++Index)
    {
        FString Line = Lines[Index].TrimStartAndEnd();
        if (Line.IsEmpty() || Line.StartsWith(TEXT("//"))) continue;
        if (Line.RemoveFromStart(TEXT("force:")))
        {
            if (!AddForce()) return false;
            Key = FName(*Line.TrimStartAndEnd());
            RowText.Reset();
        }
        else
        {
            Line.RemoveFromStart(TEXT("row:"));
            RowText += Line.TrimStartAndEnd();
        }
    }
    return AddForce() && !Table.GetRowMap().IsEmpty();
}

bool CheckTable(const UDataTable& Imported, const TCHAR* Name, bool bAllowEmpty)
{
    const FString ActivePath = FString::Printf(TEXT("/Game/Game/%s.%s"), Name, Name);
    const auto* Active = LoadObject<UDataTable>(nullptr, *ActivePath);
    if (!Active || Imported.GetRowStruct() != Active->GetRowStruct() || (!bAllowEmpty && Imported.GetRowMap().IsEmpty()))
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot validate staged table %s."), Name);
        return false;
    }

    int32 ImportedOnly = 0;
    int32 ActiveOnly = 0;
    int32 Changed = 0;
    for (const auto& Row : Imported.GetRowMap())
    {
        const uint8* const* Existing = Active->GetRowMap().Find(Row.Key);
        if (!Existing)
            ++ImportedOnly;
        else if (!Imported.GetRowStruct()->CompareScriptStruct(Row.Value, *Existing, 0))
            ++Changed;
    }
    for (const auto& Row : Active->GetRowMap())
        if (!Imported.GetRowMap().Contains(Row.Key))
            ++ActiveOnly;

    UE_LOG(LogTemp, Display, TEXT("%s: %d imported, %d active, %d new, %d active-only, %d changed."),
        Name, Imported.GetRowMap().Num(), Active->GetRowMap().Num(), ImportedOnly, ActiveOnly, Changed);
    if (Imported.GetRowStruct() == FS_OOBForce::StaticStruct())
    {
        for (const auto& Pair : Imported.GetRowMap())
        {
            const auto& NewForce = *reinterpret_cast<const FS_OOBForce*>(Pair.Value);
            for (const auto& Candidate : Active->GetRowMap())
            {
                const auto& OldForce = *reinterpret_cast<const FS_OOBForce*>(Candidate.Value);
                if (OldForce.Iff != NewForce.Iff || OldForce.Empire != NewForce.Empire) continue;
                UE_LOG(LogTemp, Display, TEXT("OOB %s -> %s (id %d -> %d): fleets %d -> %d; battalions %d -> %d; civilian %d -> %d; transport %d -> %d; infrastructure %d -> %d; payload identical=%d."),
                    *Candidate.Key.ToString(), *Pair.Key.ToString(), OldForce.Id, NewForce.Id,
                    OldForce.Fleet.Num(), NewForce.Fleet.Num(), OldForce.Battalion.Num(), NewForce.Battalion.Num(),
                    OldForce.Civilian.Num(), NewForce.Civilian.Num(), OldForce.Transport.Num(), NewForce.Transport.Num(),
                    OldForce.Infrastructure.Num(), NewForce.Infrastructure.Num(),
                    FS_OOBForce::StaticStruct()->CompareScriptStruct(Candidate.Value, Pair.Value, 0));
                break;
            }
        }
    }
    return true;
}

bool SaveStagedTable(UDataTable& Imported, const TCHAR* Name)
{
    FString BaseName(Name);
    BaseName.RemoveFromStart(TEXT("DT_"));
    const FString TestName = TEXT("DT_Test_") + BaseName;
    const FString PackageName = TEXT("/Game/Editor/GeneratedDesigns/") + TestName;
    const FString ObjectPath = PackageName + TEXT(".") + TestName;
    auto* Package = CreatePackage(*PackageName);
    auto* Staged = LoadObject<UDataTable>(nullptr, *ObjectPath, nullptr, LOAD_NoWarn);
    if (!Staged)
        Staged = NewObject<UDataTable>(Package, FName(*TestName), RF_Public | RF_Standalone);
    if (Staged->GetOutermost() != Package)
        return false;

    if (Staged->GetRowStruct())
        Staged->EmptyTable();
    Staged->RowStruct = Imported.RowStruct;
    for (const auto& Row : Imported.GetRowMap())
        Staged->AddRow(Row.Key, *reinterpret_cast<const FTableRowBase*>(Row.Value));

    const FString Filename = FPackageName::LongPackageNameToFilename(PackageName,
        FPackageName::GetAssetPackageExtension());
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    return UPackage::SavePackage(Package, Staged, *Filename, Args);
}
}
#endif

USSWDesignIngestionCommandlet::USSWDesignIngestionCommandlet()
{
    IsEditor = true;
    IsClient = false;
    IsServer = false;
    LogToConsole = true;
}

int32 USSWDesignIngestionCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
    auto* Context = NewObject<USSWGameInstance>();
    auto* Systems = NewObject<UStarshatterSystemDesignSubsystem>(Context);
    auto* Weapons = NewObject<UStarshatterWeaponDesignSubsystem>(Context);
    auto* Ships = NewObject<UStarshatterShipDesignSubsystem>(Context);

    auto* SystemTable = NewObject<UDataTable>();
    auto* WeaponTable = NewObject<UDataTable>();
    auto* ShipTable = NewObject<UDataTable>();
    SystemTable->RowStruct = FSystemDesign::StaticStruct();
    WeaponTable->RowStruct = FWeaponDesign::StaticStruct();
    ShipTable->RowStruct = FShipDesign::StaticStruct();
    Systems->SystemDesignDataTable = SystemTable;
    Weapons->WeaponDesignDataTable = WeaponTable;
    Ships->ShipDesignDataTable = ShipTable;

    Systems->LoadSystemDesigns();
    Weapons->LoadWeaponDesigns();
    Ships->InitializeShipDesigns();

    struct FTableOutput { UDataTable* Table; const TCHAR* Name; bool bAllowEmpty = false; };
    TArray<FTableOutput> Tables = {
        {SystemTable, TEXT("DT_SystemDesign")},
        {WeaponTable, TEXT("DT_WeaponDesign")},
        {ShipTable, TEXT("DT_ShipDesign")}
    };
    const auto Stage = [&Tables](const TCHAR* Name, bool bPreserveAuthoredRows = false, bool bAllowEmpty = false)
    {
        const FString Path = FString::Printf(TEXT("/Game/Game/%s.%s"), Name, Name);
        const auto* Active = LoadObject<UDataTable>(nullptr, *Path);
        if (!Active || !Active->RowStruct)
        {
            UE_LOG(LogTemp, Error, TEXT("Missing table schema: %s"), *Path);
            return static_cast<UDataTable*>(nullptr);
        }
        auto* Table = NewObject<UDataTable>();
        Table->RowStruct = Active->RowStruct;
        if (bPreserveAuthoredRows)
            for (const auto& Row : Active->GetRowMap())
                Table->AddRow(Row.Key, *reinterpret_cast<const FTableRowBase*>(Row.Value));
        Tables.Add({Table, Name, bAllowEmpty});
        return Table;
    };

    auto* Campaign = NewObject<UStarshatterGameDataSubsystem>(Context);
    Campaign->CampaignDataTable = Stage(TEXT("DT_Campaign"));
    Campaign->CampaignActionDataTable = Stage(TEXT("DT_CampaignAction"));
    Campaign->ZonesDataTable = Stage(TEXT("DT_CampaignZone"));
    Campaign->CutsceneDataTable = Stage(TEXT("DT_Cutscene"));
    Campaign->SingleMissionsDataTable = Stage(TEXT("DT_SingleMissions"));
    const UDataTable* StoryMissions = Stage(TEXT("DT_StoryMissions"), true, true);
    if (!StoryMissions || StoryMissions->GetRowStruct() != FS_CampaignMission::StaticStruct())
    {
        UE_LOG(LogTemp, Error, TEXT("DT_StoryMissions must use FS_CampaignMission rows."));
        return 1;
    }
    Campaign->MultiplayerMissionsDataTable = Stage(TEXT("DT_MultiplayerMissions"));
    Campaign->OrderOfBattleDataTable = Stage(TEXT("DT_OrderOfBattle"));
    Campaign->RanksDataTable = Stage(TEXT("FRankInfo"), true);
    Campaign->MedalsDataTable = Stage(TEXT("FMedalInfo"), true);
    if (!Campaign->CampaignDataTable || !Campaign->CampaignActionDataTable || !Campaign->ZonesDataTable ||
        !Campaign->CutsceneDataTable || !Campaign->SingleMissionsDataTable || !Campaign->MultiplayerMissionsDataTable ||
        !Campaign->OrderOfBattleDataTable || !Campaign->RanksDataTable || !Campaign->MedalsDataTable)
        return 1;
    Campaign->InitializeCampaignData();
    Campaign->LoadSingleMissions();
    Campaign->LoadMultiplayerMissions();

    Campaign->LoadAwardTables();
    // Award ingestion replaces its input with a private copy.
    for (auto& Output : Tables)
    {
        if (FCString::Strcmp(Output.Name, TEXT("FRankInfo")) == 0)
            Output.Table = Campaign->RanksDataTable;
        else if (FCString::Strcmp(Output.Name, TEXT("FMedalInfo")) == 0)
            Output.Table = Campaign->MedalsDataTable;
    }

    auto* Roster = NewObject<USSWCombatGroupSubsystem>(Context);
    Roster->CombatGroupDataTable = Stage(TEXT("DT_CombatGroup"));
    if (!Roster->CombatGroupDataTable) return 1;
    Roster->InitializeCombatRoster();
    if (!LoadOrderOfBattleOverview(*Campaign->OrderOfBattleDataTable)) return 1;

    auto* Environment = NewObject<UStarshatterEnvironmentSubsystem>(Context);
    Environment->SSWInstance = Context;
    Environment->GalaxyDataTable = Stage(TEXT("DT_GalaxyMap"));
    Environment->RegionsDataTable = Stage(TEXT("DT_Region"));
    if (!Environment->GalaxyDataTable || !Environment->RegionsDataTable) return 1;
    Environment->LoadGalaxyMap();

    if (Campaign->CampaignDataTable->GetRowMap().Num() != 5)
    {
        UE_LOG(LogTemp, Error, TEXT("Expected five campaign definitions."));
        return 1;
    }

    bool bWithinLimits = true;
    TSet<FName> UnresolvedSystemKeys;
    for (const auto& Pair : ShipTable->GetRowMap())
    {
        const auto& Row = *reinterpret_cast<const FShipDesign*>(Pair.Value);
        const auto Limit = [&](int32 Count, int32 Max, const TCHAR* Field)
        {
            if (Count <= Max) return;
            UE_LOG(LogTemp, Error, TEXT("%s: %s has %d entries, limit is %d."), *Pair.Key.ToString(), Field, Count, Max);
            bWithinLimits = false;
        };
        const auto CheckSystemKey = [&](const FString& Name)
        {
            const FName Key(*Name.TrimStartAndEnd());
            if (!Key.IsNone() && !SystemTable->GetRowMap().Contains(Key))
                UnresolvedSystemKeys.Add(Key);
        };
        for (const auto& Power : Row.Power) CheckSystemKey(Power.DesignName);
        for (const auto& Drive : Row.Drive) CheckSystemKey(Drive.DesignName);
        for (const auto& Thruster : Row.Thruster) CheckSystemKey(Thruster.DesignName);
        for (const auto& Shield : Row.Shield) CheckSystemKey(Shield.DesignName);
        for (const auto& Computer : Row.Computer) CheckSystemKey(Computer.DesignName);
        for (const auto& Gun : Row.Weapon) CheckSystemKey(Gun.DesignName);
        for (const auto& Caster : Row.Farcaster)
            Limit(Caster.ApproachPoints.Num(), Farcaster::NUM_APPROACH_PTS, TEXT("farcaster approach"));
        for (const auto& Gear : Row.LandingGear)
            Limit(Gear.GearItems.Num(), LandingGear::MAX_GEAR, TEXT("landing gear"));
        for (const auto& Deck : Row.FlightDeck)
            Limit(Deck.ApproachPoints.Num(), FlightDeck::NUM_APPROACH_PTS, TEXT("flight deck approach"));
        for (const auto& Gun : Row.Weapon)
            Limit(Gun.Muzzles.Num(), Weapon::MAX_BARRELS, TEXT("weapon muzzles"));
    }
    TArray<FName> MissingKeys = UnresolvedSystemKeys.Array();
    MissingKeys.Sort(FNameLexicalLess());
    for (const FName Key : MissingKeys)
        UE_LOG(LogTemp, Warning, TEXT("No sys.def component template for design key '%s'."), *Key.ToString());

    bool bAliasesValid = true;
    for (const auto& Entry : ShipVisualCatalog::Entries())
    {
        if (ShipVisualCatalog::DefinitionName(Entry.Design, Entry.Model) == Entry.Definition) continue;
        UE_LOG(LogTemp, Error, TEXT("Catalog alias %s / %s failed to resolve %s."), Entry.Design, Entry.Model, Entry.Definition);
        bAliasesValid = false;
    }
    if (!bWithinLimits || !bAliasesValid) return 1;

    for (const auto& Table : Tables)
        if (!CheckTable(*Table.Table, Table.Name, Table.bAllowEmpty))
            return 1;

    if (FParse::Param(*Params, TEXT("WriteStaged")))
    {
        for (const auto& Table : Tables)
            if (!SaveStagedTable(*Table.Table, Table.Name))
                return 2;
    }
    return 0;
#else
    return 1;
#endif
}


