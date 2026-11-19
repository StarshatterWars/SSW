#include "CargoCatalogSubsystem.h"
#include "JsonRecordCatalog.h"

#include "GameStructs.h"
#include "Engine/GameInstance.h"
#include "Ship.h"
#include "StarshatterAssetRegistrySubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void UCargoCatalogSubsystem::LoadCatalog()
{
    TMap<FName, FS_CargoCatalogRow> Candidate;
    FString Error;
    const FString Path = FPaths::ProjectContentDir() / TEXT("GameData/Cargo/cargo.json");
    if (!SSWJsonRecords::Load(Path, Candidate, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("%s"), *Error);
        return;
    }
    for (const auto& Pair : Candidate)
    {
        const auto& Row = Pair.Value;
        if (Row.Description.TrimStartAndEnd().IsEmpty() || !FMath::IsFinite(Row.Size) || Row.Size < 0.0 ||
            !FMath::IsFinite(Row.Value) || Row.Value < 0.0)
        {
            UE_LOG(LogTemp, Error, TEXT("Invalid cargo record: %s"), *Pair.Key.ToString());
            return;
        }
    }
    Catalog = MoveTemp(Candidate);
    UE_LOG(LogTemp, Display, TEXT("Loaded %d cargo definitions directly from JSON"), Catalog.Num());
}

bool UCargoCatalogSubsystem::ImportDefinitions(UDataTable& Table)
{
    const FString Path = FPaths::ProjectContentDir() / TEXT("GameData/Cargo/cargo.json");
    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *Path)) return false;

    Table.RowStruct = FS_CargoCatalogRow::StaticStruct();
    const TArray<FString> Problems = Table.CreateTableFromJSONString(Json);
    for (const FString& Problem : Problems)
        UE_LOG(LogTemp, Warning, TEXT("%s: %s"), *Path, *Problem);

    if (!Problems.IsEmpty() || Table.GetRowMap().IsEmpty()) return false;
    for (const auto& Pair : Table.GetRowMap())
    {
        const auto& Row = *reinterpret_cast<const FS_CargoCatalogRow*>(Pair.Value);
        if (Row.Description.TrimStartAndEnd().IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: missing description for '%s'"), *Path, *Pair.Key.ToString());
            return false;
        }
        if (!FMath::IsFinite(Row.Size) || Row.Size < 0.0 || !FMath::IsFinite(Row.Value) || Row.Value < 0.0)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: invalid mass or unit value for '%s'"), *Path, *Pair.Key.ToString());
            return false;
        }
    }
    UE_LOG(LogTemp, Display, TEXT("Imported %d cargo definitions from %s"), Table.GetRowMap().Num(), *Path);
    return true;
}

const FS_CargoCatalogRow* UCargoCatalogSubsystem::FindCargo(const FString& Item, const FString& Unit) const
{
    const auto* Definition = Catalog.Find(DataTableUtils::MakeValidName(Item.TrimStartAndEnd()));
    return Definition && Definition->Unit.Equals(Unit, ESearchCase::IgnoreCase) ? Definition : nullptr;
}

void UCargoCatalogSubsystem::ForEachDefinition(TFunctionRef<void(FName, const FS_CargoCatalogRow&)> Visit) const
{
    for (const auto& Pair : Catalog) Visit(Pair.Key, Pair.Value);
}

bool UCargoCatalogSubsystem::DescribeCargo(const Ship& Vessel, FS_Cargo& Row) const
{
    const auto* Load = Vessel.GetCargoManifest().GetPtrOrNull();
    if (!Load) return false;

    const bool bByMass = Load->Unit.Equals(TEXT("t"), ESearchCase::IgnoreCase);
    const double Quantity = bByMass ? Vessel.GetMinedOreTonnes() : Load->Quantity;
    if (Quantity <= 0.0) return false;

    Row = FS_Cargo{};
    Row.Name = Load->Item;
    Row.DisplayName = Load->Item;
    Row.Unit = Load->bPassengers ? TEXT("berths") : Load->Unit;
    Row.Quantity = Quantity;
    Row.Size = bByMass ? 1.0 : 0.0;
    Row.Type = Load->bPassengers ? ECargoType::Passengers :
        Load->FinishedItem != ESSWFinishedItem::None ? ECargoType::FinishedGoods :
        bByMass ? ECargoType::Bulk : ECargoType::General;

    if (const auto* Definition = FindCargo(Load->Item, Load->Unit))
    {
        if (!Definition->DisplayName.IsEmpty()) Row.DisplayName = Definition->DisplayName;
        Row.Image = Definition->Image;
        Row.Rarity = Definition->Rarity;
        Row.bIllicit = Definition->bIllicit;
        Row.Type = Definition->Type;
        Row.Size = Definition->Size;
        Row.Value = Definition->Value;
    }
    Row.UpdateTotalValue();
    return true;
}
