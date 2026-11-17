#include "DesignTableJson.h"

#include "Dom/JsonObject.h"
#include "Engine/DataTable.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"

namespace
{
    constexpr TCHAR RowField[] = TEXT("Row");

    bool WriteRows(const TArray<TSharedPtr<FJsonValue>>& Rows, FString& Json)
    {
        Json.Reset();
        auto Writer = TJsonWriterFactory<>::Create(&Json);
        return FJsonSerializer::Serialize(Rows, Writer);
    }

#if WITH_EDITOR
    void PreserveFloatFields(const UScriptStruct& Type, const void* Data, FJsonObject& Object);

    void PreserveFloatValue(const FProperty& Property, const void* Data, TSharedPtr<FJsonValue>& Value)
    {
        if (const auto* Float = CastField<FFloatProperty>(&Property))
            Value = MakeShared<FJsonValueNumber>(Float->GetFloatingPointPropertyValue(Data));
        else if (const auto* Struct = CastField<FStructProperty>(&Property); Struct && Value->Type == EJson::Object)
            PreserveFloatFields(*Struct->Struct, Data, *Value->AsObject());
        else if (const auto* Array = CastField<FArrayProperty>(&Property); Array && Value->Type == EJson::Array)
        {
            auto Items = Value->AsArray();
            FScriptArrayHelper Source(Array, Data);
            for (int32 Index = 0; Index < Source.Num(); ++Index)
                if (Items.IsValidIndex(Index))
                    PreserveFloatValue(*Array->Inner, Source.GetRawPtr(Index), Items[Index]);
            Value = MakeShared<FJsonValueArray>(MoveTemp(Items));
        }
    }

    void PreserveFloatFields(const UScriptStruct& Type, const void* Data, FJsonObject& Object)
    {
        // DataTable export emits float values with six digits. Reinsert their
        // full precision, including values in nested components and arrays.
        for (TFieldIterator<FProperty> Property(&Type); Property; ++Property)
            if (auto* Value = Object.Values.Find(Property->GetName()))
                PreserveFloatValue(**Property, Property->ContainerPtrToValuePtr<void>(Data), *Value);
    }
#endif
}

bool DesignTableJson::Import(UDataTable& Table, UScriptStruct& RowType, const FString& Path)
{
    FString Json;
    TArray<TSharedPtr<FJsonValue>> Rows;
    if (!FFileHelper::LoadFileToString(Json, *Path) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Rows) || Rows.IsEmpty())
        return false;

    TSet<FName> SeenKeys;
    TArray<FName> Keys;
    TArray<FName> ImportKeys;
    for (const auto& Value : Rows)
    {
        const TSharedPtr<FJsonObject>* Object = nullptr;
        FString Key;
        if (!Value->TryGetObject(Object) || !(*Object)->TryGetStringField(RowField, Key) ||
            Key.IsEmpty() || Key != Key.TrimStartAndEnd() || Key.Len() >= NAME_SIZE)
            return false;

        const FName Name(*Key);
        if (Name.IsNone() || SeenKeys.Contains(Name)) return false;
        SeenKeys.Add(Name);
        Keys.Add(Name);

        // DataTable's JSON reader sanitizes keys. Use temporary keys to
        // preserve the exact original names, even when two would sanitize alike.
        const FString ImportKey = FString::Printf(TEXT("DesignRow%d"), Keys.Num());
        ImportKeys.Add(FName(*ImportKey));
        (*Object)->SetStringField(RowField, ImportKey);
    }

    auto* Scratch = NewObject<UDataTable>();
    Scratch->RowStruct = &RowType;
    Scratch->ImportKeyField = RowField;
    if (!WriteRows(Rows, Json)) return false;
    const auto Problems = Scratch->CreateTableFromJSONString(Json);
    for (const auto& Problem : Problems)
        UE_LOG(LogTemp, Error, TEXT("%s: %s"), *Path, *Problem);
    if (!Problems.IsEmpty() || Scratch->GetRowMap().Num() != Keys.Num()) return false;

    Table.EmptyTable();
    Table.RowStruct = &RowType;
    for (int32 Index = 0; Index < Keys.Num(); ++Index)
        Table.AddRow(Keys[Index], *reinterpret_cast<const FTableRowBase*>(Scratch->FindRowUnchecked(ImportKeys[Index])));
    return true;
}

#if WITH_EDITOR
bool DesignTableJson::Export(UDataTable& Table, FString& Json)
{
    const FString PreviousKey = Table.ImportKeyField;
    Table.ImportKeyField = RowField;
    const FString InitialJson = Table.GetTableAsJSON(EDataTableExportFlags::UseJsonObjectsForStructs);
    Table.ImportKeyField = PreviousKey;

    TArray<TSharedPtr<FJsonValue>> Rows;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(InitialJson), Rows) || Rows.IsEmpty())
        return false;
    for (const auto& Row : Rows)
    {
        auto Object = Row->AsObject();
        const auto* Source = Table.FindRowUnchecked(FName(*Object->GetStringField(RowField)));
        if (!Source) return false;
        PreserveFloatFields(*Table.GetRowStruct(), Source, *Object);
    }
    return WriteRows(Rows, Json);
}

bool DesignTableJson::Compare(const UDataTable& Expected, const UDataTable& Actual)
{
    const auto* Type = Expected.GetRowStruct();
    if (!Type || Type != Actual.GetRowStruct() || Expected.GetRowMap().Num() != Actual.GetRowMap().Num())
    {
        UE_LOG(LogTemp, Error, TEXT("Design table schema or row count mismatch: %d vs %d"),
            Expected.GetRowMap().Num(), Actual.GetRowMap().Num());
        return false;
    }
    for (const auto& Row : Expected.GetRowMap())
    {
        const auto* Other = Actual.FindRowUnchecked(Row.Key);
        if (Other && Type->CompareScriptStruct(Row.Value, Other, 0)) continue;
        UE_LOG(LogTemp, Error, TEXT("Design table mismatch in row %s"), *Row.Key.ToString());
        if (Other)
            for (TFieldIterator<FProperty> Field(Type); Field; ++Field)
                if (!Field->Identical_InContainer(Row.Value, Other))
                    UE_LOG(LogTemp, Error, TEXT("Changed field: %s"), *Field->GetName());
        return false;
    }
    return true;
}
#endif
