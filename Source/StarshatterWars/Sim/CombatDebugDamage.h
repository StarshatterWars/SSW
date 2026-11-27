#pragma once
class Ship;
// Development damage uses ordinary mission statistics, with a single lethal edge.
bool ApplyCombatDebugDamage(const Ship* Attacker, Ship* Victim, double Damage);
