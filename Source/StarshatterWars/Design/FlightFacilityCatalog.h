#pragma once
#include "CoreMinimal.h"
struct FShipFlightDeck;
// JSON flight-facility policy; authoring rows/tables remain unchanged. Game-thread only.
namespace FlightFacilityCatalog
{
    bool Reload(FString& Error);
    bool Contains(FName Design);
    bool IsDockingOnly(FName Design);
    TArray<FName> Names();
    // Unknown designs retain the native radians contract. False means docking-only.
    bool ResolveDeck(FName Design,int32 AuthoredIndex,FShipFlightDeck& InOut);
}
