#pragma once
#include "GameStructs_economy.h"
class UGameInstance;
struct FSSWProductionFacility;
class FStationServices
{
public:
    static bool IsDocked();
    static bool HasIndustry();
    static const FSSWProductionFacility* Facility();
    static FName Inventory(const TCHAR* Compartment);
    static bool MoveCargo(UGameInstance*, FName Item, bool ToStation, const TCHAR* Compartment, FText& Reason);
    static int64 FullRepairCost();
    static int64 RepairQuote(class Ship&);
    static bool RepairAll(UGameInstance*, FText& Reason);
};
