#include "OOBJsonCatalog.h"
#include "DesignTableJson.h"
#include "GameStructs.h"
#include "Engine/DataTable.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
bool OOBJsonCatalog::ImportDirectory(UDataTable& Out,const FString& Directory,FString& Error)
{
 TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(Directory/TEXT("*.json")),true,false);Files.Sort();
 if(Files.IsEmpty()){Error=TEXT("No faction OOB JSON files: ")+Directory;return false;}
 auto* Candidate=NewObject<UDataTable>();Candidate->RowStruct=FS_CombatGroup::StaticStruct();
 for(const auto& File:Files)
 {
  auto* Part=NewObject<UDataTable>();
  if(!DesignTableJson::Import(*Part,*FS_CombatGroup::StaticStruct(),Directory/File)){Error=TEXT("Invalid faction OOB: ")+File;return false;}
  for(const auto& Pair:Part->GetRowMap())
  {
   const auto& Row=*reinterpret_cast<const FS_CombatGroup*>(Pair.Value);
   if(Row.Id<0 || Row.Name.IsEmpty() || Row.Iff<0 || Row.Type==ECOMBATGROUP_TYPE::NONE || Row.Location.ContainsNaN())
   {Error=TEXT("Invalid OOB group: ")+Pair.Key.ToString();return false;}
   if(Candidate->FindRowUnchecked(Pair.Key)){Error=TEXT("Duplicate faction OOB row: ")+Pair.Key.ToString();return false;}
   Candidate->AddRow(Pair.Key,Row);
  }
 }
 Out.EmptyTable();Out.RowStruct=FS_CombatGroup::StaticStruct();
 for(const auto& Pair:Candidate->GetRowMap())Out.AddRow(Pair.Key,*reinterpret_cast<const FS_CombatGroup*>(Pair.Value));
 return true;
}
