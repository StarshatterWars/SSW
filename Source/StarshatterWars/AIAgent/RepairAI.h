#pragma once
#include "CoreMinimal.h"
#include "RepairTypes.h"
struct FCampaignFleetState;
struct FCampaignFleetHull;
struct FEconomyState;

struct FFleetRepairPolicy
{
 int32 Teams=2;
 double RepairSecondsPerMassRoot=180;
 double RebuildSecondsPerMassRoot=1800;
 double PartsPerMassRoot=1;
 double AlloyPerMassRoot=4;
 double InitialParts=1000;
 double InitialAlloy=5000;
 double InitialKits=100;
 bool LoadJson(const FString&,FText& Error);
};
struct FRepairHullSpec
{
 double Mass=1;
 bool bFighter=false;
};
struct FFleetRepairContext
{
 double Now=0;
 int32 AlliedFaction=1;
 FName SupplySystem;
 TSet<FGuid> Deployed;
 TSet<FName> AuthorizedShipyards;
 TMap<FName,FRepairHullSpec> Designs;
};
struct FFleetRepairStep
{
 bool bChanged=false;
 double NextAt=0;
};
// No Actor, UObject, wall-clock or frame-tick dependency. The host supplies an
// authoritative clock, deployment snapshot and transactional persistence.
class RepairAI
{
public:
 static FFleetRepairStep Advance(FCampaignFleetState&,FEconomyState&,const FFleetRepairContext&,const FFleetRepairPolicy&);
 static bool Commit(FCampaignFleetState&,FEconomyState&,const FFleetRepairContext&,const FFleetRepairPolicy&,
                    TFunctionRef<bool()> PersistEconomy,FFleetRepairStep& Result);
 static FName Depot(const FCampaignFleetState&,int32 Faction,FName System);
 static FString Describe(const FCampaignFleetState&,const FEconomyState&,FGuid Hull,double Now);
 static bool NeedsRepair(const FPlayerHangarShip&);
};
