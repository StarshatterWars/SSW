#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Weapon.h"
#include "SimShot.h"
#include "SimRegion.h"
#include "Graphic.h"

namespace
{
class FOwnedBeamShot : public SimShot
{
public:
    FOwnedBeamShot(WeaponDesign& Design, int32& Counter)
        : SimShot(FVector::ZeroVector, Camera(), &Design, nullptr), Destroyed(Counter) {}
    ~FOwnedBeamShot() override { ++Destroyed; }
private:
    int32& Destroyed;
};
class FBeamWeaponFixture : public Weapon
{
public:
    explicit FBeamWeaponFixture(WeaponDesign& Design) : Weapon(&Design, 1, nullptr) {}
    void Attach(SimShot* Shot) { beams[0] = Shot; Observe(Shot); }
    void Stop() { KillBeams(); }
};
class FBeamRegionFixture : public SimRegion
{
public:
    FBeamRegionFixture() : SimRegion(nullptr, "Beam ownership fixture", 0) {}
    void AdvanceShots() { UpdateShots(.1); }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeamOwnershipTest, "Starshatter.Weapons.BeamRegionOwnership",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBeamOwnershipTest::RunTest(const FString&)
{
    WeaponDesign Design;
    Design.beam = true;
    Design.primary = true;
    Design.life = 1;
    Design.graphic_type = Graphic::BOLT;
    int32 Destroyed = 0;
    FBeamRegionFixture Region;
    {
        FBeamWeaponFixture WeaponFixture(Design);
        auto* Shot = new FOwnedBeamShot(Design, Destroyed);
        Region.InsertObject(Shot);
        WeaponFixture.Attach(Shot);
        WeaponFixture.Stop();
        TestNull(TEXT("Weapon clears its borrowed beam immediately"), WeaponFixture.GetBeam(0));
        TestEqual(TEXT("Stopping a region-owned beam defers deletion"), Destroyed, 0);
        TestTrue(TEXT("Stopped beam is expired"), Shot->GetLife() <= 0);
        Region.AdvanceShots();
        TestEqual(TEXT("Region deletes the expired beam exactly once"), Destroyed, 1);
        TestEqual(TEXT("No freed pointer remains in region shots"), Region.GetShots().size(), 0);
    }
    {
        FBeamWeaponFixture WeaponFixture(Design);
        auto* Shot = new FOwnedBeamShot(Design, Destroyed);
        Region.InsertObject(Shot);
        WeaponFixture.Attach(Shot);
    }
    TestEqual(TEXT("Weapon destruction also leaves deletion to region"), Destroyed, 1);
    Region.AdvanceShots();
    TestEqual(TEXT("Region cleans beam after weapon destruction"), Destroyed, 2);
    {
        FBeamWeaponFixture WeaponFixture(Design);
        WeaponFixture.Attach(new FOwnedBeamShot(Design, Destroyed));
    }
    TestEqual(TEXT("Uninserted beam is still cleaned locally"), Destroyed, 3);
    return true;
}
#endif
