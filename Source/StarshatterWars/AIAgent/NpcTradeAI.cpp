#include "NpcTradeAI.h"
#include "TransactionAI.h"
#include "SimDataEvents.h"
namespace NpcTradeAI
{
FName Escrow(FGuid Id){return FName(*(TEXT("Player trade escrow/")+Id.ToString(EGuidFormats::Digits)));}
namespace
{
struct FSilence {FSilence(){FSimDataEvents::Suppress(true);}~FSilence(){FSimDataEvents::Suppress(false);}};
bool Fail(FText& Reason,const TCHAR* Text){Reason=FText::FromString(Text);return false;}
bool Same(const FNpcTradeReceipt& A,const FNpcTradeReceipt& B)
{return A.Id==B.Id&&A.Principal==B.Principal&&A.WorldId==B.WorldId&&A.Owner==B.Owner&&A.Inventory==B.Inventory&&A.Market==B.Market&&A.Item==B.Item&&A.Quantity==B.Quantity&&A.UnitTonnes==B.UnitTonnes&&A.TotalCredits==B.TotalCredits&&A.BaseCredits==B.BaseCredits&&A.TaxCredits==B.TaxCredits&&A.bBuying==B.bBuying;}
template<typename F>bool Commit(FEconomyState& State,F&& Mutation,TFunctionRef<bool()> Save,FText& Reason)
{
    auto Candidate=State;bool Accepted=false;
    {FSilence Quiet;Accepted=Mutation(Candidate);}
    if(!Accepted)return false;
    auto Before=MoveTemp(State);State=MoveTemp(Candidate);
    if(!Save()){State=MoveTemp(Before);return Fail(Reason,TEXT("Could not save the trade. Player credits and cargo are unchanged."));}
    FSimDataEvents::Publish(State);return true;
}
void Describe(FEconomyState& State,const FNpcTradeReceipt& Trade)
{
    if(State.Journal.IsEmpty())return;
    auto& Entry=State.Journal.Last();Entry.Reference=Trade.Id;Entry.Category=EWalletTransactionCategory::Market;
    Entry.Item=Trade.Item;Entry.Quantity=Trade.Quantity;Entry.UnitPrice=double(Trade.TotalCredits)/Trade.Quantity;
    Entry.Source=Trade.bBuying?Trade.Market:Trade.Inventory;Entry.Destination=Trade.bBuying?Trade.Inventory:Trade.Market;
    Entry.BaseCredits=Trade.BaseCredits;Entry.TaxCredits=Trade.TaxCredits;
}
}
bool Stage(FEconomyState& State,const FNpcTradeReceipt& Trade,TFunctionRef<bool()> Save,FText& Reason)
{
    Reason=FText::GetEmpty();
    if(const auto* Prior=State.NpcTradeOutbox.Find(Trade.Id))
        return Same(*Prior,Trade)||Fail(Reason,TEXT("This trade identifier already belongs to another request."));
    if(Trade.WorldId.IsEmpty()||Trade.WorldId.Len()>128||!Trade.Id.IsValid()||!Trade.Principal.StartsWith(TEXT("Pilot/"))||Trade.Principal.Len()>128||!FEconomyLedger::IsPlayerOwner(State,Trade.Owner)||Trade.Inventory.IsNone()||Trade.Item.IsNone()||Trade.Market.IsNone()||
        !FMath::IsFinite(Trade.Quantity)||Trade.Quantity<=0||Trade.Quantity>1.e6||!FMath::IsFinite(Trade.UnitTonnes)||Trade.UnitTonnes<=0||!FMath::IsFinite(Trade.Quantity*Trade.UnitTonnes)||Trade.Quantity*Trade.UnitTonnes>1.e15||
        Trade.TotalCredits<=0||Trade.TotalCredits>9000000000000000LL||Trade.BaseCredits<0||Trade.TaxCredits<0||Trade.BaseCredits>Trade.TotalCredits||Trade.TotalCredits-Trade.BaseCredits!=Trade.TaxCredits)
        return Fail(Reason,TEXT("Invalid prepared NPC trade. Nothing exchanged."));
    return Commit(State,[&](FEconomyState& Work){
        auto* Hold=Work.Inventories.Find(Trade.Inventory);const double Mass=Trade.Quantity*Trade.UnitTonnes;const FName Pending=Escrow(Trade.Id);
        if(!Hold||Hold->Owner!=Trade.Owner||!Hold->bOperational||Work.Inventories.Contains(Pending)||Work.Wallets.Contains(Pending))return Fail(Reason,TEXT("The player's selected inventory is unavailable."));
        if(Trade.bBuying){
            if(FEconomyLedger::Used(*Hold)+Hold->ReservedOutputTonnes+Mass>Hold->CapacityTonnes+UE_SMALL_NUMBER||Work.Wallets.FindRef(Trade.Owner)<Trade.TotalCredits)
                return Fail(Reason,TEXT("Insufficient player credits or cargo space."));
            if(Trade.TotalCredits>0&&!FEconomyLedger::Pay(Work,Trade.Owner,Pending,Trade.TotalCredits,FGuid::NewGuid(),TEXT("Pending NPC purchase escrow")))return false;
            Describe(Work,Trade);Hold->ReservedOutputTonnes+=Mass;
        }else{
            const auto System=Hold->System;
            if(!FEconomyLedger::CreateInventory(Work,Pending,Trade.Owner,System,Mass)||!FEconomyLedger::Transfer(Work,Trade.Owner,Trade.Inventory,Pending,Trade.Item,Trade.Quantity))return Fail(Reason,TEXT("Sale cargo is unavailable or reserved by another operation."));
            Work.Inventories.FindChecked(Trade.Inventory).ReservedOutputTonnes+=Mass;
        }
        auto Stored=Trade;Stored.Phase=ENpcTradePhase::Escrowed;Work.NpcTradeOutbox.Add(Trade.Id,MoveTemp(Stored));return true;
    },Save,Reason);
}
bool Resolve(FEconomyState& State,FGuid Id,bool Committed,TFunctionRef<bool()> Save,FText& Reason)
{
    Reason=FText::GetEmpty();const auto* Prior=State.NpcTradeOutbox.Find(Id);
    if(!Prior)return Fail(Reason,TEXT("The player has no durable escrow for this receipt."));
    if(Prior->Phase!=ENpcTradePhase::Escrowed)return (Committed?Prior->Phase==ENpcTradePhase::Applied:Prior->Phase==ENpcTradePhase::Refunded)||Fail(Reason,TEXT("The settlement contradicts an already applied player receipt."));
    return Commit(State,[&](FEconomyState& Work){
        auto& Trade=Work.NpcTradeOutbox.FindChecked(Id);const auto Pending=Escrow(Id);const double Mass=Trade.Quantity*Trade.UnitTonnes;
        auto* Hold=Work.Inventories.Find(Trade.Inventory);
        if(!Hold||Hold->Owner!=Trade.Owner||!Hold->bOperational)return Fail(Reason,TEXT("Trade delivery is waiting for its player inventory. The receipt is retained."));
        Hold->ReservedOutputTonnes=FMath::Max(0.0,Hold->ReservedOutputTonnes-Mass);
        if(Trade.bBuying){
            if(Work.Wallets.FindRef(Pending)<Trade.TotalCredits)return Fail(Reason,TEXT("Trade payment escrow is incomplete. No delivery was applied."));
            if(Committed){
                if(!FEconomyLedger::Deposit(Work,Trade.Inventory,Trade.Item,Trade.Quantity,Trade.UnitTonnes))return Fail(Reason,TEXT("Trade delivery is waiting for cargo space."));
                if(Trade.TotalCredits>0&&!TransactionAI::ReconcileExternalBalance(Work,Pending,Work.Wallets.FindRef(Pending)-Trade.TotalCredits,TEXT("NPC purchase settled"),Trade.Market,EWalletTransactionCategory::Market,Id))return false;
                Describe(Work,Trade);
            }else if(Trade.TotalCredits>0&&!FEconomyLedger::Pay(Work,Pending,Trade.Owner,Trade.TotalCredits,FGuid::NewGuid(),TEXT("NPC purchase escrow refunded")))return false;
        }else if(Committed){
            const auto Balance=Work.Wallets.FindRef(Trade.Owner);
            if(Balance>MAX_int64-Trade.BaseCredits||!FEconomyLedger::Withdraw(Work,Trade.Owner,Pending,Trade.Item,Trade.Quantity))return Fail(Reason,TEXT("Sale settlement is waiting for its escrow cargo or wallet."));
            if(Trade.BaseCredits>0&&!TransactionAI::ReconcileExternalBalance(Work,Trade.Owner,Balance+Trade.BaseCredits,TEXT("NPC sale settled"),Trade.Market,EWalletTransactionCategory::Market,Id))return false;
            Describe(Work,Trade);
        }else if(!FEconomyLedger::Transfer(Work,Trade.Owner,Pending,Trade.Inventory,Trade.Item,Trade.Quantity))return Fail(Reason,TEXT("Sale refund is waiting for cargo space. Its escrow is retained."));
        Trade.Phase=Committed?ENpcTradePhase::Applied:ENpcTradePhase::Refunded;
        return true;
    },Save,Reason);
}
bool HasPending(const FEconomyState& State)
{for(const auto& Pair:State.NpcTradeOutbox)if(Pair.Value.Phase==ENpcTradePhase::Escrowed)return true;return false;}
}
