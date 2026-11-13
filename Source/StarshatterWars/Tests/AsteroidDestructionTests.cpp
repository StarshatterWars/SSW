#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Asteroid.h"
#include "SimRegion.h"
#include "SimShot.h"
#include "WeaponDesign.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAsteroidDestructionTest, "Starshatter.Asteroids.Destruction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAsteroidDestructionTest::RunTest(const FString& Parameters)
{
    Asteroid::Initialize();
    SimRegion Region(nullptr, "Asteroid damage fixture", SimRegion::REAL_SPACE);
    auto* Rock = new Asteroid(5, FVector::ZeroVector, 5e7);
    auto* Survivor = new Asteroid(6, FVector(100000, 0, 0), 5e7);
    Region.InsertObject(Rock);
    Region.InsertObject(Survivor);
    if (!TestTrue(TEXT("Asteroid collision radius is available"), Rock->GetRadius() > 0)) return false;

    const double InitialIntegrity = Rock->GetIntegrity();
    WeaponDesign MiningLaser;
    MiningLaser.name = "Mining Laser";
    MiningLaser.primary = true;
    MiningLaser.beam = true;
    MiningLaser.life = 5;
    MiningLaser.length = 2500;
    MiningLaser.damage = InitialIntegrity * 2;
    auto* MiningShot = new SimShot(FVector::ZeroVector, Camera(), &MiningLaser);
    MiningShot->SetBeamPoints(FVector(-1000, 0, 0), FVector(1000, 0, 0));
    Region.InsertObject(MiningShot);
    Region.DamageShips();
    TestEqual(TEXT("Mining laser does not deal combat damage"), MiningShot->Damage(), 0.0);
    TestEqual(TEXT("Mining beam leaves asteroid integrity intact"), Rock->GetIntegrity(), InitialIntegrity);
    TestTrue(TEXT("Mining beam ends at the rock surface"), MiningShot->GetLocation().X < 0.0);
    MiningShot->SetLife(0);

    WeaponDesign Warhead;
    Warhead.primary = false;
    Warhead.guided = false;
    Warhead.life = 5;
    Warhead.damage = Rock->GetIntegrity() * 0.6;
    Region.InsertObject(new SimShot(Rock->GetLocation(), Camera(), &Warhead));
    Region.DamageShips();
    if (!TestEqual(TEXT("A nonlethal hit leaves both rocks"), Region.GetRoids().size(), 2)) return false;
    Region.InsertObject(new SimShot(Rock->GetLocation(), Camera(), &Warhead));
    Region.DamageShips();
    if (TestEqual(TEXT("Lethal hit removes only the destroyed rock"), Region.GetRoids().size(), 1))
        TestEqual(TEXT("Surviving contact keeps its identifier"), Region.GetRoids()[0]->GetContactId(), 6);
    return true;
}
#endif
