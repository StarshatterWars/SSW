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
#include "SkillsSubsystem.h"
#include "StarshatterSystemDesignSubsystem.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "UObject/SavePackage.h"
#include "SSWGameInstance.h"
#include "StarshatterGameDataSubsystem.h"
#include "CampaignJsonCatalog.h"
#include "MissionListJson.h"
#include "AwardsJsonCatalog.h"
#include "StarshatterEnvironmentSubsystem.h"
#include "SSWCombatGroupSubsystem.h"
#include "ShipVisualCatalog.h"
#include "ShipActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "AssetCompilingManager.h"
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

    auto* SkillContext=NewObject<UGameInstance>();
    auto* Skills=NewObject<USkillsSubsystem>(SkillContext);
    if (!Skills->LoadCatalog())return false;
    for (const auto& Row : Table.GetRowMap())
    {
        const auto& Recipe = *reinterpret_cast<const FSSWProductionRecipe*>(Row.Value);
        for (const auto& Requirement:Recipe.RequiredSkillLevels)
        {
            const auto* Skill=Skills->GetCatalog().Find(Requirement.Key);
            if (!Skill || Requirement.Value<1 || Requirement.Value>Skill->MaximumLevel)
            { UE_LOG(LogTemp,Error,TEXT("Recipe %s references unavailable skill %s"),*Row.Key.ToString(),*Requirement.Key.ToString());return false; }
        }
        if ((Recipe.Minerals.IsEmpty() && Recipe.Inputs.IsEmpty()) || Recipe.Minerals.Contains(ESSWMineral::None) ||
            !FMath::IsFinite(Recipe.InputTonnes) || !FMath::IsFinite(Recipe.OutputTonnes) ||
            !FMath::IsFinite(Recipe.SecondsPerBatch) || !Recipe.HasValidSkillRequirements() ||
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
    if (FParse::Param(*Params,TEXT("ConvertMissionListsJson")))
    {
        auto* Context=NewObject<USSWGameInstance>();auto* Data=NewObject<UStarshatterGameDataSubsystem>(Context);
        int32 Missions=0,Templates=0;
        for(int32 I=0;I<=5;++I)
        {
            const FString Directory=FPaths::ProjectContentDir()/(I==0?FString(TEXT("GameData/Missions")):FString::Printf(TEXT("GameData/Campaigns/%02d"),I));
            Data->LoadMissionList(Directory+TEXT("/"),false);Data->LoadTemplateList(Directory+TEXT("/"),false);
            FString Error;TArray<FS_CampaignMissionList> ReadMissions;TArray<FS_CampaignTemplateList> ReadTemplates;
            if(!MissionListJson::WriteMissions(Directory,Data->MissionListArray,Error) ||
               !MissionListJson::WriteTemplates(Directory,Data->TemplateListArray,Error) ||
               MissionListJson::ReadMissions(Directory,ReadMissions,Error)!=MissionListJson::EReadResult::Loaded ||
               MissionListJson::ReadTemplates(Directory,ReadTemplates,Error)!=MissionListJson::EReadResult::Loaded)
            {UE_LOG(LogTemp,Error,TEXT("[MissionListsJson] %s"),*Error);return 50;}
            if(ReadMissions.Num()!=Data->MissionListArray.Num() || ReadTemplates.Num()!=Data->TemplateListArray.Num())return 51;
            for(int32 N=0;N<ReadMissions.Num();++N)
                if(!FS_CampaignMissionList::StaticStruct()->CompareScriptStruct(&ReadMissions[N],&Data->MissionListArray[N],0))return 52;
            for(int32 N=0;N<ReadTemplates.Num();++N)
                if(!FS_CampaignTemplateList::StaticStruct()->CompareScriptStruct(&ReadTemplates[N],&Data->TemplateListArray[N],0))return 53;
            Missions+=ReadMissions.Num();Templates+=ReadTemplates.Num();
        }
        UE_LOG(LogTemp,Display,TEXT("[MissionListsJson] %d mission entries, %d template entries; full DEF/JSON parity PASS"),Missions,Templates);return 0;
    }
    if (FParse::Param(*Params,TEXT("ConvertAwardsJson")))
    {
        auto* Context=NewObject<USSWGameInstance>();
        auto* Data=NewObject<UStarshatterGameDataSubsystem>(Context);
        Data->RanksDataTable=NewObject<UDataTable>(Context);Data->RanksDataTable->RowStruct=FRankInfo::StaticStruct();
        Data->MedalsDataTable=NewObject<UDataTable>(Context);Data->MedalsDataTable->RowStruct=FMedalInfo::StaticStruct();
        Data->LoadAwardTables(false);
        if (Data->RanksDataTable->GetRowMap().IsEmpty() || Data->MedalsDataTable->GetRowMap().IsEmpty()) return 40;
        const FString Path=FPaths::ProjectContentDir()/TEXT("GameData/Awards/awards.json");
        FString Error;
        if (!AwardsJsonCatalog::Export(Path,*Data->RanksDataTable,*Data->MedalsDataTable,Error))
        { UE_LOG(LogTemp,Error,TEXT("[AwardsJson] %s"),*Error);return 41; }
        auto* Ranks=NewObject<UDataTable>(Context);Ranks->RowStruct=FRankInfo::StaticStruct();
        auto* Medals=NewObject<UDataTable>(Context);Medals->RowStruct=FMedalInfo::StaticStruct();
        if (!AwardsJsonCatalog::Import(Path,*Ranks,*Medals,Error))
        { UE_LOG(LogTemp,Error,TEXT("[AwardsJson] %s"),*Error);return 42; }
        for (const auto* Pair : {Data->RanksDataTable,Data->MedalsDataTable})
        {
            const auto* Imported=Pair->GetRowStruct()==FRankInfo::StaticStruct()?Ranks:Medals;
            if (Pair->GetRowMap().Num()!=Imported->GetRowMap().Num()) return 43;
            for (const auto& Row:Pair->GetRowMap())
            {
                const auto* Other=Imported->GetRowMap().Find(Row.Key);
                if (!Other || !Pair->GetRowStruct()->CompareScriptStruct(Row.Value,*Other,0)) return 44;
            }
        }
        UE_LOG(LogTemp,Display,TEXT("[AwardsJson] %d ranks, %d medals; every legacy field preserved"),Ranks->GetRowMap().Num(),Medals->GetRowMap().Num());
        return 0;
    }
    if(FParse::Param(*Params,TEXT("ConvertCampaignJson")) || FParse::Param(*Params,TEXT("WriteCampaignJsonTables")))
    {
        const bool Export=FParse::Param(*Params,TEXT("ConvertCampaignJson"));
        auto* Context=NewObject<USSWGameInstance>();
        auto* Data=NewObject<UStarshatterGameDataSubsystem>(Context);
        Data->CampaignDataTable=NewObject<UDataTable>(Context);Data->CampaignDataTable->RowStruct=FS_Campaign::StaticStruct();
        Data->CampaignActionDataTable=NewObject<UDataTable>(Context);Data->CampaignActionDataTable->RowStruct=FS_CampaignAction::StaticStruct();
        Data->ZonesDataTable=NewObject<UDataTable>(Context);Data->ZonesDataTable->RowStruct=FS_CampaignZone::StaticStruct();
        int32 Campaigns=0,Actions=0,Zones=0;
        for(int32 I=1;I<=5;++I)
        {
            const FString Directory=FPaths::ProjectContentDir()/FString::Printf(TEXT("GameData/Campaigns/%02d"),I);
            const FTCHARToUTF8 Path(*(Directory/TEXT("campaign.def")));
            Data->LoadCampaignData(Path.Get(),true,!Export);
            FS_Campaign* Row=nullptr;
            for(const auto& Pair:Data->CampaignDataTable->GetRowMap())
                if(reinterpret_cast<FS_Campaign*>(Pair.Value)->Index==I-1)Row=reinterpret_cast<FS_Campaign*>(Pair.Value);
            if(!Row)return 30;
            if(Export)
            {
                FString Error;const FS_Campaign Original=*Row;FS_Campaign RoundTrip=*Row;
                if(!CampaignJsonCatalog::Export(Directory,Original,Error) || !CampaignJsonCatalog::Apply(Directory,RoundTrip,nullptr,nullptr,Error))
                {UE_LOG(LogTemp,Error,TEXT("[CampaignJson] %s"),*Error);return 31;}
                if(Original.Action.Num()!=RoundTrip.Action.Num() || Original.Zone.Num()!=RoundTrip.Zone.Num())return 32;
                for(int32 N=0;N<Original.Action.Num();++N)
                    if(!FS_CampaignAction::StaticStruct()->CompareScriptStruct(&Original.Action[N],&RoundTrip.Action[N],0))return 33;
                for(int32 N=0;N<Original.Zone.Num();++N)
                    if(!FS_CampaignZone::StaticStruct()->CompareScriptStruct(&Original.Zone[N],&RoundTrip.Zone[N],0))return 34;
            }
            ++Campaigns;Actions+=Row->Action.Num();Zones+=Row->Zone.Num();
        }
        UE_LOG(LogTemp,Display,TEXT("[CampaignJson] %d campaigns, %d actions, %d zones; %s"),Campaigns,Actions,Zones,Export?TEXT("DEF/JSON parity PASS"):TEXT("JSON ingestion complete"));
        if(Export)return 0;
        return SaveOutputTable(*Data->CampaignDataTable,TEXT("DT_Campaign"),false) &&
            SaveOutputTable(*Data->CampaignActionDataTable,TEXT("DT_CampaignAction"),false) &&
            SaveOutputTable(*Data->ZonesDataTable,TEXT("DT_CampaignZone"),false)?0:35;
    }
    if (FParse::Param(*Params, TEXT("ConvertMissionJson")))
    {
        auto* Context = NewObject<USSWGameInstance>();
        auto* Data = NewObject<UStarshatterGameDataSubsystem>(Context);
        return Data->ConvertPlayableMissionsToJson() ? 0 : 1;
    }
    if(FParse::Param(*Params,TEXT("PreparePropulsionMeshes")))
    {
        auto* Catalog=NewObject<UDataTable>();
        if(!UStarshatterShipDesignSubsystem::ImportJsonCatalog(*Catalog,FPaths::ProjectContentDir()/TEXT("GameData/Ships/ships.json")))return 1;
        TMap<UStaticMesh*,bool> Required;int32 Updated=0,Retained=0;
        for(const auto& Entry:ShipVisualCatalog::Entries())
        {
            auto* Class=ShipVisualCatalog::Resolve(Entry.Definition,Entry.Model);
            auto* Actor=Class?Cast<AShipActor>(Class->GetDefaultObject()):nullptr;
            UStaticMesh* Mesh=Actor && Actor->HullMesh?Actor->HullMesh->GetStaticMesh():nullptr;
            if(!Mesh || !Mesh->GetOutermost()->GetName().StartsWith(TEXT("/Game/")))continue;
            const auto* Row=Catalog->FindRow<FShipDesign>(FName(Entry.Definition),TEXT("Propulsion CPU geometry"),false);
            if(!Row)Row=Catalog->FindRow<FShipDesign>(FName(Entry.Design),TEXT("Propulsion CPU geometry"),false);
            const bool Needed=Row && ((Actor->bEnableMainEngineEmitters && Row->Drive.ContainsByPredicate([](const auto& Drive){return !Drive.Ports.IsEmpty();})) ||
                (Actor->bEnableThrusterEmitters && Row->Thruster.ContainsByPredicate([](const auto& Thruster){return !Thruster.Ports.IsEmpty();})));
            Required.FindOrAdd(Mesh)|=Needed;
        }
        for(const auto& Pair:Required)
        {
            UStaticMesh* Mesh=Pair.Key;const bool Needed=Pair.Value;if(Needed)++Retained;
            if(Mesh->bAllowCPUAccess==Needed)continue;
            const FString Filename=FPackageName::LongPackageNameToFilename(Mesh->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());
            const FString Backup=FPaths::ProjectDir()/TEXT("Backup/FleetHullPortCPU")/(Mesh->GetOutermost()->GetName().RightChop(6)+FPackageName::GetAssetPackageExtension());
            // Restore only flags changed by this migration; preserve pre-existing CPU readers.
            if(!Needed && !IFileManager::Get().FileExists(*Backup))continue;
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(Backup),true);
            if(!IFileManager::Get().FileExists(*Backup) && IFileManager::Get().Copy(*Backup,*Filename)!=COPY_OK)return 1;
            Mesh->Modify();Mesh->bAllowCPUAccess=Needed;Mesh->PostEditChange();FAssetCompilingManager::Get().FinishAllCompilation();Mesh->GetOutermost()->MarkPackageDirty();
            FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
            if(!UPackage::SavePackage(Mesh->GetOutermost(),Mesh,*Filename,Args))return 1;
            ++Updated;
        }
        UE_LOG(LogTemp,Display,TEXT("Propulsion mesh preparation: %d powered hull meshes retained, %d flags saved"),Retained,Updated);return 0;
    }
    if (FParse::Param(*Params, TEXT("SyncPropulsionFromDefinitions")))
    {
        const FString Directory=FPaths::ProjectContentDir()/TEXT("GameData/Ships");
        auto* Catalog=NewObject<UDataTable>();
        if(!UStarshatterShipDesignSubsystem::ImportJsonCatalog(*Catalog,Directory/TEXT("ships.json")))return 1;
        auto* Parser=NewObject<UStarshatterShipDesignSubsystem>(NewObject<UGameInstance>());
        auto* Parsed=NewObject<UDataTable>();Parsed->RowStruct=FShipDesign::StaticStruct();Parser->ShipDesignDataTable=Parsed;
        TMap<FName,FShipDesign> Sources;
        TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(Directory/TEXT("*.def")),true,false);
        for(const auto& File:Files)
        {
            Parsed->EmptyTable();Parser->LoadShipDesign(TCHAR_TO_UTF8(*(Directory/File)));
            for(const auto& Pair:Parsed->GetRowMap())
            {
                const auto& Row=*reinterpret_cast<const FShipDesign*>(Pair.Value);
                Sources.Add(Pair.Key,Row);Sources.Add(FName(*FPaths::GetBaseFilename(File)),Row);
            }
        }
        int32 Updated=0;
        for(const auto& Pair:Catalog->GetRowMap())
        {
            auto& Row=*reinterpret_cast<FShipDesign*>(Pair.Value);
            const bool HasPorts=Row.Drive.ContainsByPredicate([](const FShipDrive& Drive){return !Drive.Ports.IsEmpty();}) ||
                Row.Thruster.ContainsByPredicate([](const auto& Thruster){return !Thruster.Ports.IsEmpty();});
            if(!HasPorts)continue;
            const auto* Source=Sources.Find(Pair.Key);
            if(!Source || Source->Model!=Row.Model)
            {
                UE_LOG(LogTemp,Error,TEXT("Propulsion migration has no matching DEF model for %s"),*Pair.Key.ToString());return 1;
            }
            if(Row.Drive.Num()>Source->Drive.Num() || Row.Thruster.Num()>Source->Thruster.Num())
            {UE_LOG(LogTemp,Error,TEXT("Propulsion system count mismatch for %s"),*Pair.Key.ToString());return 1;}
            for(int32 I=0;I<Source->Drive.Num();++I)
                if(Row.Drive.IsValidIndex(I)){Row.Drive[I].Ports=Source->Drive[I].Ports;Row.Drive[I].Type=Source->Drive[I].Type;}else Row.Drive.Add(Source->Drive[I]);
            for(int32 I=0;I<Source->Thruster.Num();++I)
                if(Row.Thruster.IsValidIndex(I)){Row.Thruster[I].Ports=Source->Thruster[I].Ports;Row.Thruster[I].Type=Source->Thruster[I].Type;}else Row.Thruster.Add(Source->Thruster[I]);
            if(Row.Navlight.IsEmpty() && !Source->Navlight.IsEmpty())Row.Navlight=Source->Navlight;
            ++Updated;
        }
        FString Json;
        if(!DesignTableJson::Export(*Catalog,Json) || !FFileHelper::SaveStringToFile(Json,*(Directory/TEXT("ships.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))return 1;
        if(!SaveOutputTable(*Catalog,TEXT("DT_ShipDesign"),false))return 1;
        UE_LOG(LogTemp,Display,TEXT("Propulsion catalog synchronized: %d rows; other ship data preserved"),Updated);
        return 0;
    }
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

    if (FParse::Param(*Params, TEXT("WriteSingleMissions")))
    {
        auto* Context = NewObject<USSWGameInstance>();
        auto* Data = NewObject<UStarshatterGameDataSubsystem>(Context);
        Data->SingleMissionsDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/Game/DT_SingleMissions.DT_SingleMissions"));
        if (!Data->SingleMissionsDataTable) return 1;
        Data->SingleMissionsDataTable = DuplicateObject<UDataTable>(Data->SingleMissionsDataTable, GetTransientPackage());
        Data->LoadSingleMissions();
        if (Data->GetSingleMissionScenarios().Num() != 40) return 1;
        return SaveOutputTable(*Data->SingleMissionsDataTable, TEXT("DT_SingleMissions"), false) ? 0 : 1;
    }

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


    if (FParse::Param(*Params, TEXT("WriteUpdatedMissionTables")))
    {
        const TSet<FString> Requested={TEXT("DT_Campaign"),TEXT("DT_Cutscene"),TEXT("DT_SingleMissions"),TEXT("DT_StoryMissions"),TEXT("DT_MultiplayerMissions"),TEXT("DT_ShipDesign")};
        for (const auto& Table:Tables)
            if (Requested.Contains(Table.Name) && !SaveOutputTable(*Table.Table,Table.Name,false)) return 2;
    }
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


