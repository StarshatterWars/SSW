#include "ShipDesignRegistry.h"

TMap<FName, FShipDesign> ShipDesignRegistry::DesignsByName;

//-------------------------------------------------------------
// Lifecycle
//-------------------------------------------------------------

void ShipDesignRegistry::Clear()
{
    DesignsByName.Empty();
}

//-------------------------------------------------------------
// Registration
//-------------------------------------------------------------

void ShipDesignRegistry::RegisterDesign(const FName& RowName, const FShipDesign& Row)
{
    if (!RowName.IsNone())
    {
        DesignsByName.Add(RowName, Row);
    }
}

void ShipDesignRegistry::RegisterDesign(const FString& Name, const FShipDesign& Row)
{
    if (!Name.IsEmpty())
    {
        DesignsByName.Add(FName(*Name), Row);
    }
}

//-------------------------------------------------------------
// Lookup
//-------------------------------------------------------------

const FShipDesign* ShipDesignRegistry::Find(const FName& Name)
{
    return Name.IsNone() ? nullptr : DesignsByName.Find(Name);
}

const FShipDesign* ShipDesignRegistry::Find(const FString& Name)
{
    return Name.IsEmpty() ? nullptr : DesignsByName.Find(FName(*Name));
}

const FShipDesign* ShipDesignRegistry::Find(const char* Name)
{
    if (!Name || !Name[0])
    {
        return nullptr;
    }

    return DesignsByName.Find(FName(ANSI_TO_TCHAR(Name)));
}

//-------------------------------------------------------------
// Utility
//-------------------------------------------------------------

bool ShipDesignRegistry::Has(const FName& Name)
{
    return !Name.IsNone() && DesignsByName.Contains(Name);
}

bool ShipDesignRegistry::Has(const FString& Name)
{
    return !Name.IsEmpty() && DesignsByName.Contains(FName(*Name));
}

bool ShipDesignRegistry::Has(const char* Name)
{
    return Name && Name[0] && DesignsByName.Contains(FName(ANSI_TO_TCHAR(Name)));
}

int32 ShipDesignRegistry::Num()
{
    return DesignsByName.Num();
}

const TMap<FName, FShipDesign>& ShipDesignRegistry::GetAll()
{
    return DesignsByName;
}