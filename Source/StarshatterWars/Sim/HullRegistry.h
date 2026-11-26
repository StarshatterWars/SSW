#pragma once
#include "CoreMinimal.h"
#include "HullRegistry.generated.h"

UENUM()
enum class EHullOperationalState:uint8 { Operational, Damaged, Repairing, Destroyed, Retired };
USTRUCT()
struct FHullOwnershipEvent
{
 GENERATED_BODY()
 UPROPERTY(SaveGame) int64 Utc=0;
 UPROPERTY(SaveGame) FName PreviousOwner;
 UPROPERTY(SaveGame) FName NewOwner;
 UPROPERTY(SaveGame) FString Reason;
};
USTRUCT()
struct FPermanentHullRecord
{
 GENERATED_BODY()
 UPROPERTY(SaveGame) FGuid HullId;
 UPROPERTY(SaveGame) FName Design;
 UPROPERTY(SaveGame) FString DisplayName;
 UPROPERTY(SaveGame) FString HullNumber;
 UPROPERTY(SaveGame) FName LegalOwner;
 UPROPERTY(SaveGame) FName Possessor;
 UPROPERTY(SaveGame) int32 Faction=0;
 UPROPERTY(SaveGame) bool bStolen=false;
 UPROPERTY(SaveGame) EHullOperationalState Status=EHullOperationalState::Operational;
 UPROPERTY(SaveGame) TArray<FHullOwnershipEvent> OwnershipHistory;
};
USTRUCT()
struct FHullRegistryState
{
 GENERATED_BODY()
 UPROPERTY(SaveGame) TMap<FGuid,FPermanentHullRecord> Hulls;
};

// Simulation-owned plain data. Callers serialize access through the owning
// simulation domain; no UObject/Actor ownership or wall-clock polling lives here.
class STARSHATTERWARS_API FHullRegistry
{
public:
 static FGuid Allocate();
 static FString Number(FGuid Id);
 bool Register(FGuid Id,FName Design,const FString& Name,FName Owner,int32 Faction);
 bool Import(const FHullRegistryState& Imported,FString& Error);
 bool Transfer(FGuid Id,FName NewLegalOwner,FName NewPossessor,int64 Utc,const FString& Reason);
 bool SetPossessor(FGuid Id,FName Possessor);
 bool Rename(FGuid Id,const FString& Name);
 bool SetStatus(FGuid Id,EHullOperationalState Status);
 const FPermanentHullRecord* Find(FGuid Id)const{return State.Hulls.Find(Id);}
 const FHullRegistryState& Snapshot()const{return State;}
private:
 FHullRegistryState State;
};
