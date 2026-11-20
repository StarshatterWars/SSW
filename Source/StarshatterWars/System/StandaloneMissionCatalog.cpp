#include "StandaloneMissionCatalog.h"
#include "MissionListJson.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "SSWModFiles.h"
namespace StandaloneMissionCatalog
{
bool Load(const FString& Directory,TArray<FS_CampaignMissionList>& Lists,TArray<FS_CampaignMission>& Scenarios,FString& Error)
{
    TArray<FS_CampaignMissionList> Entries;
    if(MissionListJson::ReadMissions(Directory,Entries,Error)!=MissionListJson::EReadResult::Loaded)return false;
    TArray<FS_CampaignMission> Candidate;TSet<int32> Ids;
    for(const auto& Entry:Entries)
    {
        if(Entry.Id<=0 || Ids.Contains(Entry.Id) || Entry.Script!=FPaths::GetCleanFilename(Entry.Script) ||
           !(Entry.Script.EndsWith(TEXT(".def")) || Entry.Script.EndsWith(TEXT(".json"))))
        {Error=TEXT("Duplicate identity or unsafe standalone mission script: ")+Entry.Script;return false;}
        Ids.Add(Entry.Id);const FString Path=Directory/FPaths::ChangeExtension(Entry.Script,TEXT("json"));
        TArray<uint8> Bytes;bool bJson=false;
        if(!SSWModFiles::ReadMission(Directory/Entry.Script,Bytes,bJson))
        {bJson=true;if(!FFileHelper::LoadFileToArray(Bytes,*Path)){Error=TEXT("Cannot read standalone mission JSON: ")+Path;return false;}}
        if(!bJson || Bytes.Num()>8*1024*1024){Error=TEXT("Expected bounded standalone mission JSON: ")+Path;return false;}
        FString Text;FFileHelper::BufferToString(Text,Bytes.GetData(),Bytes.Num());FS_CampaignMission Row;
        if(!FJsonObjectConverter::JsonObjectStringToUStruct(Text,&Row) || Row.Element.IsEmpty())
        {Error=TEXT("Invalid standalone scenario: ")+Path;return false;}
        Row.Id=Entry.Id;Row.RowName=FName(*FString::Printf(TEXT("Mission_%03d"),Entry.Id));
        if(Row.MissionName.IsEmpty())Row.MissionName=Entry.Name;
        if(Row.MissionSystem.IsEmpty())Row.MissionSystem=Entry.System;
        if(Row.MissionRegion.IsEmpty())Row.MissionRegion=Entry.Region;
        if(Row.Objective.IsEmpty())Row.Objective=Entry.Objective;
        if(Row.Sitrep.IsEmpty())Row.Sitrep=Entry.Sitrep;
        if(Row.Desc.IsEmpty())Row.Desc=Entry.Description;
        if(Row.StartTime.IsEmpty())Row.StartTime=Entry.Start;
        Row.Source=Entry.Source;Row.DisplayType=Entry.DisplayType;
        if(Row.MissionName.IsEmpty() || Row.MissionSystem.IsEmpty() || Row.MissionRegion.IsEmpty() || !StaticEnum<EEMPIRE_NAME>()->IsValidEnumValue(int64(Row.RequiredEmpire)))
        {Error=TEXT("Standalone mission lacks a valid location or faction: ")+Path;return false;}
        Candidate.Add(MoveTemp(Row));
    }
    Lists=MoveTemp(Entries);Scenarios=MoveTemp(Candidate);Error.Reset();return true;
}
}
