#pragma once
#include "CoreMinimal.h"
#include "GameStructs_insurance.h"
#include "CampaignFleet.generated.h"

USTRUCT()
struct FCampaignFleetHull
{
 GENERATED_BODY()
 UPROPERTY(SaveGame) FGuid HullId;
 UPROPERTY(SaveGame) FGuid Replaces;
 UPROPERTY(SaveGame) FString UnitKey;
 UPROPERTY(SaveGame) FString Name;
 UPROPERTY(SaveGame) FName Design;
 UPROPERTY(SaveGame) FString Assignment;
 UPROPERTY(SaveGame) FString System;
 UPROPERTY(SaveGame) FString Region;
 UPROPERTY(SaveGame) FVector Position=FVector::ZeroVector;
 UPROPERTY(SaveGame) int32 Faction=0;
 UPROPERTY(SaveGame) int32 Slot=0;
 UPROPERTY(SaveGame) bool bDestroyed=false;
 UPROPERTY(SaveGame) bool bRepairing=false;
 UPROPERTY(SaveGame) bool bHero=false;
 UPROPERTY(SaveGame) bool bStoryRequired=false;
 UPROPERTY(SaveGame) bool bStatic=false;
 UPROPERTY(SaveGame) bool bTransferred=false;
 UPROPERTY(SaveGame) FPlayerHangarShip Condition;
};
USTRUCT()
struct FCampaignFleetUnitSlots
{
 GENERATED_BODY()
 UPROPERTY(SaveGame) TArray<FGuid> HullIds;
};
USTRUCT()
struct FCampaignFleetState
{
 GENERATED_BODY()
 bool bDirty=true;
 UPROPERTY(SaveGame) FGuid FleetId;
 UPROPERTY(SaveGame) TMap<FGuid,double> AppliedRepairProgress;
 UPROPERTY(SaveGame) bool bInitialized=false;
 UPROPERTY(SaveGame) int32 CampaignId=0;
 UPROPERTY(SaveGame) bool bHeroLost=false;
 UPROPERTY(SaveGame) TMap<FGuid,FCampaignFleetHull> Hulls;
 UPROPERTY(SaveGame) TMap<FString,FCampaignFleetUnitSlots> Units;
 UPROPERTY(SaveGame) TSet<FString> ClearedMinefields;
 UPROPERTY(SaveGame) TMap<FString,int32> LastAppliedUnavailable;
};
class Campaign;
class CombatUnit;
class Ship;
class Sim;
DECLARE_MULTICAST_DELEGATE(FSSWCampaignFleetChanged);
class FCampaignFleet
{
public:
 static FSSWCampaignFleetChanged& Changed();
 static void Notify();
 static FString UnitKey(const CombatUnit&);
 static void Prepare(Campaign&);
 static void RestoreLocations(Campaign&);
 static void Capture(Campaign&,Sim&);
 static void CaptureHull(Campaign&,Ship&);
 static FGuid ResolveSlot(Campaign&,const CombatUnit*,int32 Slot);
 static FGuid Resolve(Campaign&,const CombatUnit*,int32 AvailableIndex,bool MobileOnly=true);
 static void Apply(Campaign&,Ship&);
 static void Destroy(Campaign&,FGuid);
 static FString Status(const FCampaignFleetHull&);
 static bool CanMove(const FCampaignFleetHull&);
 static bool CanMove(class Ship&);
 static bool MissionReady(Campaign&,class Mission&);
 static bool Ready(const FCampaignFleetState&,const FString& Unit);
 static FString MinefieldKey(class MissionElement&);
 static void CommitObjectives(Campaign&,class Mission&);
 static bool ClearMinefield(FCampaignFleetState&,const FString& Key,bool DesignatedObjective,bool Successful);
};
