#pragma once
#include "CoreMinimal.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// JSON catalog arrays use Name as a stable identifier, not a DataTable row.
// Parse into a candidate so malformed replacements never erase a working catalog.
namespace SSWJsonRecords
{
template<class Row>
bool Load(const FString& Path, TMap<FName, Row>& Out, FString& Error)
{
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *Path))
    { Error = TEXT("Cannot read JSON catalog: ") + Path; return false; }
    TArray<TSharedPtr<FJsonValue>> Values;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Values) || Values.IsEmpty())
    { Error = TEXT("Expected a nonempty JSON catalog array: ") + Path; return false; }
    TMap<FName, Row> Candidate;
    for (const auto& Value : Values)
    {
        const auto Object = Value.IsValid() && Value->Type == EJson::Object ? Value->AsObject() : nullptr;
        FString Name;
        if (!Object || !Object->TryGetStringField(TEXT("Name"), Name) || Name.TrimStartAndEnd().IsEmpty())
        { Error = TEXT("JSON record requires Name: ") + Path; return false; }
        // Preserve legacy identifiers used by existing inventory/market saves.
        FString Key;
        const FString Invalid(INVALID_NAME_CHARACTERS);
        for (TCHAR Ch : Name.TrimStartAndEnd())
            if (!Invalid.Contains(FString::Chr(Ch))) Key.AppendChar(Ch);
        const FName Id(*Key);
        if (Id.IsNone()) { Error = TEXT("Invalid JSON record name: ") + Name; return false; }
        Row Record;
        if (Candidate.Contains(Id) || !FJsonObjectConverter::JsonObjectToUStruct(Object.ToSharedRef(), &Record))
        { Error = TEXT("Duplicate or invalid JSON record: ") + Name; return false; }
        Candidate.Add(Id, MoveTemp(Record));
    }
    Out = MoveTemp(Candidate);
    return true;
}
}
