#pragma once
#include "CoreMinimal.h"
#include "StationServiceCatalog.h"

struct FStationServiceDescriptor
{
    ESSWStationService Service;
    const TCHAR* Label;
    const TCHAR* RegistryKey;
    const TCHAR* Explanation;
};
struct FStationDockContext
{
    bool bDocked=false;
    bool bFactionAllowed=true;
    FName Design,Facility,ActualDock;
    FString Name,System,Region;
};
class STARSHATTERWARS_API StationServicesAI
{
public:
    static bool FactionAllows(const class Ship* Craft,const class Ship* Base);
    static FStationDockContext Context(FName Viewed=NAME_None);
    static bool CanRepair(class UGameInstance*,FText& Reason,FName Viewed=NAME_None);
    static bool ExecuteRepair(class UGameInstance*,FText& Reason,FName Viewed=NAME_None);
    static TConstArrayView<FStationServiceDescriptor> Descriptors();
    static bool CanUse(const FStationDockContext&,ESSWStationService,FText& Reason);
};
