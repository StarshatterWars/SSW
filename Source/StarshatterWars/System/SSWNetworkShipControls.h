#pragma once
#include "CoreMinimal.h"
class Ship;
// Shared by arena and persistent-universe transports. SSW owns flight and weapon physics.
namespace SSWNetworkShipControls
{
    void Apply(Ship& Vessel,const FVector& Rotation,const FVector& Translation,float Throttle,bool Augmenter,bool Primary);
    void Clear(Ship& Vessel);
    void CarriedStores(Ship& Vessel,TArray<FName>& Out);
    void Target(Ship& Vessel,FName Action);
    void SensorCommand(Ship& Vessel,FName Action);
}
