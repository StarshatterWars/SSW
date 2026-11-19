#include "MissionRewardRules.h"
#include "TransactionAI.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
namespace { FName Key(int32 Campaign,int32 Mission){return FName(*FString::Printf(TEXT("%d:%d"),Campaign,Mission));} }
const FMissionRewardCatalog& FMissionRewardCatalog::Get()
{
    static const FMissionRewardCatalog Catalog=[](){FMissionRewardCatalog C;FString Json;FText Error;
        if(!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Agents/mission_rewards.json"))) || !C.LoadJson(Json,Error))
            UE_LOG(LogTemp,Error,TEXT("[MissionRewards] Using base reward fallback: %s"),*Error.ToString());return C;}();return Catalog;
}
bool FMissionRewardCatalog::LoadJson(const FString& Json,FText& Error)
{
    TSharedPtr<FJsonObject> Root;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root) || !Root)return false;
    auto Read=[](const TSharedPtr<FJsonObject>& O,FMissionRewardPolicy& P)
    {
        const auto Number=[&](const TCHAR* Name,int64& Value){double N=0;if(!O->TryGetNumberField(Name,N) || !FMath::IsFinite(N) || N<0 || N>1000000000 || N!=FMath::FloorToDouble(N))return false;Value=static_cast<int64>(N);return true;};
        int64 Limit=0;
        if(!Number(TEXT("BaseCredits"),P.Base) || P.Base<=0 || !Number(TEXT("RecoveryBonus"),P.RecoveryBonus) || !Number(TEXT("SecondaryObjectiveBonus"),P.ObjectiveBonus) || !Number(TEXT("TimeBonus"),P.TimeBonus) || !Number(TEXT("TimeLimitSeconds"),Limit))return false;
        if(P.TimeBonus>0 && Limit<=0)return false;P.TimeLimitSeconds=static_cast<int32>(Limit);return true;
    };
    const TSharedPtr<FJsonObject>* Def=nullptr;FMissionRewardPolicy NewDefault;
    const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;TMap<FName,FMissionRewardPolicy> NewPolicies;
    if(!Root->TryGetObjectField(TEXT("Default"),Def) || !Read(*Def,NewDefault) || !Root->TryGetArrayField(TEXT("Missions"),Rows))return false;
    for(const auto& V:*Rows)
    {
        auto O=V.IsValid()?V->AsObject():nullptr;int32 Campaign=0,Mission=0;FMissionRewardPolicy P;
        if(!O || !O->TryGetNumberField(TEXT("CampaignId"),Campaign) || Campaign<0 || !O->TryGetNumberField(TEXT("MissionId"),Mission) || Mission<=0 || !Read(O,P) || NewPolicies.Contains(Key(Campaign,Mission)))return false;
        NewPolicies.Add(Key(Campaign,Mission),P);
    }
    Default=NewDefault;Policies=MoveTemp(NewPolicies);Error=FText::GetEmpty();return true;
}
FMissionRewardQuote FMissionRewardCatalog::Quote(int32 Campaign,int32 Mission,bool Docked,int32 Secondary,double Elapsed) const
{
    const auto* Override=Policies.Find(Key(Campaign,Mission));const auto& P=Override?*Override:Default;
    FMissionRewardQuote Q;Q.Base=P.Base;Q.Recovery=Docked?P.RecoveryBonus:0;
    Q.Objectives=P.ObjectiveBonus*FMath::Clamp(Secondary,0,100);
    Q.Time=FMath::IsFinite(Elapsed) && Elapsed>0 && P.TimeLimitSeconds>0 && Elapsed<=P.TimeLimitSeconds?P.TimeBonus:0;return Q;
}
bool FMissionRewardCatalog::Award(FEconomyState& State,int32 Campaign,int32 Mission,const FMissionRewardQuote& Q,bool Success,bool First,bool Replay) const
{
    if(!Success || !First || Replay || Campaign<0 || Mission<=0 || Q.Base<=0 || Q.Recovery<0 || Q.Objectives<0 || Q.Time<0 || Q.Base>1000000000 || Q.Recovery>1000000000 || Q.Objectives>100000000000LL || Q.Time>1000000000)return false;
    FGuid Receipt;FGuid::ParseExact(FMD5::HashAnsiString(*FString::Printf(TEXT("SSW Mission Reward %d:%d"),Campaign,Mission)),EGuidFormats::Digits,Receipt);
    if(State.CommittedTransactions.Contains(Receipt))return false;
    const int64 Balance=TransactionAI::Balance(State);if(Balance<0 || Balance>MAX_int64-Q.Total())return false;
    return TransactionAI::ReconcileExternalBalance(State,FEconomyLedger::Pilot(),Balance+Q.Total(),
        FName(*FString::Printf(TEXT("Mission %d:%d reward (base %lld; bonuses %lld)"),Campaign,Mission,Q.Base,Q.Total()-Q.Base)),
        TEXT("Mission command"),EWalletTransactionCategory::MissionReward,Receipt,Receipt);
}
