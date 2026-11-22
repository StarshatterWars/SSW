#pragma once
#include "CoreMinimal.h"
#include "GameStructs.h"
class Ship;
class Mission;
namespace OwnedHullLocation
{
    FString Carrier(const FString& Name);
    FString Station(const FString& System, const FString& Name);
    FString Dock(const Ship& Craft);
    FString Current(const Ship& Craft);
    bool IsAt(const FPlayerHangarShip& Hull, const FString& Dock);
    bool RecoverPurchase(const FS_PlayerGameInfo& Profile, FPlayerHangarShip& Hull);
    TArray<FGuid> StoredAt(const FS_PlayerGameInfo& Profile, const FString& Dock);
    bool CanProvision(const FPlayerHangarShip& Hull, Mission& Entry, bool Active);
}
