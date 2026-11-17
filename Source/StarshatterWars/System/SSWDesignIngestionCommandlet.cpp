#include "SSWDesignIngestionCommandlet.h"

#if WITH_EDITOR
#include "Algo/Find.h"
#include "Engine/DataTable.h"
#include "DesignTableJson.h"
#include "ShipDesignRegistry.h"
#include "CargoCatalogSubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "StarshatterShipDesignSubsystem.h"
#include "GameStructs_products.h"
#include "StarshatterSystemDesignSubsystem.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "UObject/SavePackage.h"
#include "SSWGameInstance.h"
#include "StarshatterGameDataSubsystem.h"
#include "StarshatterEnvironmentSubsystem.h"
#include "SSWCombatGroupSubsystem.h"
#include "ShipVisualCatalog.h"
#include "FreightManifest.h"
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

bool ImportProductionRecipes(UDataTable& Table)
{
    const FString Path = FPaths::ProjectContentDir() / TEXT("GameData/Cargo/recipes.json");
    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *Path))
    {
        UE_LOG(LogTemp, Error, TEXT("Could not read production recipes from %s"), *Path);
        return false;
    }
    Table.RowStruct = FSSWProductionRecipe::StaticStruct();
    const TArray<FString> Problems = Table.CreateTableFromJSONString(Json);
    for (const FString& Problem : Problems)
        UE_LOG(LogTemp, Error, TEXT("%s: %s"), *Path, *Problem);
    if (!Problems.IsEmpty() || Table.GetRowMap().IsEmpty()) return false;

    for (const auto& Row : Table.GetRowMap())
    {
        const auto& Recipe = *reinterpret_cast<const FSSWProductionRecipe*>(Row.Value);
        if ((Recipe.Minerals.IsEmpty() && Recipe.Inputs.IsEmpty()) || Recipe.Minerals.Contains(ESSWMineral::None) ||
            !FMath::IsFinite(Recipe.InputTonnes) || !FMath::IsFinite(Recipe.OutputTonnes) ||
            !FMath::IsFinite(Recipe.SecondsPerBatch) ||
            !Recipe.CanStart(Recipe.InputTonnes, Recipe.OutputTonnes))
        {
            UE_LOG(LogTemp, Error, TEXT("Invalid production recipe %s in %s"), *Row.Key.ToString(), *Path);
            return false;
        }
    }
    for (const auto& Pair : Table.GetRowMap())
    {
        const auto& Recipe = *reinterpret_cast<const FSSWProductionRecipe*>(Pair.Value);
        TSet<FName> Seen; double Mass = 0;
        for (const auto& Input : Recipe.Inputs)
        {
            if (Input.Item.IsNone() || Seen.Contains(Input.Item) || !FMath::IsFinite(Input.Quantity) ||
                !FMath::IsFinite(Input.UnitTonnes) || Input.Quantity <= 0 || Input.UnitTonnes <= 0) return false;
            Seen.Add(Input.Item); Mass += Input.Quantity * Input.UnitTonnes;
        }
        if (!Recipe.Inputs.IsEmpty() && !FMath::IsNearlyEqual(Mass, Recipe.InputTonnes, 1.e-6)) return false;
    }
    UE_LOG(LogTemp, Display, TEXT("Imported %d production recipes from %s"), Table.GetRowMap().Num(), *Path);
    return true;
}

