#pragma once
#include "GameStructs_economy.h"
#include "ContractTransactions.h"

enum class ELedgerCommand : uint8
{ Transfer,BlueprintPurchase,IndustryInstall,IndustryCancel,ContractAccept,ContractBid,ContractDeliver,ContractAbandon,ContractClaim,ContractMaintain,MarketMaintain };
struct FNativeLedgerCommand
{
    ELedgerCommand Action=ELedgerCommand::Transfer;
    FGuid Receipt,Object;
    FName Source,Destination,System,Item,Facility,Recipe,Contract,Hold;
    double Quantity=0,Capacity=0,Duration=0;
    int64 Credits=0,NowUtc=0;
    int32 Runs=1;
    bool bOriginal=false,bSeedTraining=false;
    TSet<FName> LostPackages;
    TArray<FCommodityAmount> Materials,Products;
};
struct FNativeLedgerReply
{
    bool bSucceeded=false,bChanged=false,bContracts=false,bOrders=false;
    FGuid Object;
    FText Reason;
    TMap<FName,FContractTransaction> SourceContracts;
    TMap<FGuid,FMarketOrder> SourceOrders;
};
namespace LedgerCommandAI
{
    FNativeLedgerReply Execute(FEconomyState&,const FNativeLedgerCommand&);
}
