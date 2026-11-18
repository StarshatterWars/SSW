#pragma once
#include "GameStructs_insurance.h"
#include "GameStructs.h"
class Ship;
class UGameInstance;

class FOwnedHullCondition
{
public:
    static bool IsOwnedLive(const FPlayerHangarShip&, FGuid AssignedHull, FName LiveDesign, FName LiveCargo, bool FreeFlight);
    static bool CaptureActive(FS_PlayerGameInfo&);
    // Raw snapshots also support exact repair rollback. They never change component grades.
    static void Capture(Ship&, FPlayerHangarShip&);
    static void Apply(Ship&, const FPlayerHangarShip&);
    static void ApplyPristine(Ship&);
    static bool RepairAndSave(UGameInstance*, Ship&, int64 TotalCost, FText& Reason);
    static bool RepairTransaction(Ship&, FS_PlayerGameInfo&, FPlayerHangarShip* Owned,
        TFunctionRef<bool()> Pay, TFunctionRef<bool()> Repair, TFunctionRef<bool()> Persist);
};