bool SaveOutputTable(UDataTable& Imported, const TCHAR* Name, bool bStaged = true)
{
    FString BaseName(Name);
    BaseName.RemoveFromStart(TEXT("DT_"));
    const FString TestName = bStaged ? TEXT("DT_Test_") + BaseName : FString(Name);
    const FString PackageName = (bStaged ? TEXT("/Game/Editor/GeneratedDesigns/") : TEXT("/Game/Game/")) + TestName;
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

bool ExportVerifiedShipCatalog(UDataTable& Legacy)
{
    const auto* Active = LoadObject<UDataTable>(nullptr, TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
    if (!Active || Active->GetRowStruct() != FShipDesign::StaticStruct() || Active->GetRowMap().IsEmpty())
        return false;

    const FString Verification = FPaths::ProjectSavedDir() / TEXT("ShipJsonRoundTrip.json");
    const auto Verify = [&Verification](UDataTable& Source, FString& Json)
    {
        if (!DesignTableJson::Export(Source, Json) ||
            !FFileHelper::SaveStringToFile(Json, *Verification, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
            return false;
        auto* Imported = NewObject<UDataTable>();
        return UStarshatterShipDesignSubsystem::ImportJsonCatalog(*Imported, Verification) &&
            DesignTableJson::Compare(Source, *Imported);
    };

    FString Json;
    if (!Verify(Legacy, Json)) return false;
    UE_LOG(LogTemp, Display, TEXT("Verified exact JSON round trip of %d legacy ship definitions"), Legacy.GetRowMap().Num());

    auto* Catalog = DuplicateObject<UDataTable>(Active, GetTransientPackage());
    int32 Added = 0;
    Legacy.ForeachRow<FShipDesign>(TEXT("Include DEF-only ships"), [Catalog, &Added](FName Name, const FShipDesign& Row)
    {
        if (!Catalog->GetRowMap().Contains(Name))
        {
            Catalog->AddRow(Name, Row);
            ++Added;
        }
    });

    for (const auto& Pair : Catalog->GetRowMap())
    {
        auto& Row = *reinterpret_cast<FShipDesign*>(Pair.Value);
        UStarshatterShipDesignSubsystem::CompleteLegacyBaseCost(Pair.Key, Row);
        ShipDesignRegistry::CompleteLegacySkinMetadata(Pair.Key, Row.Skin);
    }
    if (!Verify(*Catalog, Json)) return false;

    const FString Destination = FPaths::ProjectContentDir() / TEXT("GameData/Ships/ships.json");
    if (!FFileHelper::SaveStringToFile(Json, *Destination, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        return false;
    UE_LOG(LogTemp, Display, TEXT("Verified and wrote %d ship rows (%d active, %d DEF-only) to %s"),
        Catalog->GetRowMap().Num(), Active->GetRowMap().Num(), Added, *Destination);
    return true;
}

bool WriteShipAndWeaponJsonTables()
{
    auto* Ships = NewObject<UDataTable>();
    auto* Weapons = NewObject<UDataTable>();
    const FString GameData = FPaths::ProjectContentDir() / TEXT("GameData");
    if (!UStarshatterShipDesignSubsystem::ImportJsonCatalog(*Ships, GameData / TEXT("Ships/ships.json")) ||
        !UStarshatterWeaponDesignSubsystem::ImportJsonCatalog(*Weapons, GameData / TEXT("Weapons/wep.json")))
        return false;
    if (!SaveOutputTable(*Ships, TEXT("DT_ShipDesign"), false) ||
        !SaveOutputTable(*Weapons, TEXT("DT_WeaponDesign"), false))
        return false;

    UE_LOG(LogTemp, Display, TEXT("Saved %d JSON ship rows and %d JSON weapon rows to the active DataTables"),
        Ships->GetRowMap().Num(), Weapons->GetRowMap().Num());
    return true;
}

bool ImportShipCargo(bool bPricesOnly = false)
{
    constexpr TCHAR TablePath[] = TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign");
    const UDataTable* Active = LoadObject<UDataTable>(nullptr, TablePath);
    if (!Active || Active->GetRowStruct() != FShipDesign::StaticStruct()) return false;

    UDataTable* Updated = DuplicateObject<UDataTable>(Active, GetTransientPackage());
    int32 UpdatedRows = 0;
    for (const FName RowName : Updated->GetRowNames())
    {
        FShipDesign* Row = Updated->FindRow<FShipDesign>(RowName, TEXT("Import ship cargo"));
        if (!Row) return false;

        const FString Definition = ShipVisualCatalog::DefinitionName(Row->ShipName, Row->Model);
        auto Profile = SSWFreightManifest::LoadProfile(Definition);
        if (Profile.BaseCost <= 0 && Definition != RowName.ToString())
            Profile.BaseCost = SSWFreightManifest::LoadProfile(RowName.ToString()).BaseCost;
        if (Profile.BaseCost > 0)
            Row->BaseCost = Profile.BaseCost;
        if (bPricesOnly)
        {
            UpdatedRows += Profile.BaseCost > 0 ? 1 : 0;
            continue;
        }

        if (Profile.Capacity.Tonnes <= 0.0 && Definition != RowName.ToString())
            Profile = SSWFreightManifest::LoadProfile(RowName.ToString());

        if (Profile.Capacity.Tonnes <= 0.0)
        {
            const uint32 ShipClasses = static_cast<uint32>(CLASSIFICATION::DROPSHIPS) |
                (static_cast<uint32>(CLASSIFICATION::STARSHIPS) & ~static_cast<uint32>(CLASSIFICATION::FARCASTER));
            if (static_cast<uint32>(Row->ShipType) & ShipClasses)
            {
                UE_LOG(LogTemp, Error, TEXT("Missing cargo capacity for ship row %s (DEF %s)."),
                    *RowName.ToString(), *Definition);
                return false;
            }
            continue;
        }

        if (!Profile.Options.IsEmpty())
            Row->CargoManifest = MoveTemp(Profile.Options);
        Row->CargoCapacity = Profile.Capacity;
        ++UpdatedRows;
    }
    UE_LOG(LogTemp, Display, TEXT("Imported %s for %d design rows."),
        bPricesOnly ? TEXT("base costs") : TEXT("cargo capacities"), UpdatedRows);
    return UpdatedRows > 0 && SaveOutputTable(*Updated, TEXT("DT_ShipDesign"), false);
}
}

bool USSWDesignIngestionCommandlet::ImportSingleShipDefinition(const FString& Definition)
{
    const auto Entries = ShipVisualCatalog::Entries();
    const auto* Entry = Algo::FindByPredicate(Entries, [&Definition](const auto& Candidate)
    {
        return Definition.Equals(Candidate.Definition, ESearchCase::IgnoreCase);
    });
    if (!Entry)
    {
        UE_LOG(LogTemp, Error, TEXT("Unknown ship definition: %s"), *Definition);
        return false;
    }

    constexpr TCHAR TablePath[] = TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign");
    const UDataTable* Active = LoadObject<UDataTable>(nullptr, TablePath);
    if (!Active || Active->GetRowStruct() != FShipDesign::StaticStruct())
        return false;

    auto* Context = NewObject<USSWGameInstance>();
    auto* Ships = NewObject<UStarshatterShipDesignSubsystem>(Context);
    auto* Imported = NewObject<UDataTable>(Ships);
    Imported->RowStruct = FShipDesign::StaticStruct();
    Ships->ShipDesignDataTable = Imported;

    const FString Path = FPaths::ProjectContentDir() / TEXT("GameData/Ships") /
        (FString(Entry->Definition) + TEXT(".def"));
    Ships->LoadShipDesign(TCHAR_TO_ANSI(*Path));

    const FName RowName(Entry->Design);
    const FShipDesign* Row = Imported->FindRow<FShipDesign>(RowName, TEXT("Import single ship"));
    if (!Row)
        return false;

    UDataTable* Updated = DuplicateObject<UDataTable>(Active, GetTransientPackage());
    Updated->AddRow(RowName, *Row);
    UE_LOG(LogTemp, Display, TEXT("Imported %s with %.2f tonnes of cargo capacity."),
        *RowName.ToString(), Row->CargoCapacity.Tonnes);
    return SaveOutputTable(*Updated, TEXT("DT_ShipDesign"), false);
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
    if (FParse::Param(*Params, TEXT("ExportShipJson")))
    {
        auto* Legacy = NewObject<UDataTable>();
        Legacy->RowStruct = FShipDesign::StaticStruct();
        auto* Parser = NewObject<UStarshatterShipDesignSubsystem>(NewObject<UGameInstance>());
        auto* Individual = NewObject<UDataTable>();
        Individual->RowStruct = FShipDesign::StaticStruct();
        Parser->ShipDesignDataTable = Individual;
        const FString Directory = FPaths::ProjectContentDir() / TEXT("GameData/Ships");
        TArray<FString> Files;
        IFileManager::Get().FindFiles(Files, *(Directory / TEXT("*.def")), true, false);

        struct FSourceDesign { FName Key; FName FileKey; FShipDesign Design; };
        TArray<FSourceDesign> Sources;
        for (const FString& File : Files)
        {
            Individual->EmptyTable();
            const FTCHARToUTF8 Path(*(Directory / File));
            Parser->LoadShipDesign(Path.Get());
            if (Individual->GetRowMap().Num() != 1)
            {
                UE_LOG(LogTemp, Error, TEXT("Expected one ship design in %s"), *File);
                return 1;
            }
            for (const auto& Entry : Individual->GetRowMap())
            {
                const auto& Row = *reinterpret_cast<const FShipDesign*>(Entry.Value);
                Legacy->AddRow(Entry.Key, Row);
                Sources.Add({Entry.Key, FName(*FPaths::GetBaseFilename(File)), Row});
            }
        }
        for (const auto& Source : Sources)
        {
            const auto* Canonical = Legacy->FindRow<FShipDesign>(Source.Key, TEXT("Ship JSON migration"), false);
            if (!Canonical || Canonical->Model == Source.Design.Model) continue;
            if (Legacy->GetRowMap().Contains(Source.FileKey))
            {
                UE_LOG(LogTemp, Error, TEXT("Conflicting ship variant key %s"), *Source.FileKey.ToString());
                return 1;
            }
            Legacy->AddRow(Source.FileKey, Source.Design);
        }
        return ExportVerifiedShipCatalog(*Legacy) ? 0 : 1;
    }
    if (FParse::Param(*Params, TEXT("WriteShipAndWeaponJsonTables")))
        return WriteShipAndWeaponJsonTables() ? 0 : 1;

    if (FParse::Param(*Params, TEXT("WriteCargoCatalog")))
    {
        auto* Table = NewObject<UDataTable>();
        return UCargoCatalogSubsystem::ImportDefinitions(*Table) &&
            SaveOutputTable(*Table, TEXT("DT_CargoCatalog"), false) ? 0 : 1;
    }

    if (FParse::Param(*Params, TEXT("WriteSystemDesign")))
    {
        auto* Table = NewObject<UDataTable>();
        return UStarshatterSystemDesignSubsystem::ImportDefinitions(*Table) &&
            SaveOutputTable(*Table, TEXT("DT_SystemDesign"), false) ? 0 : 1;
    }

    FString ShipDefinition;
    if (FParse::Value(*Params, TEXT("ShipDefinition="), ShipDefinition))
        return ImportSingleShipDefinition(ShipDefinition) ? 0 : 1;

    if (FParse::Param(*Params, TEXT("WriteProductionFacilities")))
    {
        FString Json;
        if (!FFileHelper::LoadFileToString(Json, *(FPaths::ProjectContentDir() /
            TEXT("GameData/Production/facilities.json")))) return 1;
        auto* Table = NewObject<UDataTable>();
        Table->RowStruct = FSSWProductionFacility::StaticStruct();
        const TArray<FString> Problems = Table->CreateTableFromJSONString(Json);
        for (const FString& Problem : Problems)
            UE_LOG(LogTemp, Error, TEXT("%s"), *Problem);
        if (!Problems.IsEmpty() || Table->GetRowMap().IsEmpty()) return 1;
        UE_LOG(LogTemp, Display, TEXT("Imported %d production facilities"), Table->GetRowMap().Num());
        return SaveOutputTable(*Table, TEXT("DT_ProductionFacilities"), false) ? 0 : 1;
    }

    if (FParse::Param(*Params, TEXT("WriteProductionRecipes")))
    {
        auto* Table = NewObject<UDataTable>();
        return ImportProductionRecipes(*Table) &&
            SaveOutputTable(*Table, TEXT("DT_ProductionRecipes"), false) ? 0 : 1;
    }

    if (FParse::Param(*Params, TEXT("WriteJanusProduction")))
    {
        auto* Table = NewObject<UDataTable>();
        if (!ImportProductionRecipes(*Table)) return 2;
        if (!ImportSingleShipDefinition(TEXT("ProcessingFacility")) ||
            !ImportSingleShipDefinition(TEXT("AsteroidBase")) ||
            !ImportSingleShipDefinition(TEXT("MiningStation")))
            return 2;

        auto* Context = NewObject<USSWGameInstance>();
        auto* Environment = NewObject<UStarshatterEnvironmentSubsystem>(Context);
        Environment->SSWInstance = Context;
        Environment->GalaxyDataTable = NewObject<UDataTable>(Context);
        Environment->GalaxyDataTable->RowStruct = FS_Galaxy::StaticStruct();
        Environment->RegionsDataTable = NewObject<UDataTable>(Context);
        Environment->RegionsDataTable->RowStruct = FRegion::StaticStruct();
        Environment->LoadGalaxyMap();
        auto* LiveGalaxy = LoadObject<UDataTable>(nullptr, TEXT("/Game/Game/DT_GalaxyMap.DT_GalaxyMap"));
        if (!LiveGalaxy || LiveGalaxy->GetRowStruct() != FS_Galaxy::StaticStruct()) return 2;
        auto* Updated = DuplicateObject<UDataTable>(LiveGalaxy, GetTransientPackage());
        for (const FName Name : Updated->GetRowNames())
        {
            auto* Row = Updated->FindRow<FS_Galaxy>(Name, TEXT("Production import"));
            const auto* Authored = Environment->GalaxyDataTable->FindRow<FS_Galaxy>(Name, TEXT("Production import"), false);
            if (Row && Authored) Row->ProductionSites = Authored->ProductionSites;
        }
        auto* LiveRegions = LoadObject<UDataTable>(nullptr, TEXT("/Game/Game/DT_Region.DT_Region"));
        if (!LiveRegions || LiveRegions->GetRowStruct() != FRegion::StaticStruct()) return 2;
        auto* UpdatedRegions = DuplicateObject<UDataTable>(LiveRegions, GetTransientPackage());
        const auto* Kalon = Environment->RegionsDataTable->FindRow<FRegion>(TEXT("Kalon"), TEXT("Production import"));
        if (!Kalon) return 2;
        UpdatedRegions->AddRow(TEXT("Kalon"), *Kalon);
        return SaveOutputTable(*UpdatedRegions, TEXT("DT_Region"), false) &&
            SaveOutputTable(*Table, TEXT("DT_ProductionRecipes"), false) &&
            SaveOutputTable(*Updated, TEXT("DT_GalaxyMap"), false) ? 0 : 2;
    }

    if (FParse::Param(*Params, TEXT("ShipPricesOnly")))
        return ImportShipCargo(true) ? 0 : 1;

    if (FParse::Param(*Params, TEXT("ShipCargoOnly")))
        return ImportShipCargo() ? 0 : 1;

    auto* Context = NewObject<USSWGameInstance>();

    if (FParse::Param(*Params, TEXT("WriteWorldTables")))
    {
        auto* Environment = NewObject<UStarshatterEnvironmentSubsystem>(Context);
        Environment->SSWInstance = Context;
        Environment->GalaxyDataTable = NewObject<UDataTable>(Context);
        Environment->GalaxyDataTable->RowStruct = FS_Galaxy::StaticStruct();
        Environment->RegionsDataTable = NewObject<UDataTable>(Context);
        Environment->RegionsDataTable->RowStruct = FRegion::StaticStruct();
        Environment->LoadGalaxyMap();
        const auto SaveWorlds = [](const auto& Authored, const TCHAR* Name, const auto& RowType)
        {
            using FBody = typename TDecay<decltype(RowType)>::Type;
            const FString Path = FString::Printf(TEXT("/Game/Game/%s.%s"), Name, Name);
            auto* Active = LoadObject<UDataTable>(nullptr, *Path);
            if (!Active || Active->GetRowStruct() != FBody::StaticStruct() || Authored.IsEmpty()) return false;
            auto* Updated = DuplicateObject<UDataTable>(Active, GetTransientPackage());
            TSet<FString> Applied;
            for (const auto& Pair : Updated->GetRowMap())
            {
                auto& Row = *reinterpret_cast<FBody*>(Pair.Value);
                const FString Key = Row.Name.TrimStartAndEnd();
                if (const FBody* Source = Authored.Find(Key))
                {
                    Row.Population = Source->Population;
                    Row.Production = Source->Production;
                    Row.WorldScapeProfile = Source->WorldScapeProfile;
                    Row.Locations.RemoveAll([](const FNamedBodyLocation& Site) { return Site.Type == ELocationType::City; });
                    for (const auto& Site : Source->Locations)
                        if (Site.Type == ELocationType::City) Row.Locations.Add(Site);
                    Applied.Add(Key);
                }
            }
            for (const auto& Pair : Authored)
                if (!Applied.Contains(Pair.Key)) Updated->AddRow(FName(*Pair.Key), Pair.Value);
            return SaveOutputTable(*Updated, Name, false);
        };
        return SaveWorlds(Environment->PlanetMapByName, TEXT("DT_Planets"), FPlanet{})
            && SaveWorlds(Environment->MoonMapByName, TEXT("DT_Moons"), FMoon{}) ? 0 : 2;
    }

    auto* Weapons = NewObject<UStarshatterWeaponDesignSubsystem>(Context);
    auto* Ships = NewObject<UStarshatterShipDesignSubsystem>(Context);

    auto* SystemTable = NewObject<UDataTable>();
    auto* WeaponTable = NewObject<UDataTable>();
    auto* ShipTable = NewObject<UDataTable>();
    SystemTable->RowStruct = FSystemDesign::StaticStruct();
    WeaponTable->RowStruct = FWeaponDesign::StaticStruct();
    ShipTable->RowStruct = FShipDesign::StaticStruct();
    Weapons->WeaponDesignDataTable = WeaponTable;
    Ships->ShipDesignDataTable = ShipTable;

    if (!UStarshatterSystemDesignSubsystem::ImportDefinitions(*SystemTable)) return 1;
    const FString GameData = FPaths::ProjectContentDir() / TEXT("GameData");
    if (!UStarshatterWeaponDesignSubsystem::ImportJsonCatalog(*WeaponTable, GameData / TEXT("Weapons/wep.json")) ||
        !UStarshatterShipDesignSubsystem::ImportJsonCatalog(*ShipTable, GameData / TEXT("Ships/ships.json"))) return 1;
    Weapons->ReadWeaponDesignData();

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
    UDataTable* StoryMissions = Stage(TEXT("DT_StoryMissions"), true, true);
    if (!StoryMissions || StoryMissions->GetRowStruct() != FS_CampaignMission::StaticStruct())
    {
        UE_LOG(LogTemp, Error, TEXT("DT_StoryMissions must use FS_CampaignMission rows."));
        return 1;
    }
    if (!Campaign->ImportStoryMissions(*StoryMissions))
    {
        UE_LOG(LogTemp, Error, TEXT("Story mission DEF validation failed."));
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
        UE_LOG(LogTemp, Warning, TEXT("No sys.json component template for design key '%s'."), *Key.ToString());

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
            if (!SaveOutputTable(*Table.Table, Table.Name))
                return 2;
    }
    if (FParse::Param(*Params, TEXT("WriteStoryMissions")) &&
        !SaveOutputTable(*StoryMissions, TEXT("DT_StoryMissions"), false)) return 2;
    return 0;
#else
    return 1;
#endif
}


