#include "FactionBriefingCatalog.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
namespace FactionBriefingCatalog
{
const TArray<FFactionCampaignBriefing>& All()
{
    static const TArray<FFactionCampaignBriefing> Rows=[]()
    {
        TArray<FFactionCampaignBriefing> Result;FString Text;TSharedPtr<FJsonObject> Root;
        if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("GameData/Campaigns/campaign_information.json"))) ||
           !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root) || !Root)return Result;
        const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;int32 Version=0;
        if(!Root->TryGetNumberField(TEXT("schemaVersion"),Version) || Version!=1 || !Root->TryGetArrayField(TEXT("factionBriefings"),Values))return Result;
        TSet<FName> Ids;
        for(const auto& V:*Values)
        {
            auto O=V?V->AsObject():nullptr;FFactionCampaignBriefing Row;FString Id,Empire;
            if(!O || !O->TryGetStringField(TEXT("Name"),Id) || Id.IsEmpty() || Ids.Contains(FName(*Id)) ||
               !O->TryGetStringField(TEXT("Empire"),Empire) || !O->TryGetStringField(TEXT("Title"),Row.Title) || Row.Title.IsEmpty() ||
               !O->TryGetStringField(TEXT("Body"),Row.Body) || Row.Body.IsEmpty() ||
               !O->TryGetNumberField(TEXT("CampaignId"),Row.CampaignId) || Row.CampaignId<1 || Row.CampaignId>5)return TArray<FFactionCampaignBriefing>();
            const int64 Enum=StaticEnum<EEMPIRE_NAME>()->GetValueByNameString(Empire);
            if(Enum==INDEX_NONE || Enum==int64(EEMPIRE_NAME::Unknown) || Enum==int64(EEMPIRE_NAME::NONE))return TArray<FFactionCampaignBriefing>();
            Row.Empire=EEMPIRE_NAME(Enum);Row.Id=FName(*Id);O->TryGetNumberField(TEXT("FirstMissionId"),Row.FirstMissionId);
            const TArray<TSharedPtr<FJsonValue>>* Required=nullptr;
            if(O->TryGetArrayField(TEXT("PrerequisiteMissionIds"),Required))for(const auto& R:*Required)Row.Prerequisites.Add(int32(R->AsNumber()));
            Ids.Add(Row.Id);Result.Add(MoveTemp(Row));
        }
        return Result;
    }();return Rows;
}
bool Available(const FFactionCampaignBriefing& Row,const FS_PlayerGameInfo& Pilot)
{
    if(Pilot.Empire!=int32(Row.Empire))return false;
    for(int32 Id:Row.Prerequisites)if(!Pilot.HasCompletedMission(1000,Id))return false;
    return true;
}
const FFactionCampaignBriefing* Find(FName Id){return All().FindByPredicate([&](const auto& Row){return Row.Id==Id;});}
}
