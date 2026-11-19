#include "StationServiceCatalog.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "SSWModFiles.h"

namespace
{
bool ServiceFromName(const FString& Name,ESSWStationService& Out)
{
    static const TCHAR* Names[]={TEXT("Cargo"),TEXT("Market"),TEXT("Insurance"),TEXT("Repair"),TEXT("Loadout"),
        TEXT("HullSelection"),TEXT("Refit"),TEXT("Manufacturing"),TEXT("Blueprints"),TEXT("SkillBooks")};
    for(uint8 I=0;I<UE_ARRAY_COUNT(Names);++I)if(Name==Names[I]){Out=static_cast<ESSWStationService>(I);return true;}
    return false;
}
}
const FStationServiceCatalog& FStationServiceCatalog::Get()
{
    static const FStationServiceCatalog Catalog=[]()
    {
        FStationServiceCatalog Candidate;FString Error;
        if(!Candidate.Load(FPaths::ProjectContentDir()/TEXT("GameData/Production/station_services.json"),Error))
            UE_LOG(LogTemp,Error,TEXT("[StationServices] Catalog unavailable; services denied: %s"),*Error);
        return Candidate;
    }();
    return Catalog;
}
bool FStationServiceCatalog::Load(const FString& Path,FString& Error)
{
    TArray<uint8> Bytes;
    if((!SSWModFiles::Read(Path,Bytes) && !FFileHelper::LoadFileToArray(Bytes,*Path)) || Bytes.Num()>1024*1024)
    {Error=TEXT("Unreadable or oversized station service catalog: ")+Path;return false;}
    FString Text;FFileHelper::BufferToString(Text,Bytes.GetData(),Bytes.Num());return LoadJson(Text,Error);
}
bool FStationServiceCatalog::LoadJson(const FString& Json,FString& Error)
{
    Error.Reset();TSharedPtr<FJsonObject> Root;double Version=0;
    const TArray<TSharedPtr<FJsonValue>>* Profiles=nullptr;const TArray<TSharedPtr<FJsonValue>>* Stations=nullptr;
    if(Json.Len()>1024*1024 || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root) || !Root ||
        !Root->TryGetNumberField(TEXT("schemaVersion"),Version) || Version!=1 ||
        !Root->TryGetArrayField(TEXT("profiles"),Profiles) || !Root->TryGetArrayField(TEXT("stations"),Stations) ||
        !Profiles || !Stations || Profiles->Num()>128 || Stations->Num()>4096)
    {Error=TEXT("Invalid station service schema");return false;}
    TMap<FName,TSet<ESSWStationService>> Definitions;
    for(const auto& Value:*Profiles)
    {
        const auto Object=Value && Value->Type==EJson::Object?Value->AsObject():nullptr;
        FString Name;const TArray<TSharedPtr<FJsonValue>>* Services=nullptr;
        if(!Object || !Object->TryGetStringField(TEXT("name"),Name) || Name.TrimStartAndEnd().IsEmpty() ||
            Definitions.Contains(FName(*Name)) || !Object->TryGetArrayField(TEXT("services"),Services) || !Services)
        {Error=TEXT("Invalid or duplicate service profile");return false;}
        TSet<ESSWStationService> Capabilities;
        for(const auto& Item:*Services)
        {
            FString NameValue;ESSWStationService Service;
            if(!Item || !Item->TryGetString(NameValue) || !ServiceFromName(NameValue,Service) || Capabilities.Contains(Service))
            {Error=TEXT("Unknown or duplicate station service");return false;}
            Capabilities.Add(Service);
        }
        Definitions.Add(FName(*Name),MoveTemp(Capabilities));
    }
    TArray<FEntry> Candidate;
    for(const auto& Value:*Stations)
    {
        const auto Object=Value && Value->Type==EJson::Object?Value->AsObject():nullptr;
        FString Design,Facility,Profile;
        if(!Object || !Object->TryGetStringField(TEXT("design"),Design) || Design.TrimStartAndEnd().IsEmpty() ||
            !Object->TryGetStringField(TEXT("profile"),Profile) || !Definitions.Contains(FName(*Profile)))
        {Error=TEXT("Invalid station role or unknown profile");return false;}
        if(Object->HasField(TEXT("facility")) && !Object->TryGetStringField(TEXT("facility"),Facility))
        {Error=TEXT("Invalid station facility override");return false;}
        FEntry Entry;Entry.Design=FName(*Design);Entry.Facility=FName(*Facility);Entry.Services=Definitions.FindChecked(FName(*Profile));
        if(Candidate.ContainsByPredicate([&](const FEntry& Existing){return Existing.Design==Entry.Design && Existing.Facility==Entry.Facility;}))
        {Error=TEXT("Duplicate station role");return false;}
        Candidate.Add(MoveTemp(Entry));
    }
    Entries=MoveTemp(Candidate);return true;
}
bool FStationServiceCatalog::HasService(FName Design,FName Facility,ESSWStationService Service) const
{
    if(Design.IsNone())return false;
    const FEntry* Default=nullptr;
    for(const auto& Entry:Entries)
        if(Entry.Design==Design)
        {
            if(!Entry.Facility.IsNone() && Entry.Facility==Facility)return Entry.Services.Contains(Service);
            if(Entry.Facility.IsNone())Default=&Entry;
        }
    return Default && Default->Services.Contains(Service);
}
