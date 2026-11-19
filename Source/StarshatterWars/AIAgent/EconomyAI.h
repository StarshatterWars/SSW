#pragma once
#include "GameStructs_economy.h"
#include "GameStructs_products.h"
class UCargoCatalogSubsystem;
// The sim owns this controller; widgets only inspect its persisted results.
class EconomyAI
{
public:
    static bool AdvanceMine(FEconomyState&,FName Deposit,FName Inventory,FName System,FName Mineral,double InitialTonnes,double TonnesPerSecond,double Capacity,double Seconds);
    static void AdvanceFacilities(FEconomyState&,const TMap<FName,FSSWProductionFacility>&,const TMap<FName,FSSWProductionRecipe>&,double Seconds);
    bool Advance(FEconomyState&,const UCargoCatalogSubsystem&,double Seconds,int64 NowUtc);
    static FName NPCAccount(FName Inventory);
    static double PriceTrend(const FEconomyState&,FName System,FName Item);
    static void RecordTrade(FEconomyState&,FName System,FName Item,double Quantity,double TotalCredits);
    static bool Snapshot(FEconomyState&,FName System,FName Item,double Supply,double Demand,double BasePrice,int64 NowUtc);
    static FName Key(FName System,FName Item);
    static double PriceFactor(double Stock,double Demand);
    static double FreightEstimate(double Tonnes,int32 Jumps);
    static const FRegionalMarketHistory* History(const FEconomyState&,FName System,FName Item);
};
