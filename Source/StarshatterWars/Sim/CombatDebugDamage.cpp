#include "CombatDebugDamage.h"
#include "Ship.h"
#include "SimEvent.h"

bool ApplyCombatDebugDamage(const Ship* Attacker, Ship* Victim, double Damage)
{
    if (!Attacker || !Victim || Attacker == Victim || !FMath::IsFinite(Damage) || Damage <= 0 ||
        Victim->IsDead() || Victim->IsDying() || Victim->InTransition() || Victim->GetIntegrity() < 1)
        return false;
    Victim->Physical::InflictDamage(Damage);
    if (Victim->GetIntegrity() >= 1) return true;
    if (auto* Stats = ShipStats::Find(Attacker->GetName()))
    {
        Stats->AddEvent(ESimEvent::GUNS_KILL, Victim->GetName());
        if (Attacker->GetIFF() != Victim->GetIFF() && (Victim->GetIFF() > 0 || Attacker->GetIFF() > 1))
            Stats->AddPoints(Victim->Value());
    }
    if (auto* Stats = ShipStats::Find(Victim->GetName()))
        Stats->AddEvent(ESimEvent::DESTROYED, Attacker->GetName());
    Victim->DeathSpiral();
    return true;
}
