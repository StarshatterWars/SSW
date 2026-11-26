#include "HullRegistry.h"

FGuid FHullRegistry::Allocate(){return FGuid::NewGuid();}
FString FHullRegistry::Number(FGuid Id){return Id.IsValid()?TEXT("SSW-")+Id.ToString(EGuidFormats::Digits).ToUpper():FString();}
bool FHullRegistry::Register(FGuid Id,FName Design,const FString& Name,FName Owner,int32 Faction)
{
 if(!Id.IsValid() || Design.IsNone())return false;
 if(const auto* Existing=Find(Id))return Existing->Design==Design && Existing->Status!=EHullOperationalState::Destroyed;
 FPermanentHullRecord Record;Record.HullId=Id;Record.Design=Design;Record.DisplayName=Name;
 Record.HullNumber=Number(Id);Record.LegalOwner=Record.Possessor=Owner;Record.Faction=Faction;
 State.Hulls.Add(Id,MoveTemp(Record));return true;
}
bool FHullRegistry::Import(const FHullRegistryState& Imported,FString& Error)
{
 // Validate the entire import before mutating the registry. A conflicting copied
 // record must never rename/retype an unrelated hull or silently resurrect it.
 TSet<FString> Numbers;
 for(const auto& Pair:Imported.Hulls)
 {
  const auto& Record=Pair.Value;
  if(!Pair.Key.IsValid() || Record.HullId!=Pair.Key || Record.Design.IsNone() || Record.HullNumber!=Number(Pair.Key) || Numbers.Contains(Record.HullNumber))
  {Error=TEXT("Invalid or duplicate hull registration.");return false;}
  if(const auto* Existing=Find(Pair.Key);Existing && (Existing->Design!=Record.Design || Existing->HullNumber!=Record.HullNumber ||
      (Existing->Status==EHullOperationalState::Destroyed && Record.Status!=EHullOperationalState::Destroyed)))
  {Error=TEXT("Hull identity collision or attempted resurrection.");return false;}
  Numbers.Add(Record.HullNumber);
 }
 for(const auto& Pair:Imported.Hulls)
 {
  if(!State.Hulls.Contains(Pair.Key))State.Hulls.Add(Pair.Key,Pair.Value);
  else if(Pair.Value.Status==EHullOperationalState::Destroyed)State.Hulls.FindChecked(Pair.Key).Status=EHullOperationalState::Destroyed;
 }
 Error.Reset();return true;
}
bool FHullRegistry::Transfer(FGuid Id,FName Owner,FName Possessor,int64 Utc,const FString& Reason)
{
 auto* Record=State.Hulls.Find(Id);
 if(!Record || Owner.IsNone() || Possessor.IsNone() || Utc<0 || Reason.IsEmpty() || Record->Status==EHullOperationalState::Destroyed)return false;
 if(Record->LegalOwner!=Owner){FHullOwnershipEvent Event;Event.Utc=Utc;Event.PreviousOwner=Record->LegalOwner;Event.NewOwner=Owner;Event.Reason=Reason;Record->OwnershipHistory.Add(Event);}
 // An authorized custody transaction (including a rental) does not confer
 // title and does not make the possessor a thief. Unapproved custody is
 // reported separately through SetPossessor.
 Record->LegalOwner=Owner;Record->Possessor=Possessor;Record->bStolen=false;return true;
}
bool FHullRegistry::SetPossessor(FGuid Id,FName Possessor)
{
 auto* Record=State.Hulls.Find(Id);if(!Record || Possessor.IsNone() || Record->Status==EHullOperationalState::Destroyed)return false;
 Record->Possessor=Possessor;Record->bStolen=Possessor!=Record->LegalOwner;return true;
}
bool FHullRegistry::Rename(FGuid Id,const FString& Name)
{auto* Record=State.Hulls.Find(Id);if(!Record || Name.IsEmpty())return false;Record->DisplayName=Name;return true;}
bool FHullRegistry::SetStatus(FGuid Id,EHullOperationalState Status)
{auto* Record=State.Hulls.Find(Id);if(!Record || (Record->Status==EHullOperationalState::Destroyed && Status!=EHullOperationalState::Destroyed))return false;Record->Status=Status;return true;}
