#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "SimElement.h"
#include "SimRegion.h"
#include "Instruction.h"

namespace {
class FRecoveryWaypointShip : public Ship {
public:
    explicit FRecoveryWaypointShip(ShipDesign* Design) : Ship("Recovery waypoint fixture", "", Design, 1, 0, nullptr, false) { radius = 100; }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryWaypointTest, "Starshatter.Carrier.Recovery.ProximityKeepsRecoveryOrder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRecoveryWaypointTest::RunTest(const FString&)
{
    for (EInstruction Action : {EInstruction::RTB, EInstruction::Dock, EInstruction::Vector}) {
        ShipDesign Design;
        SimElement Element("Recovery waypoint fixture", 1);
        SimRegion Region(nullptr, "Recovery waypoint fixture", SimRegion::REAL_SPACE);
        auto* Unit = new FRecoveryWaypointShip(&Design);
        Region.InsertObject(Unit);
        Element.AddShip(Unit);
        Unit->SetElement(&Element);
        auto* Route = new Instruction(&Region, FVector::ZeroVector, Action);
        Element.AddNavPoint(Route, nullptr, false);
        Unit->MoveTo(FVector::ZeroVector);
        Unit->ExecNavFrame(0.1);
        if (Action == EInstruction::Vector)
            TestEqual(TEXT("An ordinary travel leg still completes at the waypoint"), Route->GetStatus(), INSTRUCTION_STATUS::COMPLETE);
        else {
            TestTrue(TEXT("Proximity cannot complete a recovery order"), Route->GetStatus() <= INSTRUCTION_STATUS::ACTIVE);
            TestEqual(TEXT("Recovery order remains available to the landing AI"), Unit->GetNextNavPoint(), Route);
        }
    }
    return true;
}
#endif
