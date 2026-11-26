#pragma once
#include "GameStructs_economy.h"

namespace SerializedHullStockAI
{
 bool Ensure(FEconomyState&,FName Inventory,FName Design,int32 PhysicalCount,const TArray<FGuid>* Preferred=nullptr);
 TArray<FGuid> Available(const FEconomyState&,FName Inventory,FName Design,int32 AvailableCount);
 bool Reserve(FEconomyState&,FGuid Hull,FGuid Receipt,FName Inventory,FName Design);
 bool Deliver(FEconomyState&,FGuid Hull,FGuid Receipt,const FString& Dock);
 FGuid Purchased(const FEconomyState&,FGuid Receipt);
}
