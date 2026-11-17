#include "FittingMetrics.h"
#include "WeaponDesign.h"
#include "Ship.h"
#include "Weapon.h"
#include "WeaponGroup.h"
#include "Power.h"
#include "Drive.h"
#include "Shield.h"

FWeaponOutputMetrics FFittingMetrics::Weapon(const WeaponDesign& Design, int32 Barrels, double Quality, double Recharge, double RefireMultiplier)
{
    FWeaponOutputMetrics Result;
    if (Design.IsMiningLaser() || Barrels <= 0 || !FMath::IsFinite(Quality) || Quality <= 0 || Design.damage <= 0 || Design.charge <= 0) return Result;
    Barrels = FMath::Min(Barrels, 32);
    // Match Weapon::Fire/FireBarrel and SimShot::Damage: charge is not normalized to 1.
    const double Charge = Design.charge * Quality;
    const double DamagePerShot = Design.damage * Charge * (Design.primary && !Design.beam ? FMath::Max(0.0f, Design.life) : 1.0);
    const double Cycle = (Design.syncro ? Design.refire_delay : Barrels * Design.refire_delay + Design.salvo_delay) * RefireMultiplier;
    const double ShotsPerCycle = Barrels;
    Result.VolleyDamage = DamagePerShot * ShotsPerCycle * (Design.beam ? FMath::Max(0.0f, Design.life) * Quality : 1.0);
    if (!FMath::IsFinite(Cycle) || Cycle <= 0) return Result;
    Result.bRateKnown = true;
    const double Seconds = Design.beam ? FMath::Max(Cycle, double(Design.life) * Quality) : Cycle;
    Result.BurstDPS = Result.VolleyDamage / Seconds;
    Result.EnergyPerSecond = Design.charge * ShotsPerCycle / Seconds;
    const double Rate = Recharge < 0 ? FMath::Max(0.0f, Design.recharge_rate) : FMath::Max(0.0, Recharge);
    Result.SustainedDPS = Result.BurstDPS * FMath::Clamp(Rate / FMath::Max(UE_SMALL_NUMBER, Result.EnergyPerSecond), 0.0, 1.0);
    return Result;
}
FShipFittingMetrics FFittingMetrics::ShipFit(Ship& Craft)
{
    FShipFittingMetrics Result;
    auto& Reactors = Craft.GetReactors();
    for (int32 Index = 0; Index < Reactors.size(); ++Index)
        if (const auto* Reactor = Reactors[Index])
        { Result.ReactorOutput += Reactor->GetRatedOutput(); Result.AvailableOutput += Reactor->GetRatedOutput() * Reactor->GetAvailability() / 100; }
    auto& Systems = Craft.GetSystems();
    for (int32 Index = 0; Index < Systems.size(); ++Index)
        if (Systems[Index] && !dynamic_cast<PowerSource*>(Systems[Index])) Result.PowerDemand += Systems[Index]->GetRatedDemand();
    const double PowerFraction = Result.PowerDemand > 0 ? FMath::Clamp(Result.AvailableOutput / Result.PowerDemand, 0.0, 1.0) : 1;
    TSet<const class Weapon*> Seen;
    auto& Groups = Craft.GetWeapons();
    for (int32 Group = 0; Group < Groups.size(); ++Group)
    {
        if (!Groups[Group]) continue;
        auto& Weapons = Groups[Group]->GetWeapons();
        for (int32 Index = 0; Index < Weapons.size(); ++Index)
        {
            auto* Mount = Weapons[Index];
            if (!Mount || !Mount->Design() || Seen.Contains(Mount) || !Mount->Enabled() || Mount->Ammo() == 0) continue;
            Seen.Add(Mount);
            const auto Output = Weapon(*Mount->Design(), Mount->GetTurretBarrelCount(), Mount->GetOperatingAvailability(), Mount->GetRatedDemand() * PowerFraction, Mount->GetStatus()==SYSTEM_STATUS::NOMINAL ? 1 : 2);
            Result.BurstDPS += Output.BurstDPS; Result.SustainedDPS += Output.SustainedDPS;
        }
    }
    auto& Drives = Craft.GetDrives();
    for (int32 Index = 0; Index < Drives.size(); ++Index)
        if (Drives[Index]) Result.Thrust += Drives[Index]->MaxThrust() * Drives[Index]->GetOperatingAvailability();
    if (const auto* Shield = Craft.GetShield())
    {
        Result.ShieldCapacity = Shield->GetCapacity();
        if (Shield->GetShieldCapacitor() && Shield->GetRatedDemand() > 0 && PowerFraction > 0)
            Result.ShieldRechargeSeconds = Shield->GetCapacity() / (Shield->GetRatedDemand() * PowerFraction);
    }
    return Result;
}
FText FFittingMetrics::Describe(Ship& Craft)
{
    const auto Stats = ShipFit(Craft);
    return FText::Format(INVTEXT("BURST {0} DPS  |  SUSTAINED {1} DPS\nREACTOR {2} W  |  AVAILABLE {3} W  |  RATED DEMAND {4} W\nTHRUST {5}  |  SHIELD CAPACITY {6}  |  SHIELD RECHARGE {7}"),
        FText::AsNumber(Stats.BurstDPS), FText::AsNumber(Stats.SustainedDPS), FText::AsNumber(Stats.ReactorOutput),
        FText::AsNumber(Stats.AvailableOutput), FText::AsNumber(Stats.PowerDemand), FText::AsNumber(Stats.Thrust),
        FText::AsNumber(Stats.ShieldCapacity), Stats.ShieldRechargeSeconds ? FText::Format(INVTEXT("{0} s"), FText::AsNumber(*Stats.ShieldRechargeSeconds)) : INVTEXT("N/A"));
}
