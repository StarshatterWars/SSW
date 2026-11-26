#include "GeneratedShipNames.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool FGeneratedShipNames::Load(const FString& JsonFile,FString& Error)
{
 FString Text;TSharedPtr<FJsonObject> Root;
 if(!FFileHelper::LoadFileToString(Text,*JsonFile) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root) || !Root)
 {Error=TEXT("Ship-name vocabulary could not be read.");return false;}
 TArray<FString> First,Second;
 if(!Root->TryGetStringArrayField(TEXT("prefixes"),First) || !Root->TryGetStringArrayField(TEXT("suffixes"),Second) || First.IsEmpty() || Second.IsEmpty())
 {Error=TEXT("Ship names require non-empty prefixes and suffixes.");return false;}
 for(const auto* Words:{&First,&Second})for(const FString& Word:*Words)
     if(Word.IsEmpty() || Word.Len()>32 || Word.Contains(TEXT("\n")) || Word.Contains(TEXT("\r")))
     {Error=TEXT("Invalid ship-name vocabulary entry.");return false;}
 Prefixes=MoveTemp(First);Suffixes=MoveTemp(Second);Error.Reset();return true;
}
FString FGeneratedShipNames::ForHull(FGuid Id)const
{
 if(!Id.IsValid() || Prefixes.IsEmpty() || Suffixes.IsEmpty())return {};
 return Prefixes[(Id.A^Id.C)%uint32(Prefixes.Num())]+TEXT(" ")+Suffixes[(Id.B^Id.D)%uint32(Suffixes.Num())];
}
