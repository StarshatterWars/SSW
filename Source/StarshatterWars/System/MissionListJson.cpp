#include "MissionListJson.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "SSWModFiles.h"
#include "UObject/UnrealType.h"

namespace
{
constexpr int32 MaxBytes=8*1024*1024,MaxRows=10000;
bool Valid(const FS_CampaignMissionList& Row)
{
    return Row.Id>=0 && Row.MinRank>=0 && !Row.Script.TrimStartAndEnd().IsEmpty() &&
        StaticEnum<EMissionType>()->IsValidEnumValue(static_cast<int64>(Row.Type)) &&
        StaticEnum<EMISSIONSTATUS>()->IsValidEnumValue(static_cast<int64>(Row.Status)) &&
        StaticEnum<EMissionSource>()->IsValidEnumValue(static_cast<int64>(Row.Source)) &&
        StaticEnum<EMissionDisplayType>()->IsValidEnumValue(static_cast<int64>(Row.DisplayType));
}
bool Valid(const FS_CampaignTemplateList& Row)
{
    return Row.Id>=0 && Row.MinRank>=0 && Row.MaxRank>=0 && !Row.Script.TrimStartAndEnd().IsEmpty() &&
        StaticEnum<EMissionType>()->IsValidEnumValue(static_cast<int64>(Row.MissionType)) &&
        StaticEnum<ECOMBATGROUP_TYPE>()->IsValidEnumValue(static_cast<int64>(Row.GroupType)) &&
        StaticEnum<EMissionSource>()->IsValidEnumValue(static_cast<int64>(Row.Source)) &&
        StaticEnum<EMissionDisplayType>()->IsValidEnumValue(static_cast<int64>(Row.DisplayType));
}
template<class Row> bool Parse(const FString& Text,const TCHAR* Kind,TArray<Row>& Out,FString& Error)
{
    Error.Reset();TSharedPtr<FJsonObject> Root;
    if(Text.Len()>MaxBytes || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root) || !Root)
    {Error=TEXT("Invalid or oversized list JSON");return false;}
    double Version=0;FString Actual;const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;
    if(!Root->TryGetNumberField(TEXT("schemaVersion"),Version) || Version!=1 ||
        !Root->TryGetStringField(TEXT("kind"),Actual) || Actual!=Kind ||
        !Root->TryGetArrayField(TEXT("rows"),Values) || !Values || Values->Num()>MaxRows)
    {Error=TEXT("Invalid list schemaVersion, kind or rows");return false;}
    Row Defaults;auto Shape=MakeShared<FJsonObject>();
    if(!FJsonObjectConverter::UStructToJsonObject(Row::StaticStruct(),&Defaults,Shape,0,0))
    {Error=TEXT("Cannot determine list row schema");return false;}
    TArray<Row> Candidate;Candidate.Reserve(Values->Num());
    for(int32 Index=0;Index<Values->Num();++Index)
    {
        const auto& Value=(*Values)[Index];const auto Object=Value && Value->Type==EJson::Object?Value->AsObject():nullptr;
        if(!Object){Error=FString::Printf(TEXT("List row %d must be an object"),Index);return false;}
        for(const auto& Field:Shape->Values)
            if(!Object->HasField(Field.Key)){Error=FString::Printf(TEXT("List row %d missing field %s"),Index,*Field.Key);return false;}
        for(const auto& Field:Object->Values)
            if(!Shape->HasField(Field.Key)){Error=FString::Printf(TEXT("List row %d unknown field %s"),Index,*Field.Key);return false;}
        Row Item;
        if(!FJsonObjectConverter::JsonObjectToUStruct(Object.ToSharedRef(),&Item,0,0) || !Valid(Item))
        {Error=FString::Printf(TEXT("Invalid list row %d"),Index);return false;}
        Candidate.Add(MoveTemp(Item));
    }
    Out=MoveTemp(Candidate);return true;
}
template<class Row> MissionListJson::EReadResult Read(const FString& Directory,const TCHAR* Kind,const TCHAR* Leaf,TArray<Row>& Out,FString& Error)
{
    const FString Path=Directory/Leaf;TArray<uint8> Bytes;
    bool bModJson=false;
    const FString LegacyPath=Directory/(FCString::Strcmp(Kind,TEXT("missions"))==0?TEXT("Missions.def"):TEXT("Templates.def"));
    const bool bModOverride=SSWModFiles::ReadMission(LegacyPath,Bytes,bModJson);
    if(bModOverride && !bModJson){Error.Reset();return MissionListJson::EReadResult::Missing;}
    if(!bModOverride && !SSWModFiles::Read(Path,Bytes))
    {
        if(!FPaths::FileExists(Path)){Error.Reset();return MissionListJson::EReadResult::Missing;}
        if(!FFileHelper::LoadFileToArray(Bytes,*Path)){Error=TEXT("Cannot read ")+Path;return MissionListJson::EReadResult::Invalid;}
    }
    if(Bytes.Num()>MaxBytes){Error=TEXT("Oversized list JSON: ")+Path;return MissionListJson::EReadResult::Invalid;}
    FString Text;FFileHelper::BufferToString(Text,Bytes.GetData(),Bytes.Num());
    if(!Parse(Text,Kind,Out,Error)){Error=Path+TEXT(": ")+Error;return MissionListJson::EReadResult::Invalid;}
    return MissionListJson::EReadResult::Loaded;
}
template<class Row> bool Write(const FString& Directory,const TCHAR* Kind,const TCHAR* Leaf,const TArray<Row>& Rows,FString& Error)
{
    Error.Reset();auto Root=MakeShared<FJsonObject>();Root->SetNumberField(TEXT("schemaVersion"),1);Root->SetStringField(TEXT("kind"),Kind);
    TArray<TSharedPtr<FJsonValue>> Values;
    for(const auto& RowValue:Rows)
    {
        auto Object=MakeShared<FJsonObject>();
        if(!Valid(RowValue) || !FJsonObjectConverter::UStructToJsonObject(Row::StaticStruct(),&RowValue,Object,0,0))
        {Error=TEXT("Cannot serialize invalid list row");return false;}
        Values.Add(MakeShared<FJsonValueObject>(Object));
    }
    Root->SetArrayField(TEXT("rows"),Values);FString Text;
    if(Rows.Num()>MaxRows || !FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Text)) || Text.Len()>MaxBytes ||
        !FFileHelper::SaveStringToFile(Text,*(Directory/Leaf),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {Error=TEXT("Cannot write list JSON: ")+Directory/Leaf;return false;}
    return true;
}
}
namespace MissionListJson
{
bool ParseMissions(const FString& Text,TArray<FS_CampaignMissionList>& Out,FString& Error){return Parse(Text,TEXT("missions"),Out,Error);}
bool ParseTemplates(const FString& Text,TArray<FS_CampaignTemplateList>& Out,FString& Error){return Parse(Text,TEXT("templates"),Out,Error);}
EReadResult ReadMissions(const FString& Directory,TArray<FS_CampaignMissionList>& Out,FString& Error){return Read(Directory,TEXT("missions"),TEXT("missions.json"),Out,Error);}
EReadResult ReadTemplates(const FString& Directory,TArray<FS_CampaignTemplateList>& Out,FString& Error){return Read(Directory,TEXT("templates"),TEXT("templates.json"),Out,Error);}
bool WriteMissions(const FString& Directory,const TArray<FS_CampaignMissionList>& Rows,FString& Error){return Write(Directory,TEXT("missions"),TEXT("missions.json"),Rows,Error);}
bool WriteTemplates(const FString& Directory,const TArray<FS_CampaignTemplateList>& Rows,FString& Error){return Write(Directory,TEXT("templates"),TEXT("templates.json"),Rows,Error);}
}
