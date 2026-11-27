#include "EconomyServiceSubsystem.h"
#include "StarshatterPlayerSubsystem.h"
#include "MarketCommandAI.h"
#include "NpcTradeAI.h"
#include "Engine/GameInstance.h"
#include "Misc/SecureHash.h"
#include "Dom/JsonObject.h"
namespace
{
using FDone=TFunction<void(bool,const FText&,FGuid)>;
using FSharedDone=TSharedRef<FDone>;
FString Profile(UStarshatterPlayerSubsystem* P){return P?P->GetSlotName()+TEXT("/")+P->GetActiveSavedPlayerSlot()+FString::Printf(TEXT("/%d/%lld"),P->GetUserIndex(),P->GetPlayerInfo().CreateTime):FString();}
void Finish(const FSharedDone& Done,bool Success,const FString& Text,FGuid Id={}){if(*Done){auto Callback=MoveTemp(*Done);Callback(Success,FText::FromString(Text),Id);}}
TSharedRef<FJsonObject> Request(const FNpcTradeReceipt& T,const TCHAR* Action,const TCHAR* Suffix)
{
    auto Body=MakeShared<FJsonObject>();const auto Id=T.Id.ToString(EGuidFormats::Digits);
    Body->SetStringField(TEXT("action"),Action);Body->SetStringField(TEXT("id"),Id+Suffix);Body->SetStringField(TEXT("tradeId"),Id);
    Body->SetStringField(TEXT("actor"),T.Principal);Body->SetStringField(TEXT("worldId"),T.WorldId);return Body;
}
bool Credits(const TSharedPtr<FJsonObject>& Root,const TCHAR* Name,int64& Out)
{double V=0;if(!Root||!Root->TryGetNumberField(Name,V)||!FMath::IsFinite(V)||V<0||V>9.e15||V!=FMath::FloorToDouble(V))return false;Out=int64(V);return true;}
}
bool UEconomyServiceSubsystem::Trade(const FNativeMarketCommand& CommandIn,TFunction<bool(FText&)>&& Validate,FDone&& Completed,FText& Reason)
{
    auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if(!RequiresExternal()||!IsReady()||bTradePending||!Players||!Players->HasLoaded())
    {Reason=INVTEXT("The economy is not ready, or a trade is awaiting confirmation. Nothing exchanged.");return false;}
    if(CommandIn.Action!=EMarketCommand::Trade){Reason=INVTEXT("Queued orders need the remote order adapter. No local order was placed.");return false;}
    if(!Validate||!Validate(Reason))return false;
    FNpcTradeReceipt T;T.Id=CommandIn.Receipt;T.WorldId=Cached.WorldId;const FString Bound=Profile(Players);
    T.Principal=TEXT("Pilot/")+FMD5::HashAnsiString(*Bound);T.Owner=FEconomyLedger::Pilot();T.bBuying=CommandIn.Quote.bBuy;
    T.Inventory=T.bBuying?CommandIn.Quote.Destination:CommandIn.Quote.Source;
    T.Market=T.bBuying?CommandIn.Quote.Source:CommandIn.Quote.Destination;T.Item=CommandIn.Quote.Item;T.Quantity=CommandIn.Quote.Quantity;
    T.TotalCredits=CommandIn.Quote.TotalCredits;
    if(!T.Id.IsValid()||T.Quantity<=0||T.TotalCredits<=0){Reason=INVTEXT("Invalid market quote. Nothing exchanged.");return false;}
    auto Body=Request(T,TEXT("prepare_external_trade"),TEXT("/prepare"));Body->SetStringField(TEXT("market"),T.Market.ToString());
    Body->SetStringField(TEXT("item"),T.Item.ToString());Body->SetNumberField(TEXT("quantity"),T.Quantity);Body->SetStringField(TEXT("side"),T.bBuying?TEXT("Buy"):TEXT("Sell"));
    Body->SetStringField(TEXT("expectedTotal"),FString::Printf(TEXT("%lld"),T.TotalCredits));
    const auto Done=MakeShared<FDone>(MoveTemp(Completed));const TWeakObjectPtr<UEconomyServiceSubsystem> Weak(this);bTradePending=true;FString Error;
    const bool Sent=Command(Body,[Weak,T,Bound,Done,Check=MoveTemp(Validate)](bool Ok,TSharedPtr<FJsonObject> Reply,const FString& Failure)mutable{
        auto* Self=Weak.Get();if(!Self)return;
        auto* P=Self->GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();FText Why;FString World;
        bool Prepared=Ok&&Reply&&Reply->TryGetStringField(TEXT("worldId"),World)&&World==T.WorldId&&P&&P->HasLoaded()&&Profile(P)==Bound&&Check&&Check(Why);
        if(Prepared)Prepared=Reply->TryGetNumberField(TEXT("unitTonnes"),T.UnitTonnes)&&Credits(Reply,TEXT("totalCredits"),T.TotalCredits)&&Credits(Reply,TEXT("baseCredits"),T.BaseCredits)&&Credits(Reply,TEXT("taxCredits"),T.TaxCredits);
        if(!Prepared||!NpcTradeAI::Stage(P->GetEconomy(),T,[P](){return P->SaveActivePlayerProgress();},Why)){
            Self->bTradePending=false;
            // No saved player escrow exists. Release NPC preparation when possible;
            // an unreachable service expires it without crediting the player.
            if(Ok){FString Ignored;Self->Command(Request(T,TEXT("cancel_external_trade"),TEXT("/cancel")),{},Ignored);}
            Finish(Done,false,!Why.IsEmpty()?Why.ToString():!Failure.IsEmpty()?Failure:TEXT("The prepared trade changed. No player exchange was applied."));return;
        }
        Self->SettleTrade(T,Bound,[Done](bool Good,const FText& Text,FGuid Id){Finish(Done,Good,Text.ToString(),Id);});
    },Error);
    if(!Sent){bTradePending=false;Reason=FText::FromString(Error);return false;}
    Reason=INVTEXT("Trade submitted. Waiting for saved settlement.");return true;
}
void UEconomyServiceSubsystem::SettleTrade(FNpcTradeReceipt T,const FString& Bound,FDone&& Completed)
{
    const auto Done=MakeShared<FDone>(MoveTemp(Completed));const TWeakObjectPtr<UEconomyServiceSubsystem> Weak(this);
    auto Body=Request(T,TEXT("settle_external_trade"),TEXT("/settle"));Body->SetStringField(TEXT("playerEscrowId"),T.Id.ToString(EGuidFormats::Digits));FString Error;
    if(!Command(Body,[Weak,T,Bound,Done](bool Ok,TSharedPtr<FJsonObject> Reply,const FString& Failure){if(auto* Self=Weak.Get())Self->ApplyTradeReply(T,Bound,Ok,MoveTemp(Reply),Failure,[Done](bool Good,const FText& Text,FGuid Id){Finish(Done,Good,Text.ToString(),Id);});},Error))
    {bTradePending=false;Finish(Done,false,Error+TEXT(" Saved escrow will be reconciled on reconnect."));}
}
void UEconomyServiceSubsystem::ApplyTradeReply(const FNpcTradeReceipt& T,const FString& Bound,bool Ok,TSharedPtr<FJsonObject> Reply,const FString& Failure,FDone&& Completed)
{
    bTradePending=false;const auto Done=MakeShared<FDone>(MoveTemp(Completed));
    auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();FString State,World;FText Why;
    if(!Ok||!Reply||!Reply->TryGetStringField(TEXT("state"),State)||!Reply->TryGetStringField(TEXT("worldId"),World)||World!=T.WorldId||Cached.WorldId!=T.WorldId||
        !Players||!Players->HasLoaded()||Profile(Players)!=Bound)
    {Finish(Done,false,Failure.IsEmpty()?TEXT("Settlement is pending verification. Saved escrow is retained."):Failure);return;}
    if(State!=TEXT("Committed")&&State!=TEXT("Cancelled")){Finish(Done,false,TEXT("Trade is pending. Saved escrow is retained."));return;}
    const bool Committed=State==TEXT("Committed");
    if(!NpcTradeAI::Resolve(Players->GetEconomy(),T.Id,Committed,[Players](){return Players->SaveActivePlayerProgress();},Why))
    {Finish(Done,false,Why.ToString());return;}
    Finish(Done,Committed,Committed?TEXT("Trade settled and saved."):TEXT("Trade cancelled; player escrow returned."),T.Id);
}
void UEconomyServiceSubsystem::RecoverPendingTrades()
{
    if(!RequiresExternal()||!IsReady()||bTradePending)return;
    auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();if(!Players||!Players->HasLoaded())return;
    for(const auto& Pair:Players->GetEconomy().NpcTradeOutbox)
    {
        const auto T=Pair.Value;if(T.Phase!=ENpcTradePhase::Escrowed)continue;
        if(T.WorldId.IsEmpty()||T.WorldId!=Cached.WorldId){Cached.Error=TEXT("A trade belongs to another economy world. Its escrow is retained; no cross-world replay is allowed.");return;}
        bTradePending=true;const auto Bound=Profile(Players);const TWeakObjectPtr<UEconomyServiceSubsystem> Weak(this);FString Error;
        if(!Command(Request(T,TEXT("external_trade_status"),TEXT("/status")),[Weak,T,Bound](bool Ok,TSharedPtr<FJsonObject> Reply,const FString& Failure){
            auto* Self=Weak.Get();if(!Self)return;FString State,World;
            if(Ok&&Reply&&Reply->TryGetStringField(TEXT("worldId"),World)&&World==T.WorldId&&Reply->TryGetStringField(TEXT("state"),State)&&State==TEXT("Prepared"))Self->SettleTrade(T,Bound,{});
            else Self->ApplyTradeReply(T,Bound,Ok,MoveTemp(Reply),Failure,{});
        },Error)){bTradePending=false;Cached.Error=Error;}
        return; // One durable receipt at a time; each heartbeat schedules the next.
    }
}
