#pragma once
#include "GameStructs_economy.h"
class UCargoCatalogSubsystem;
class FEconomyRuntime
{
public:
    static void InitializeMarkets(FEconomyState&, const UCargoCatalogSubsystem&);
    static FName Warehouse(const FString& System, const FString& Station, const TCHAR* Compartment);
    static void EnsureWarehouse(FEconomyState&, FName Key, FName System, double Capacity, bool bMarket = false);
    static FName IndustryOwner() { return FName(TEXT("Civilian industry")); }
};
