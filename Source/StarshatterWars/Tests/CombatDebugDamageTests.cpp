#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "CombatDebugDamage.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "SimEvent.h"
#include "Misc/ScopeExit.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDebugDamageTest,"Starshatter.Combat.DebugKillProgression",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FCombatDebugDamageTest::RunTest(const FString&)
{
    ShipStats::Initialize(); ON_SCOPE_EXIT { ShipStats::Initialize(); };
    ShipDesign Design; Design.integrity=100; Design.mass=100;
    Ship Attacker("Debug attacker","",&Design,1,0,nullptr,false);
    Ship Victim("Debug victim","",&Design,2,0,nullptr,false);
    TestFalse(TEXT("No attacker cannot produce a credited kill"),ApplyCombatDebugDamage(nullptr,&Victim,50));
    TestTrue(TEXT("First hit applies damage"),ApplyCombatDebugDamage(&Attacker,&Victim,50));
    auto* Stats=ShipStats::Find(Attacker.GetName()); Stats->Summarize();
    TestEqual(TEXT("Nonlethal damage is not a kill"),Stats->GetGunKills(),0);
    TestTrue(TEXT("Lethal edge applies"),ApplyCombatDebugDamage(&Attacker,&Victim,100));
    Stats->Summarize(); TestEqual(TEXT("Debug kill counts as one gun kill"),Stats->GetGunKills(),1);
    TestFalse(TEXT("Repeated damage does not duplicate the kill"),ApplyCombatDebugDamage(&Attacker,&Victim,100));
    Stats->Summarize(); TestEqual(TEXT("Kill remains exactly once"),Stats->GetGunKills(),1);
    return true;
}
#endif
