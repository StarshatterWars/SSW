#include "FactionTrainingCatalog.h"
#include "MissionListJson.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
const TCHAR* FactionTrainingCatalog::Title(EEMPIRE_NAME Empire)
{
 if(Empire==EEMPIRE_NAME::Marakan)return TEXT("Hegemony Flight Induction");
 if(Empire==EEMPIRE_NAME::Dantari)return TEXT("Separatist Flight Induction");
 return TEXT("Operation Live Fire");
}
const FS_Campaign* FactionTrainingCatalog::Resolve(const FS_Campaign& Original,EEMPIRE_NAME Empire,FString& Error)
{
 if(Empire!=EEMPIRE_NAME::Marakan && Empire!=EEMPIRE_NAME::Dantari)return &Original;
 // Heap-owned snapshots keep Campaign::CampaignData valid across profile changes and map rehashes.
 static TMap<EEMPIRE_NAME,TSharedPtr<FS_Campaign>> Snapshots;
 if(const auto* Existing=Snapshots.Find(Empire))return Existing->Get();
 const FString Faction=StaticEnum<EEMPIRE_NAME>()->GetNameStringByValue(int64(Empire));
 const FString Directory=FPaths::ProjectContentDir()/TEXT("GameData/Campaigns/Training")/Faction;
 auto Candidate=MakeShared<FS_Campaign>(Original);Candidate->Name=Title(Empire);Candidate->Index=0;
 Candidate->Action.Reset();Candidate->Zone.Reset();Candidate->Combatant.Reset();Candidate->TemplateList.Reset();
 Candidate->StoryMissions.Reset();Candidate->TemplateMissions.Reset();Candidate->ScriptedMissions.Reset();Candidate->Missions.Reset();
 Candidate->ActionSize=0;Candidate->CombatantSize=0;Candidate->bAvailable=true;Candidate->bCompleted=false;
 FString HeaderText;TSharedPtr<FJsonObject> Headers;
 if(!FFileHelper::LoadFileToString(HeaderText,*(FPaths::ProjectContentDir()/TEXT("GameData/Campaigns/campaign_information.json"))) ||
    !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(HeaderText),Headers) || !Headers)
 {Error=TEXT("Cannot load faction training headers");return nullptr;}
 bool HeaderFound=false;
 for(const auto& Value:Headers->GetArrayField(TEXT("campaigns")))
 {
  const auto H=Value->AsObject();if(H->GetStringField(TEXT("Empire"))!=Faction || H->GetIntegerField(TEXT("CampaignId"))!=1)continue;
  Candidate->Description=H->GetStringField(TEXT("Description"));Candidate->Situation=H->GetStringField(TEXT("Situation"));
  Candidate->Start=H->GetStringField(TEXT("Start"));Candidate->System=H->GetStringField(TEXT("System"));Candidate->Region=H->GetStringField(TEXT("Region"));
  Candidate->MainImage=H->GetStringField(TEXT("MainImage"));Candidate->Orders.Reset();
  for(const auto& Order:H->GetArrayField(TEXT("Orders")))Candidate->Orders.Add(Order->AsString());HeaderFound=true;break;
 }
 if(!HeaderFound || MissionListJson::ReadMissions(Directory,Candidate->MissionList,Error)!=MissionListJson::EReadResult::Loaded || Candidate->MissionList.Num()!=7)
 {if(Error.IsEmpty())Error=TEXT("Incomplete faction training catalog");return nullptr;}
 for(int32 I=0;I<7;++I)
 {
  const auto& Entry=Candidate->MissionList[I];FS_CampaignMission Mission;FString Text;
  const FString Scene=Directory/TEXT("Scenes")/FString::Printf(TEXT("T%d.json"),I+1);
  if(Entry.Id!=101+I || !FFileHelper::LoadFileToString(Text,*Scene) || !FJsonObjectConverter::JsonObjectStringToUStruct(Text,&Mission) ||
     Mission.Id!=Entry.Id || Mission.RequiredEmpire!=Empire || Mission.Element.IsEmpty() || Mission.MissionType!=EMissionType::TRAINING)
  {Error=TEXT("Invalid faction training scenario: ")+Scene;return nullptr;}
  Candidate->Missions.Add(MoveTemp(Mission));
 }
 const auto* Result=&Candidate.Get();Snapshots.Add(Empire,MoveTemp(Candidate));Error.Reset();return Result;
}
