#include "OOBJsonCatalog.h"
#include "GameStructs.h"
#include "JsonObjectConverter.h"
#include "Engine/DataTable.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
bool OOBJsonCatalog::LoadDirectory(TMap<FName,FS_CombatGroup>& Out,const FString& Directory,FString& Error)
{
 TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(Directory/TEXT("*.json")),true,false);Files.Sort();
 if(Files.IsEmpty()){Error=TEXT("No faction OOB JSON files: ")+Directory;return false;}
 TMap<FName,FS_CombatGroup> Candidate;
 for(const auto& File:Files)
 {
  FString Text;TArray<TSharedPtr<FJsonValue>> Rows;
  if(IFileManager::Get().FileSize(*(Directory/File))>32*1024*1024 || !FFileHelper::LoadFileToString(Text,*(Directory/File)) ||
     !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Rows) || Rows.IsEmpty())
  {Error=TEXT("Invalid or empty faction OOB: ")+File;return false;}
  for(const auto& Value:Rows)
  {
   const auto Object=Value?Value->AsObject():nullptr;FString Key;FS_CombatGroup Row;
   if(!Object || !Object->TryGetStringField(TEXT("Row"),Key) || Key.IsEmpty() ||
      !FJsonObjectConverter::JsonObjectToUStruct(Object.ToSharedRef(),&Row) ||
      Row.Id<0 || Row.Name.IsEmpty() || Row.Iff<0 || Row.Type==ECOMBATGROUP_TYPE::NONE || Row.Location.ContainsNaN())
   {Error=TEXT("Invalid OOB group in ")+File+TEXT(": ")+Key;return false;}
   const FName Name(*Key);
   if(Candidate.Contains(Name)){Error=TEXT("Duplicate faction OOB row: ")+Key;return false;}
   Candidate.Add(Name,MoveTemp(Row));
  }
 }
 Out=MoveTemp(Candidate);Error.Reset();return true;
}
bool OOBJsonCatalog::ImportDirectory(UDataTable& Out,const FString& Directory,FString& Error)
{
 TMap<FName,FS_CombatGroup> Rows;if(!LoadDirectory(Rows,Directory,Error))return false;
 Out.EmptyTable();Out.RowStruct=FS_CombatGroup::StaticStruct();
 for(const auto& Pair:Rows)Out.AddRow(Pair.Key,Pair.Value);
 return true;
}
