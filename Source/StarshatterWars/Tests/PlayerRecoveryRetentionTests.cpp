#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "SimRegion.h"
#include "Ship.h"
#include "ShipDesign.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerRecoveryRetentionTest,"Starshatter.Carrier.Recovery.RetainPlayerUntilDebrief",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerRecoveryRetentionTest::RunTest(const FString& Parameters)
{
    ShipDesign Design;
    SimRegion Region(nullptr,"Recovery retention fixture",SimRegion::REAL_SPACE);
    auto* Player = new Ship("Recovered player","",&Design,1,0,nullptr,false);
    Region.GetShips().append(Player);
    Region.SetPlayerShip(Player);
    Player->SetFlightPhase(EOPSMode::DOCKED);
    Region.DockShips();
    TestTrue(TEXT("Recovered player remains in region until presentation closes"),Region.GetShips().contains(Player));
    TestEqual(TEXT("Player reference survives recovery"),Region.GetPlayerShip(),Player);
    TestFalse(TEXT("Parked fighter is not destroyed"),Player->IsDead());
    TestEqual(TEXT("Parked fighter stays docked"),Player->GetFlightPhase(),EOPSMode::DOCKED);
    Region.DockShips();
    TestEqual(TEXT("Repeated recovery processing retains one player"),Region.GetNumShips(),1);
    TestTrue(TEXT("Repeated processing preserves parked fighter"),Region.GetShips().contains(Player));
    auto* Wingman = new Ship("Recovered wingman","",&Design,1,0,nullptr,false);
    Region.GetShips().append(Wingman);
    Wingman->SetFlightPhase(EOPSMode::DOCKED);
    Region.DockShips();
    TestFalse(TEXT("NPC recovery retains legacy immediate removal"),Region.GetShips().contains(Wingman));
    TestTrue(TEXT("NPC recovery does not remove parked player"),Region.GetShips().contains(Player));
    return true; // Region owns the fighter and performs normal teardown.
}
#endif
