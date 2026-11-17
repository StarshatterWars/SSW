#pragma once
#include "CoreMinimal.h"
class Ship;
class WeaponDesign;
struct FWeaponOutputMetrics
{
    bool bRateKnown = false;
    double VolleyDamage = 0;
    double BurstDPS = 0;
    double SustainedDPS = 0;
    double EnergyPerSecond = 0;
};
struct FShipFittingMetrics
{
    double ReactorOutput = 0;
    double AvailableOutput = 0;
    double PowerDemand = 0;
    double BurstDPS = 0;
    double SustainedDPS = 0;
    double Thrust = 0;
    double ShieldCapacity = 0;
    TOptional<double> ShieldRechargeSeconds;
};
class FFittingMetrics
{
public:
    static FWeaponOutputMetrics Weapon(const WeaponDesign&, int32 Barrels, double OperatingAvailability = 1, double RechargeWatts = -1, double RefireMultiplier = 1);
    static FShipFittingMetrics ShipFit(Ship&);
    static FText Describe(Ship&);
};
