#pragma once
#include "TurretActor.h"
namespace TurretCatalog {
TSubclassOf<ATurretActor> FindClass(FName WeaponType);
TSubclassOf<ATurretActor> VisualClass(FName WeaponType);
void ShipMounts(FName ShipDef, TArray<FShipTurretMountDef>& Out);
}
