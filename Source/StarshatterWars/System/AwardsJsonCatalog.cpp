#include "AwardsJsonCatalog.h"
#include <type_traits>
#include "GameStructs.h"
#include "Engine/DataTable.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "SSWModFiles.h"

namespace
{
bool CorrectTables(const UDataTable& Ranks, const UDataTable& Medals, FString& Error)
{
    if (Ranks.GetRowStruct() != FRankInfo::StaticStruct() || Medals.GetRowStruct() != FMedalInfo::StaticStruct())
    { Error = TEXT("Awards require the existing FRankInfo and FMedalInfo row types"); return false; }
    return true;
}

template<class Row> bool ReadRows(const FJsonObject& Object, const TCHAR* Field, TArray<Row>& Rows, FString& Error)
{
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Object.TryGetArrayField(Field, Values) || !Values || Values->IsEmpty() || Values->Num() > 4096)
    { Error = FString::Printf(TEXT("Missing, empty or oversized awards array: %s"), Field); return false; }
    TSet<int32> Ids; TSet<FName> Names;
    for (const auto& Value : *Values)
    {
        if (!Value || Value->Type != EJson::Object)
        { Error = TEXT("Awards entries must be objects"); return false; }
        Row Item;
        FText Failure;
        if (!FJsonObjectConverter::JsonObjectToUStruct(Value->AsObject().ToSharedRef(), &Item, 0, 0, true, &Failure))
        { Error = TEXT("Incomplete/invalid awards row: ") + Failure.ToString(); return false; }
        int32 Id = 0;
        if constexpr (std::is_same_v<Row, FRankInfo>) Id = Item.RankId;
        else Id = Item.MedalId;
        const FName Name(*Item.Name);
        if (Id < 0 || Name.IsNone() || Item.Name != Item.Name.TrimStartAndEnd() || Ids.Contains(Id) || Names.Contains(Name))
        { Error = TEXT("Empty name, invalid ID or duplicate award: ") + Item.Name; return false; }
        if constexpr (std::is_same_v<Row, FMedalInfo>)
        {
            if (Id == 0 || (Id & (Id - 1)) != 0)
            { Error = TEXT("Medal IDs must retain their authored single-bit flag: ") + Item.Name; return false; }
        }
        Ids.Add(Id); Names.Add(Name); Rows.Add(MoveTemp(Item));
    }
    return true;
}

template<class Row> bool WriteRows(const UDataTable& Table, TArray<TSharedPtr<FJsonValue>>& Values)
{
    TArray<FName> Keys = Table.GetRowNames();
    Keys.Sort([&](FName A, FName B)
    {
        const auto* Left = Table.FindRow<Row>(A, TEXT("Awards export"));
        const auto* Right = Table.FindRow<Row>(B, TEXT("Awards export"));
        if constexpr (std::is_same_v<Row, FRankInfo>) return Left->RankId < Right->RankId;
        else return Left->MedalId < Right->MedalId;
    });
    for (FName Key : Keys)
    {
        const auto* Item = Table.FindRow<Row>(Key, TEXT("Awards export"));
        auto Object = MakeShared<FJsonObject>();
        if (!Item || FName(*Item->Name) != Key || !FJsonObjectConverter::UStructToJsonObject(Row::StaticStruct(), Item, Object, 0, 0)) return false;
        Values.Add(MakeShared<FJsonValueObject>(Object));
    }
    return true;
}
}

bool AwardsJsonCatalog::Load(const FString& Path, TArray<FRankInfo>& Ranks, TArray<FMedalInfo>& Medals, FString& Error)
{
    Error.Reset();
    TArray<uint8> Bytes;
    if ((!SSWModFiles::Read(Path, Bytes) && (!FPaths::FileExists(Path) || !FFileHelper::LoadFileToArray(Bytes, *Path))) || Bytes.IsEmpty() || Bytes.Num() > 4 * 1024 * 1024)
    { Error = TEXT("Unreadable/oversized awards JSON: ") + Path; return false; }
    FString Text; FFileHelper::BufferToString(Text, Bytes.GetData(), Bytes.Num());
    TSharedPtr<FJsonObject> Object;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Object) || !Object)
    { Error = TEXT("Invalid awards JSON: ") + Path; return false; }
    double Version = 0;
    if (!Object->TryGetNumberField(TEXT("schemaVersion"), Version) || Version != 1)
    { Error = TEXT("Unsupported awards schemaVersion"); return false; }
    TArray<FRankInfo> RankRows; TArray<FMedalInfo> MedalRows;
    if (!ReadRows(*Object, TEXT("ranks"), RankRows, Error) || !ReadRows(*Object, TEXT("medals"), MedalRows, Error)) return false;
    Ranks = MoveTemp(RankRows); Medals = MoveTemp(MedalRows);
    return true;
}

bool AwardsJsonCatalog::Import(const FString& Path, UDataTable& Ranks, UDataTable& Medals, FString& Error)
{
    if (!CorrectTables(Ranks, Medals, Error)) return false;
    TArray<FRankInfo> RankRows; TArray<FMedalInfo> MedalRows;
    if (!Load(Path, RankRows, MedalRows, Error)) return false;
    // Legacy conversion/test adapter only; gameplay uses value arrays.
    Ranks.EmptyTable(); Medals.EmptyTable();
    for (const auto& Row : RankRows) Ranks.AddRow(FName(*Row.Name), Row);
    for (const auto& Row : MedalRows) Medals.AddRow(FName(*Row.Name), Row);
    return true;
}

bool AwardsJsonCatalog::Export(const FString& Path, const UDataTable& Ranks, const UDataTable& Medals, FString& Error)
{
    Error.Reset();
    if (!CorrectTables(Ranks, Medals, Error)) return false;
    TArray<TSharedPtr<FJsonValue>> RankRows, MedalRows;
    if (!WriteRows<FRankInfo>(Ranks, RankRows) || !WriteRows<FMedalInfo>(Medals, MedalRows))
    { Error = TEXT("Cannot serialize complete native awards rows"); return false; }
    auto Object = MakeShared<FJsonObject>(); Object->SetNumberField(TEXT("schemaVersion"), 1);
    Object->SetArrayField(TEXT("ranks"), RankRows); Object->SetArrayField(TEXT("medals"), MedalRows);
    FString Text;
    if (!FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Text)) ||
        !FFileHelper::SaveStringToFile(Text, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    { Error = TEXT("Cannot write awards JSON: ") + Path; return false; }
    return true;
}
