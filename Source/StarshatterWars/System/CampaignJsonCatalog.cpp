#include "CampaignJsonCatalog.h"
#include "MissionListJson.h"
#include "Engine/DataTable.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "SSWModFiles.h"

namespace
{
bool Read(const FString& Path,const FString& Campaign,const TCHAR* Field,TArray<TSharedPtr<FJsonValue>>& Rows,FString& Error)
{
    FString Text;TSharedPtr<FJsonObject> Object;TArray<uint8> Bytes;
    if((!SSWModFiles::Read(Path,Bytes) && !FFileHelper::LoadFileToArray(Bytes,*Path)) || Bytes.Num()>8*1024*1024)
    {Error=TEXT("Unreadable/oversized campaign JSON: ")+Path;return false;}
    FFileHelper::BufferToString(Text,Bytes.GetData(),Bytes.Num());
    if(Text.Len()>8*1024*1024 ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Object) || !Object)
    {Error=TEXT("Unreadable/invalid campaign JSON: ")+Path;return false;}
    double Version=0;FString Owner;const TArray<TSharedPtr<FJsonValue>>* Items=nullptr;
    if(!Object->TryGetNumberField(TEXT("schemaVersion"),Version) || Version!=1 ||
        !Object->TryGetStringField(TEXT("campaign"),Owner) || Owner!=Campaign ||
        !Object->TryGetArrayField(Field,Items) || !Items || Items->Num()>10000)
    {Error=TEXT("Invalid schema, campaign ownership or rows: ")+Path;return false;}
    Rows=*Items;return true;
}
template<class Row> bool Write(const FString& Path,const FString& Campaign,const TCHAR* Field,const TArray<Row>& Rows,FString& Error)
{
    auto Object=MakeShared<FJsonObject>();Object->SetNumberField(TEXT("schemaVersion"),1);Object->SetStringField(TEXT("campaign"),Campaign);
    TArray<TSharedPtr<FJsonValue>> Items;
    for(const auto& Item:Rows)
    {
        auto Value=MakeShared<FJsonObject>();
        if(!FJsonObjectConverter::UStructToJsonObject(Row::StaticStruct(),&Item,Value,0,0)){Error=TEXT("Cannot serialize campaign row");return false;}
        Items.Add(MakeShared<FJsonValueObject>(Value));
    }
    Object->SetArrayField(Field,Items);FString Text;
    if(!FJsonSerializer::Serialize(Object,TJsonWriterFactory<>::Create(&Text)) ||
        !FFileHelper::SaveStringToFile(Text,*Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {Error=TEXT("Cannot write campaign JSON: ")+Path;return false;}
    return true;
}
void ClearCampaign(UDataTable* Table,const FString& Prefix)
{if(Table)for(FName Name:Table->GetRowNames())if(Name.ToString().StartsWith(Prefix))Table->RemoveRow(Name);}
}

bool CampaignJsonCatalog::Apply(const FString& Directory,FS_Campaign& Campaign,UDataTable* Actions,UDataTable* Zones,FString& Error)
{
    Error.Reset();FS_Campaign Candidate=Campaign;
    const FString ActionPath=Directory/TEXT("actions.json"),ZonePath=Directory/TEXT("zones.json");
    const bool HasActions=FPaths::FileExists(ActionPath),HasZones=FPaths::FileExists(ZonePath);
    if((Actions && Actions->GetRowStruct()!=FS_CampaignAction::StaticStruct()) ||
       (Zones && Zones->GetRowStruct()!=FS_CampaignZone::StaticStruct()))
    {Error=TEXT("Incorrect campaign table row structure");return false;}
    if(HasActions)
    {
        TArray<TSharedPtr<FJsonValue>> Values;if(!Read(ActionPath,Campaign.Name,TEXT("actions"),Values,Error))return false;
        Candidate.Action.Reset();TSet<int32> Ids;
        for(const auto& Value:Values)
        {
            FS_CampaignAction Row;const auto Object=Value?Value->AsObject():nullptr;
            if(!Object || !Object->HasField(TEXT("id")) || !FJsonObjectConverter::JsonObjectToUStruct(Object.ToSharedRef(),&Row,0,0) ||
                Row.Id<0 || Ids.Contains(Row.Id) || Row.Type.IsEmpty() || (!Row.Campaign.IsEmpty() && Row.Campaign!=Campaign.Name))
            {Error=TEXT("Invalid/duplicate action or campaign mismatch: ")+ActionPath;return false;}
            Row.Campaign=Campaign.Name;Ids.Add(Row.Id);Candidate.Action.Add(MoveTemp(Row));
        }
        Candidate.ActionSize=Candidate.Action.Num();
    }
    if(HasZones)
    {
        TArray<TSharedPtr<FJsonValue>> Values;if(!Read(ZonePath,Campaign.Name,TEXT("zones"),Values,Error))return false;
        Candidate.Zone.Reset();
        for(const auto& Value:Values)
        {
            FS_CampaignZone Row;const auto Object=Value?Value->AsObject():nullptr;
            if(!Object || !FJsonObjectConverter::JsonObjectToUStruct(Object.ToSharedRef(),&Row,0,0) || Row.System.IsEmpty() || Row.Region.IsEmpty())
            {Error=TEXT("Invalid campaign zone: ")+ZonePath;return false;}
            // Preserve repeated legacy regions and their authored order.
            Candidate.Zone.Add(MoveTemp(Row));
        }
    }
    if (MissionListJson::ReadMissions(Directory,Candidate.MissionList,Error)==MissionListJson::EReadResult::Invalid ||
        MissionListJson::ReadTemplates(Directory,Candidate.TemplateList,Error)==MissionListJson::EReadResult::Invalid) return false;
    // Commit only after both files validate. Remove stale rows on reload without
    // changing another campaign's entries or replacing table assets.
    if(HasActions && Actions)
    {
        ClearCampaign(Actions,Campaign.Name+TEXT(".Action"));
        for(const auto& Row:Candidate.Action)Actions->AddRow(FName(*FString::Printf(TEXT("%s.Action%03d"),*Campaign.Name,Row.Id)),Row);
    }
    if(HasZones && Zones)
    {
        ClearCampaign(Zones,Campaign.Name+TEXT(".Zone"));
        for(int32 I=0;I<Candidate.Zone.Num();++I)Zones->AddRow(FName(*FString::Printf(TEXT("%s.Zone%03d"),*Campaign.Name,I)),Candidate.Zone[I]);
    }
    Campaign=MoveTemp(Candidate);return true;
}
bool CampaignJsonCatalog::Export(const FString& Directory,const FS_Campaign& Campaign,FString& Error)
{
    return Write(Directory/TEXT("actions.json"),Campaign.Name,TEXT("actions"),Campaign.Action,Error) &&
        Write(Directory/TEXT("zones.json"),Campaign.Name,TEXT("zones"),Campaign.Zone,Error);
}
