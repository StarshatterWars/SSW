#pragma once
#include "Ship.h"
// Player command access only; autonomous carrier AI continues to own its normal operations.
namespace FlightOpsAccessRules
{
 inline bool CanCommand(Ship* PlayerCraft,Ship* Carrier)
 {
  return PlayerCraft && Carrier && PlayerCraft==Carrier && !PlayerCraft->IsDead() && !PlayerCraft->IsDying() &&
      PlayerCraft->GetClassification()==CLASSIFICATION::CARRIER && PlayerCraft->GetHangar();
 }
}
