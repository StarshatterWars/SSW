#pragma once
#include "StationServices.h"

enum class EMarketCommand : uint8 { Trade, PlaceBuy, PlaceSell, Modify, Cancel };
// Prepared by the authoritative station adapter; value data only.
struct FNativeMarketCommand
{
    EMarketCommand Action=EMarketCommand::Trade;
    FStationMarketQuote Quote;
    FName Market,Inventory,Item;
    FGuid Receipt,Order;
    double Quantity=0,UnitTonnes=1;
    int64 LimitPrice=0,Ask=0,Bid=0,NowUtc=0;
};
struct FNativeMarketReply
{
    bool bSucceeded=false;
    FText Reason;
    FGuid Receipt,Order;
    TMap<FGuid,FMarketOrder> SourceOrders;
};
namespace MarketCommandAI
{
    FNativeMarketReply Execute(FEconomyState&,const FNativeMarketCommand&);
}
