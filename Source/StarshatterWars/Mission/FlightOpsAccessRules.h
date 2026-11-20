#pragma once
#include "Ship.h"
// Player command access only; autonomous carrier AI continues to own its normal operations.
namespace FlightOpsAccessRules
{
 inline bool CanCommand(Ship* Pilot,Ship* Carrier)
 {
  return Pilot && Carrier && Pilot==Carrier && !Pilot->IsDead() && !Pilot->IsDying() &&
      Pilot->GetClassification()==CLASSIFICATION::CARRIER && Pilot->GetHangar();
 }
}
