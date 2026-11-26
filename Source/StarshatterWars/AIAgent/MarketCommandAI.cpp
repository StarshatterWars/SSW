#include "MarketCommandAI.h"
#include "SimDataEvents.h"

namespace
{
void CrossNPC(FEconomyState& State,FGuid Id,int64 Ask,int64 Bid,int64 Now)
{
    const auto* Found=State.MarketOrders.Find(Id);if(!Found || Found->State!=EMarketOrderState::Open)return;
    const auto Order=*Found;const auto* NPC=State.Inventories.Find(Order.Market);if(!NPC)return;
    const FName Owner=NPC->Owner;double Count=0;int64 Price=0;
    const auto Side=Order.Side==EMarketOrderSide::Buy?EMarketOrderSide::Sell:EMarketOrderSide::Buy;
    if(Order.Side==EMarketOrderSide::Buy && Ask>0 && Order.UnitPrice>=Ask)
    {Price=Ask;Count=FMath::Min(Order.Remaining,FMath::FloorToDouble(FEconomyLedger::Available(State,Order.Market,Order.Item)));}
    else if(Order.Side==EMarketOrderSide::Sell && Bid>0 && Order.UnitPrice<=Bid)
    {
        Price=Bid;Count=FMath::Min3(Order.Remaining,FMath::FloorToDouble(double(State.Wallets.FindRef(Owner))/Price),
            FMath::FloorToDouble(FMath::Max(0.0,NPC->CapacityTonnes-FEconomyLedger::Used(*NPC))/Order.UnitTonnes));
    }
    if(Count<=0)return;
    FText Reason;const FGuid Counter=FEconomyLedger::PlaceMarketOrder(State,Owner,Order.Market,Order.Market,Order.Item,Side,Count,Order.UnitTonnes,Price,Now,Now+60,FGuid::NewGuid(),Reason);
    FEconomyLedger::AdvanceMarketOrders(State,Now);
    if(Counter.IsValid())if(const auto* Remaining=State.MarketOrders.Find(Counter);Remaining && Remaining->State==EMarketOrderState::Open)
        FEconomyLedger::CancelMarketOrder(State,Owner,Counter,FGuid::NewGuid(),Reason);
}
}
FNativeMarketReply MarketCommandAI::Execute(FEconomyState& State,const FNativeMarketCommand& C)
{
    FScopedSimDataChangeBatch Batch(State);FNativeMarketReply Out;Out.Receipt=C.Receipt;Out.Order=C.Order;Out.SourceOrders=State.MarketOrders;
    if(!C.Receipt.IsValid() || State.CommittedTransactions.Contains(C.Receipt) || C.NowUtc<0 || C.NowUtc>MAX_int64-86400)
    {Out.Reason=INVTEXT("Invalid or already processed market command. Nothing exchanged.");return Out;}
    const auto Before=State;
    if(C.Action==EMarketCommand::Trade)
        Out.bSucceeded=FStationServices::CommitMarketTrade(State,C.Quote,C.Receipt,[]{return true;},Out.Reason);
    else if(C.Action==EMarketCommand::PlaceBuy || C.Action==EMarketCommand::PlaceSell)
    {
        Out.Order=FEconomyLedger::PlaceMarketOrder(State,FEconomyLedger::Pilot(),C.Market,C.Inventory,C.Item,
            C.Action==EMarketCommand::PlaceBuy?EMarketOrderSide::Buy:EMarketOrderSide::Sell,C.Quantity,C.UnitTonnes,C.LimitPrice,C.NowUtc,C.NowUtc+86400,C.Receipt,Out.Reason,ECommercePurpose::Brokerage);
        Out.bSucceeded=Out.Order.IsValid();
    }
    else if(C.Action==EMarketCommand::Modify)
        Out.bSucceeded=FEconomyLedger::ModifyMarketOrder(State,FEconomyLedger::Pilot(),C.Order,C.LimitPrice,C.Receipt,Out.Reason,ECommercePurpose::Brokerage);
    else if(C.Action==EMarketCommand::Cancel)
        Out.bSucceeded=FEconomyLedger::CancelMarketOrder(State,FEconomyLedger::Pilot(),C.Order,C.Receipt,Out.Reason);
    if(!Out.bSucceeded){State=Before;return Out;}
    if(C.Action!=EMarketCommand::Trade)
    {
        if(C.Action!=EMarketCommand::Cancel)CrossNPC(State,Out.Order,C.Ask,C.Bid,C.NowUtc);
        FEconomyLedger::AdvanceMarketOrders(State,C.NowUtc);
        Out.Reason=INVTEXT("ORDER CHANGE SAVED. REAL STOCK AND FUNDED COUNTERPARTS ONLY; OPEN REMAINDERS STAY RESERVED.");
    }
    return Out;
}
