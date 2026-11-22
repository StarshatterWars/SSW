#include "GalaxyJsonCatalog.h"
#include "GameStructs.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "SSWModFiles.h"
namespace
{
template<class T> bool Read(const FJsonObject& Root,const TCHAR* Field,TArray<T>& Out,FString& Error)
{
    const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
    if(!Root.TryGetArrayField(Field,Rows) || !Rows || Rows->IsEmpty() || Rows->Num()>65536)
    {Error=FString::Printf(TEXT("Invalid galaxy array: %s"),Field);return false;}
    TSet<FName> Names;
    for(const auto& Value:*Rows)
    {
        T Item;FText Failure;
        if(!Value || Value->Type!=EJson::Object || !FJsonObjectConverter::JsonObjectToUStruct(Value->AsObject().ToSharedRef(),&Item,0,0,true,&Failure))
        {Error=TEXT("Invalid/incomplete galaxy row: ")+Failure.ToString();return false;}
        const FName Key(*Item.Name);
        if(Key.IsNone() || Item.Name!=Item.Name.TrimStartAndEnd() || Names.Contains(Key))
        {Error=TEXT("Empty or duplicate galaxy name: ")+Item.Name;return false;}
        Names.Add(Key);Out.Add(MoveTemp(Item));
    }
    return true;
}
template<class T> bool Write(const TArray<T>& Rows,TArray<TSharedPtr<FJsonValue>>& Values)
{
    for(const auto& Row:Rows)
    {
        auto Object=MakeShared<FJsonObject>();
        if(!FJsonObjectConverter::UStructToJsonObject(T::StaticStruct(),&Row,Object,0,0))return false;
        Values.Add(MakeShared<FJsonValueObject>(Object));
    }
    return true;
}
}
bool GalaxyJsonCatalog::Parse(const FString& Json,TArray<FS_Galaxy>& Systems,TArray<FRegion>& Regions,FString& Error)
{
    Error.Reset();TSharedPtr<FJsonObject> Root;double Version=0;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root) || !Root || !Root->TryGetNumberField(TEXT("schemaVersion"),Version) || Version!=1)
    {Error=TEXT("Invalid galaxy JSON or unsupported schemaVersion");return false;}
    TArray<FS_Galaxy> NewSystems;TArray<FRegion> NewRegions;
    if(!Read(*Root,TEXT("systems"),NewSystems,Error) || !Read(*Root,TEXT("regions"),NewRegions,Error))return false;
    TSet<FName> Names;for(const auto& System:NewSystems)Names.Add(FName(*System.Name));
    for(const auto& System:NewSystems)
    {
        if(System.Location.ContainsNaN() || System.Stellar.IsEmpty())
        {Error=TEXT("Invalid system position or missing star: ")+System.Name;return false;}
        for(const auto& Link:System.Link)if(!Names.Contains(FName(*Link)))
        {Error=TEXT("Unknown system link: ")+System.Name+TEXT(" -> ")+Link;return false;}
    }
    Systems=MoveTemp(NewSystems);Regions=MoveTemp(NewRegions);return true;
}
bool GalaxyJsonCatalog::Load(const FString& Path,TArray<FS_Galaxy>& Systems,TArray<FRegion>& Regions,FString& Error)
{
    TArray<uint8> Bytes;
    if((!SSWModFiles::Read(Path,Bytes) && !FFileHelper::LoadFileToArray(Bytes,*Path)) || Bytes.IsEmpty() || Bytes.Num()>64*1024*1024)
    {Error=TEXT("Unreadable or oversized galaxy JSON: ")+Path;return false;}
    FString Json;FFileHelper::BufferToString(Json,Bytes.GetData(),Bytes.Num());return Parse(Json,Systems,Regions,Error);
}
bool GalaxyJsonCatalog::Save(const FString& Path,const TArray<FS_Galaxy>& Systems,const TArray<FRegion>& Regions,FString& Error)
{
    auto Root=MakeShared<FJsonObject>();Root->SetNumberField(TEXT("schemaVersion"),1);
    TArray<TSharedPtr<FJsonValue>> SystemValues,RegionValues;
    if(!Write(Systems,SystemValues) || !Write(Regions,RegionValues)){Error=TEXT("Galaxy serialization failed");return false;}
    Root->SetArrayField(TEXT("systems"),SystemValues);Root->SetArrayField(TEXT("regions"),RegionValues);
    FString Json;
    if(!FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Json))){Error=TEXT("Galaxy JSON encoding failed");return false;}
    TArray<FS_Galaxy> VerifiedSystems;TArray<FRegion> VerifiedRegions;
    if(!Parse(Json,VerifiedSystems,VerifiedRegions,Error))return false;
    if(!FFileHelper::SaveStringToFile(Json,*Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {Error=TEXT("Galaxy JSON write failed: ")+Path;return false;}
    return true;
}
