#include "SerializedHullStockAI.h"
#include "HullRegistry.h"

bool SerializedHullStockAI::Ensure(FEconomyState& State,FName Inventory,FName Design,int32 Count)
{
 if(Inventory.IsNone() || Design.IsNone() || Count<0 || Count>10000)return false;
 TArray<FGuid> Active;
 for(const auto& Pair:State.SerializedHulls)
     if(Pair.Value.Inventory==Inventory && Pair.Value.Design==Design && !Pair.Value.SoldReceipt.IsValid() && !Pair.Value.bWithdrawn)Active.Add(Pair.Key);
 Active.Sort([](const FGuid& A,const FGuid& B){return A.ToString()<B.ToString();});
 bool Changed=false;
 for(int32 Index=Active.Num()-1;Index>=0 && Active.Num()>Count;--Index)
 {
  auto& Record=State.SerializedHulls.FindChecked(Active[Index]);
  if(Record.ReservedBy.IsValid())continue;
  Record.bWithdrawn=true;Active.RemoveAt(Index);Changed=true;
 }
 while(Active.Num()<Count)
 {
  FSerializedHullStock Record;do{Record.HullId=FHullRegistry::Allocate();}while(State.SerializedHulls.Contains(Record.HullId));
  Record.Inventory=Inventory;Record.Design=Design;Active.Add(Record.HullId);State.SerializedHulls.Add(Record.HullId,Record);Changed=true;
 }
 return Changed;
}
TArray<FGuid> SerializedHullStockAI::Available(const FEconomyState& State,FName Inventory,FName Design,int32 Count)
{
 TArray<FGuid> Out;
 for(const auto& Pair:State.SerializedHulls)
     if(Pair.Value.Inventory==Inventory && Pair.Value.Design==Design && !Pair.Value.SoldReceipt.IsValid() && !Pair.Value.bWithdrawn && !Pair.Value.ReservedBy.IsValid())Out.Add(Pair.Key);
 Out.Sort([](const FGuid& A,const FGuid& B){return A.ToString()<B.ToString();});
 Out.SetNum(FMath::Min(Out.Num(),FMath::Max(0,Count)));return Out;
}
bool SerializedHullStockAI::Reserve(FEconomyState& State,FGuid Hull,FGuid Receipt,FName Inventory,FName Design)
{
 auto* Record=State.SerializedHulls.Find(Hull);
 if(!Receipt.IsValid() || !Record || Record->HullId!=Hull || Record->Inventory!=Inventory || Record->Design!=Design || Record->bWithdrawn || Record->SoldReceipt.IsValid() || Record->ReservedBy.IsValid())return false;
 if(State.HullPurchaseReceipts.Contains(Receipt))return false;
 for(const auto& Pair:State.SerializedHulls)if(Pair.Value.ReservedBy==Receipt)return false;
 Record->ReservedBy=Receipt;return true;
}
bool SerializedHullStockAI::Deliver(FEconomyState& State,FGuid Hull,FGuid Receipt,const FString& Dock)
{
 auto* Record=State.SerializedHulls.Find(Hull);
 if(!Record || Record->ReservedBy!=Receipt || !Receipt.IsValid() || Dock.IsEmpty() || Record->SoldReceipt.IsValid() || State.HullPurchaseReceipts.Contains(Receipt))return false;
 Record->ReservedBy.Invalidate();Record->SoldReceipt=Receipt;Record->Delivery=Dock;State.HullPurchaseReceipts.Add(Receipt,Hull);return true;
}
FGuid SerializedHullStockAI::Purchased(const FEconomyState& State,FGuid Receipt){return State.HullPurchaseReceipts.FindRef(Receipt);}
