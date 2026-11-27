#include "EconomyOwnership.h"
#include "GameStructs_economy.h"
#include "EconomyRuntime.h"
#include "EconomyAI.h"
namespace SSWEconomyOwnership
{
bool IsNpcOwner(const FEconomyState& State,FName Owner)
{return Owner==FEconomyRuntime::IndustryOwner()&&!FEconomyLedger::IsPlayerOwner(State,Owner);}
namespace
{
TSet<FName> Accounts(const FEconomyState& State)
{
    TSet<FName> Result;
    for(const auto& Pair:State.Inventories)if(IsNpcOwner(State,Pair.Value.Owner))
    {
        Result.Add(Pair.Value.Owner);
        Result.Add(Pair.Value.TradingOwner.IsNone()?EconomyAI::NPCAccount(Pair.Key):Pair.Value.TradingOwner);
        Result.Add(FName(*(TEXT("Residents/")+Pair.Value.System.ToString())));
    }
    for(auto It=Result.CreateIterator();It;++It)if(FEconomyLedger::IsPlayerOwner(State,*It))It.RemoveCurrent();
    return Result;
}
}
FEconomyState ExtractNpc(const FEconomyState& Source)
{
    FEconomyState Out;const auto OwnedAccounts=Accounts(Source);
    for(const auto& P:Source.Inventories)if(IsNpcOwner(Source,P.Value.Owner))Out.Inventories.Add(P.Key,P.Value);
    for(const auto& P:Source.Wallets)if(OwnedAccounts.Contains(P.Key))Out.Wallets.Add(P.Key,P.Value);
    for(const auto& P:Source.Reservations)if(IsNpcOwner(Source,P.Value.Owner)&&Out.Inventories.Contains(P.Value.Inventory))Out.Reservations.Add(P.Key,P.Value);
    for(const auto& Job:Source.Jobs)if(IsNpcOwner(Source,Job.Owner))Out.Jobs.Add(Job);
    for(const auto& P:Source.Blueprints)if(IsNpcOwner(Source,P.Value.Owner))Out.Blueprints.Add(P.Key,P.Value);
    for(const auto& P:Source.MarketOrders)if(IsNpcOwner(Source,P.Value.Owner))Out.MarketOrders.Add(P.Key,P.Value);
    Out.IndustryPolicies=Source.IndustryPolicies;Out.RecipeActivities=Source.RecipeActivities;
    Out.BackgroundOreRemaining=Source.BackgroundOreRemaining;Out.BackgroundMineDeadlines=Source.BackgroundMineDeadlines;Out.BackgroundMineLastCycle=Source.BackgroundMineLastCycle;
    Out.NextBackgroundMineAt=Source.NextBackgroundMineAt;Out.LastBackgroundMineAt=Source.LastBackgroundMineAt;
    Out.NextRegionalPriceAt=Source.NextRegionalPriceAt;Out.LastRegionalPriceAt=Source.LastRegionalPriceAt;
    Out.RegionalMarkets=Source.RegionalMarkets;Out.EconomyClockSeconds=Source.EconomyClockSeconds;
    Out.EconomyStepSeconds=Source.EconomyStepSeconds;Out.NPCIncomeSeconds=Source.NPCIncomeSeconds;
    Out.OfflineFacilities=Source.OfflineFacilities;Out.GroundFactories=Source.GroundFactories;
    // Pilot contracts, journal/receipts, licenses, equipment, repairs and hull ownership are not imported.
    return Out;
}
bool MatchesNpc(const FEconomyState& A,const FEconomyState& B)
{
    const auto Left=ExtractNpc(A),Right=ExtractNpc(B);
    return FEconomyState::StaticStruct()->CompareScriptStruct(&Left,&Right,0);
}
void ApplyNpcProjection(FEconomyState& Local,const FEconomyState& Remote)
{
    const auto PriorAccounts=Accounts(Local);const auto Incoming=ExtractNpc(Remote);
    for(auto It=Local.Inventories.CreateIterator();It;++It)if(IsNpcOwner(Local,It.Value().Owner))It.RemoveCurrent();
    for(const auto& P:Incoming.Inventories)if(!Local.Inventories.Contains(P.Key))Local.Inventories.Add(P.Key,P.Value);
    for(FName Account:PriorAccounts)if(!FEconomyLedger::IsPlayerOwner(Local,Account))Local.Wallets.Remove(Account);
    for(const auto& P:Incoming.Wallets)if(!FEconomyLedger::IsPlayerOwner(Local,P.Key))Local.Wallets.Add(P.Key,P.Value);
    for(auto It=Local.Reservations.CreateIterator();It;++It)if(IsNpcOwner(Local,It.Value().Owner))It.RemoveCurrent();
    for(const auto& P:Incoming.Reservations)if(!Local.Reservations.Contains(P.Key))Local.Reservations.Add(P.Key,P.Value);
    Local.Jobs.RemoveAll([&](const auto& Job){return IsNpcOwner(Local,Job.Owner);});
    for(const auto& Job:Incoming.Jobs)if(!Local.Jobs.ContainsByPredicate([&](const auto& Existing){return Existing.Id==Job.Id;}))Local.Jobs.Add(Job);
    for(auto It=Local.Blueprints.CreateIterator();It;++It)if(IsNpcOwner(Local,It.Value().Owner))It.RemoveCurrent();
    for(const auto& P:Incoming.Blueprints)if(!Local.Blueprints.Contains(P.Key))Local.Blueprints.Add(P.Key,P.Value);
    for(auto It=Local.MarketOrders.CreateIterator();It;++It)if(IsNpcOwner(Local,It.Value().Owner))It.RemoveCurrent();
    for(const auto& P:Incoming.MarketOrders)if(!Local.MarketOrders.Contains(P.Key))Local.MarketOrders.Add(P.Key,P.Value);
    Local.RegionalMarkets=Incoming.RegionalMarkets;
    Local.BackgroundOreRemaining=Incoming.BackgroundOreRemaining;Local.BackgroundMineDeadlines=Incoming.BackgroundMineDeadlines;Local.BackgroundMineLastCycle=Incoming.BackgroundMineLastCycle;
    Local.NextBackgroundMineAt=Incoming.NextBackgroundMineAt;Local.LastBackgroundMineAt=Incoming.LastBackgroundMineAt;
    Local.NextRegionalPriceAt=Incoming.NextRegionalPriceAt;Local.LastRegionalPriceAt=Incoming.LastRegionalPriceAt;
    // Clock, policy and facility lifecycle activation belong to the verified connection/handoff, not a display merge.
}
}
