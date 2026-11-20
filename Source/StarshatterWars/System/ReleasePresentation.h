#pragma once
#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
namespace SSWReleasePresentation
{
 struct FInfo
 {
  FString Stage=TEXT("Alpha 4");
  FString Warning=TEXT("Work in progress. This alpha build may contain bugs, crashes, incomplete features and save incompatibilities. Gameplay and balance may change.");
  FString Version(const FString& Build)const{return Stage+TEXT(" | Build ")+Build;}
 };
 inline FInfo Read()
 {
  FInfo Info;FString Text;TSharedPtr<FJsonObject> Object;
  if(FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("Build/release-info.json"))) &&
     FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Object) && Object)
  {
   FString Stage,Warning;
   if(Object->TryGetStringField(TEXT("stage"),Stage) && !Stage.IsEmpty())Info.Stage=Stage;
   if(Object->TryGetStringField(TEXT("warning"),Warning) && !Warning.IsEmpty())Info.Warning=Warning;
  }
  return Info;
 }
}
