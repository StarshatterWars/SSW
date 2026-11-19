#pragma once
#include "GameStructs_economy.h"
#include "StationServiceCatalog.h"
class UGameInstance;
struct FSSWProductionFacility;
struct FStationMarketQuote
{
    FName Item, Buyer, Seller, Source, Destination;
    double Quantity = 0, Available = 0, InHold = 0, Maximum = 0, UnitPrice = 0;
    int64 TotalCredits = 0, Balance = 0;
    FString Unit;
    bool bBuy = true, bAllowed = false;
    FText Reason;
};
class FStationServices
{
public:
    static bool IsDocked();
    static bool Offers(ESSWStationService Service);
    static bool HasIndustry();
    static const FSSWProductionFacility* Facility();
    static FName Inventory(const TCHAR* Compartment);
    static bool MoveCargo(UGameInstance*, FName Item, bool ToStation, const TCHAR* Compartment, FText& Reason);
    static bool MoveCargo(UGameInstance*, FName Item, bool ToStation, const TCHAR* Compartment, double Quantity, FText& Reason);
    static double CargoTransferLimit(UGameInstance*, FName Item, bool ToStation, const TCHAR* Compartment, FText& Reason);
    // Read-only capacity/reservation calculation, also used by service regression checks.
    static double CargoTransferLimit(const FEconomyState&, FName Source, FName Destination, FName Item, double MissingDestinationCapacity, bool WholeUnits);
    static bool QuoteMarket(UGameInstance*, FName Item, double Quantity, bool Buy, FStationMarketQuote& Quote);
    static bool TradeMarket(UGameInstance*, FName Item, double Quantity, bool Buy, int64 ExpectedTotal, FGuid Receipt, FText& Reason);
    // Executes an already-authorized quote atomically in memory; failed persistence restores it.
    static bool CommitMarketTrade(FEconomyState&, const FStationMarketQuote&, FGuid Receipt, TFunctionRef<bool()> Persist, FText& Reason);
    static int64 FullRepairCost();
    static int64 RepairQuote(class Ship&);
    static bool RepairAll(UGameInstance*, FText& Reason);
};
