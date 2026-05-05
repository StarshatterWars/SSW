/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.

    ORIGINAL AUTHOR AND STUDIO: Carlos Bott / Fractal Dev Studios

    SUBSYSTEM:    Stars.exe
    FILE:         ShipDesignRegistry.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    ShipDesignRegistry

    Pure C++ global registry for ship designs.

    - No UObject dependency
    - Populated by UStarshatterShipDesignSubsystem
    - Read-only access for legacy/runtime systems
    - Converts FShipDesign DataTable rows into legacy ShipDesign objects
*/

#pragma once

#include "CoreMinimal.h"
#include "ShipDesign.h"
#include "MissionElement.h"
#include "GameStructs_System.h"

class STARSHATTERWARS_API ShipDesignRegistry
{
public:

    //-------------------------------------------------------------
    // Lifecycle
    //-------------------------------------------------------------
    static void Clear();

    //-------------------------------------------------------------
    // Registration
    //-------------------------------------------------------------
    static void RegisterDesign(const FName& RowName, const FShipDesign& Row);
    static void RegisterDesign(const FString& Name, const FShipDesign& Row);

    //-------------------------------------------------------------
    // Unreal row lookup
    //-------------------------------------------------------------
    static const FShipDesign* Find(const FName& Name);
    static const FShipDesign* Find(const FString& Name);
    static const FShipDesign* Find(const char* Name);

    //-------------------------------------------------------------
    // Legacy runtime lookup
    //-------------------------------------------------------------
    static ShipDesign* FindLegacy(const FName& Name);
    static ShipDesign* FindLegacy(const FString& Name);
    static ShipDesign* FindLegacy(const char* Name);

    //-------------------------------------------------------------
    // Utility
    //-------------------------------------------------------------
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

    static ShipDesign* ConvertToLegacyDesign(const FName& RowName, const FShipDesign& Row);
    static void CopyStringToAnsi(char* Dest, int32 DestSize, const FString& Source);
    static void ResetShipComponents(ShipDesign* Legacy);

private:

    static TMap<FName, FShipDesign> DesignsByName;
    static TMap<FName, ShipDesign*> LegacyDesignsByName;
};