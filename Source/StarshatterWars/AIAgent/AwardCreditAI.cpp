#include "AwardCreditAI.h"
#include "TransactionAI.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
const AwardCreditAI& AwardCreditAI::Get()
{
 static const AwardCreditAI Policy=[](){AwardCreditAI C;FString Json;FText Error;if(!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Awards/awards.json"))) || !C.LoadJson(Json,Error))UE_LOG(LogTemp,Error,TEXT("[AwardCredits] %s"),*Error.ToString());return C;}();return Policy;
}
bool AwardCreditAI::LoadJson(const FString& Json,FText& Error)
{
 TSharedPtr<FJsonObject> Root;const TSharedPtr<FJsonObject>* Policy=nullptr;
 if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root) || !Root || !Root->TryGetObjectField(TEXT("creditAwards"),Policy)){Error=INVTEXT("Award credit policy is missing.");return false;}
 TMap<int32,int64> NewRanks,NewMedals;
 const auto Read=[&](const TCHAR* Name,TMap<int32,int64>& Out)
 {
  const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;if(!(*Policy)->TryGetArrayField(Name,Rows) || !Rows || Rows->IsEmpty())return false;
  for(const auto& Value:*Rows){auto O=Value->AsObject();int32 Id=0;double Amount=0;if(!O || !O->TryGetNumberField(TEXT("id"),Id) || Id<0 || Out.Contains(Id) || !O->TryGetNumberField(TEXT("credits"),Amount) || !FMath::IsFinite(Amount) || Amount<0 || Amount>100000000 || FMath::FloorToDouble(Amount)!=Amount)return false;Out.Add(Id,int64(Amount));}return true;
 };
 if(!Read(TEXT("ranks"),NewRanks) || !Read(TEXT("medals"),NewMedals)){Error=INVTEXT("Invalid or duplicate award credit policy.");return false;}
 Ranks=MoveTemp(NewRanks);Medals=MoveTemp(NewMedals);Error=FText::GetEmpty();return true;
}
int64 AwardCreditAI::Credits(bool Rank,int32 Id)const{return (Rank?Ranks:Medals).FindRef(Id);}
bool AwardCreditAI::Grant(FEconomyState& State,bool Rank,int32 Id)const
{
 const int64 Amount=Credits(Rank,Id);if(Amount<=0)return false;
 const FString Label=(Rank?FString::Printf(TEXT("Promotion to rank %d"),Id):FString::Printf(TEXT("Medal award %d"),Id));
 FGuid Receipt;FGuid::ParseExact(FMD5::HashAnsiString(*(TEXT("SSW Award Credit/")+Label)),EGuidFormats::Digits,Receipt);
 const int64 Balance=TransactionAI::Balance(State);if(State.CommittedTransactions.Contains(Receipt) || Balance<0 || Balance>MAX_int64-Amount)return false;
 return TransactionAI::ReconcileExternalBalance(State,FEconomyLedger::Pilot(),Balance+Amount,FName(*Label),TEXT("Fleet Personnel Office"),EWalletTransactionCategory::MissionReward,Receipt,Receipt);
}
