#pragma once

#include "CoreMinimal.h"
#include "ShipDesign.h"
#include "MissionElement.h"
#include "GameStructs_System.h"

/*
    ShipDesignRegistry

    Pure C++ global registry for ship designs.

    - No UObject dependency
    - Populated by UStarshatterShipDesignSubsystem
    - Read-only access for legacy/runtime systems
*/

class STARSHATTERWARS_API ShipDesignRegistry
{
public:

    // Lifecycle
    static void Clear();

    // Registration (called by subsystem)
    static void RegisterDesign(const FName& RowName, const FShipDesign& Row);
    static void RegisterDesign(const FString& Name, const FShipDesign& Row);

    // Lookup
    static const FShipDesign* Find(const FName& Name);
    static const FShipDesign* Find(const FString& Name);
    static const FShipDesign* Find(const char* Name);

    // Utility
    static bool Has(const FName& Name);
    static bool Has(const FString& Name);
    static bool Has(const char* Name);

    static int32 Num();

    static const TMap<FName, FShipDesign>& GetAll();

    static const FShipDesign* ResolveDesignRow(const MissionElement* Elem)
    {
        return Elem ? Elem->GetShipDesign() : nullptr;
    }

private:
    static TMap<FName, FShipDesign> DesignsByName;
};