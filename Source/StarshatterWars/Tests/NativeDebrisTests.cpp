#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Components/StaticMeshComponent.h"
#include "DataLoader.h"
#include "Debris.h"
#include "DebrisActor.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Game.h"
#include "SimShot.h"
#include "WeaponDesign.h"
#include "MissionCameraRig.h"
#include "ShipActor.h"
#include "Ship.h"
#include "ShipDesignRegistry.h"
#include "ShipKiller.h"
#include "Explosion.h"
#include "Sim.h"
#include "SimRegion.h"

namespace
{
class FDeathSpiralSim : public Sim
{
public:
    FDeathSpiralSim() : Sim(nullptr) {}
    void SetFixtureRegion(SimRegion* Region) { active_region = Region; }
};
class FCombatHit : public SimShot
{
public:
    FCombatHit(WeaponDesign* Design, const FVector& Start, const FVector& End)
        : SimShot(Start, Camera(), Design)
    {
        armed = true;
        origin = Start;
        MoveTo(End);
    }
};
class FDeathSpiralRegion : public SimRegion
{
public:
    FDeathSpiralRegion() : SimRegion(nullptr, "Wreckage fixture", SimRegion::REAL_SPACE) {}
    using SimRegion::UpdateShips;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNativeDebrisTest, "Starshatter.VisualPass.NativeDebris",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNativeDebrisTest::RunTest(const FString&)
{
    const bool bOwnLoader = !DataLoader::GetLoader();
    if (bOwnLoader) DataLoader::Initialize();
    ON_SCOPE_EXIT { if (bOwnLoader) DataLoader::Close(); };
    TUniquePtr<FDeathSpiralSim> OwnedSim;
    if (!Sim::GetSim())
    {
        // The fixture needs no effects. Standalone editor tests run outside the game data directory.
        if (!FPaths::FileExists(TEXT("Explosions/Explosions.def")))
            AddExpectedError(TEXT("could notxparse 'Explosions.def'"), EAutomationExpectedErrorFlags::Contains, 1);
        OwnedSim = MakeUnique<FDeathSpiralSim>();
    }

    FShipDesign Row;
    Row.ShipName = TEXT("Broadsword");
    Row.ShipEmpire = EShipEmpire::Marakan;
    Row.Scale = 3.0;
    Row.Mass = 10000;
    Row.Integrity = 1000;
    FShipDeathSpiral& Spiral = Row.DeathSpiral.AddDefaulted_GetRef();
    Spiral.Time = 1.0f;
    FExplosion& Final = Spiral.Explosions.AddDefaulted_GetRef();
    Final.Type = EExplosionType::LARGE_EXPLOSION;
    Final.bFinal = true;
    FDebris& Entry = Spiral.Debris.AddDefaulted_GetRef();
    Entry.ModelName = TEXT("BroadswordDebris.mag");
    Entry.Count = 2;
    Entry.Location = FVector(0, 0, 200);
    Entry.Mass = 28000;
    Entry.Speed = 75;
    Entry.Drag = 0.002f;
    Entry.Life = 380;
    const FName Key(TEXT("WreckageFixture"));
    const auto Saved = ShipDesignRegistry::GetAll();
    ON_SCOPE_EXIT {
        ShipDesignRegistry::Clear();
        for (const auto& Pair : Saved) ShipDesignRegistry::RegisterDesign(Pair.Key, Pair.Value);
    };
    ShipDesignRegistry::RegisterDesign(Key, Row);
    ShipDesign* Design = ShipDesignRegistry::FindLegacy(Key);
    if (!TestNotNull(TEXT("Converted ship design"), Design)) return false;
    const ShipDebris& Fragment = Design->debris[0];
    TestEqual(TEXT("Legacy extension is removed from asset key"), Fragment.VisualModel, FName(TEXT("/Game/Models/Broadsword/Debris/BroadswordDebris/BP_BroadswordDebris")));
    TestEqual(TEXT("Authored drag survives ingestion"), Fragment.GetDrag(), Entry.Drag);
    const auto VisualClass = ADebrisActor::ResolveVisualClass(Fragment.VisualModel);
    if (!TestNotNull(TEXT("Debris Blueprint resolves"), VisualClass.Get())) return false;

    FDeathSpiralRegion Region;
    if (OwnedSim) OwnedSim->SetFixtureRegion(&Region);
    ON_SCOPE_EXIT { if (OwnedSim) OwnedSim->SetFixtureRegion(nullptr); };
    auto* Unit = new Ship("Wreckage fixture", "", Design, 100, 0, nullptr, false);
    Region.InsertObject(Unit);
    Unit->SetRegion(&Region);
    Unit->SetFlightPhase(EOPSMode::ACTIVE);
    Unit->MoveTo(FVector(100, 200, 300));
    Unit->SetVelocity(FVector(10, 20, 30));
    TestNull(TEXT("The fixture uses an Unreal-only hull"), Unit->GetRep());
    const auto Options = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Options);
    if (!TestNotNull(TEXT("Debris test world"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    auto* Visual = World->SpawnActor<AShipActor>();
    auto* View = World->SpawnActor<ACameraActor>();
    auto* PC = World->SpawnActor<APlayerController>();
    FMissionCameraRig CameraRig;
    double Zoom = 1.0;
    Visual->SetActorLocation(FVector(300, 100, 200));
    Visual->HullMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    Visual->BindRuntimeShip(Unit);
    ON_SCOPE_EXIT { Visual->BindRuntimeShip(nullptr); };
    WeaponDesign AttackingWeapon;
    AttackingWeapon.primary = true;
    AttackingWeapon.damage = 100000;
    AttackingWeapon.charge = 1;
    AttackingWeapon.life = 1;
    AttackingWeapon.target_type = 0x000fffff;
    auto* Shot = new FCombatHit(&AttackingWeapon, FVector(100, 200, 100), FVector(100, 200, 300));
    Shot->SetCharge(1);
    Region.InsertObject(Shot);
    const bool bWasPaused = Game::Paused();
    Game::SetPausedState(false);
    Region.DamageShips();
    Game::SetPausedState(bWasPaused);
    if (!TestTrue(TEXT("Projectile collision begins death spiral"), Unit->IsDying())) return false;
    CameraRig.Tick(PC, View, Visual, Unit, &Region, nullptr, 0.016, Zoom);
    const FVector DeathCameraLocation = View->GetActorLocation();
    TestFalse(TEXT("Death camera moves out of the previous eye position"), DeathCameraLocation.IsNearlyZero());
    Region.UpdateShips(0.25);
    CameraRig.Tick(PC, View, Visual, Unit, &Region, nullptr, 0.016, Zoom);
    TestTrue(TEXT("Death camera holds its authored position through the spiral"), View->GetActorLocation().Equals(DeathCameraLocation, 0.01));
    TestTrue(TEXT("Region retains the ship during the spiral"), Region.GetShips().contains(Unit) && Unit->IsDying());
    TestEqual(TEXT("No fragments before breakup"), Region.GetRocks().size(), 0);
    const auto& Cam = Unit->GetCam();
    const Matrix& Orientation = Cam.Orientation();
    const FVector Forward(Orientation(0, 2), Orientation(1, 2), Orientation(2, 2));
    const FVector ExpectedLocation = Unit->GetLocation() + Forward * 200;
    const FVector ExpectedVelocity = Unit->GetVelocity() + Forward * 75;
    Region.UpdateShips(1.0);
    TestTrue(TEXT("Region removes the ship at breakup"), Unit->IsDead() && !Region.GetShips().contains(Unit));
    if (OwnedSim) TestTrue(TEXT("Final explosion enters the active region"), Region.GetExplosions().size() > 0);
    TestEqual(TEXT("Death spiral spawns the authored fragment count"), Region.GetRocks().size(), Entry.Count);
    if (Region.GetRocks().size() != Entry.Count) return false;
    const Debris* First = Region.GetRocks()[0];
    TestEqual(TEXT("Fragment lifetime comes from DEF"), First->GetLife(), double(Entry.Life));
    TestTrue(TEXT("First fragment retains authored offset"), First->GetLocation().Equals(ExpectedLocation, 0.01));
    TestTrue(TEXT("First fragment inherits ship velocity and authored ejection speed"), First->GetVelocity().Equals(ExpectedVelocity, 0.01));
    TestEqual(TEXT("Source ship scale is retained"), First->GetVisualScale(), double(Row.Scale));
    TestTrue(TEXT("UE fragment has a simulation collision radius"), First->GetRadius() > 0);

    auto* Actor = World->SpawnActor<ADebrisActor>(VisualClass);
    if (!TestNotNull(TEXT("Debris actor spawns"), Actor)) return false;
    Actor->UpdateFromSimulation(*First, FVector(500, 100, 200));
    TestTrue(TEXT("Actor follows mapped simulation location"), Actor->GetActorLocation().Equals(FVector(500, 100, 200)));
    TestTrue(TEXT("Actor follows source ship scale"), Actor->GetActorScale3D().Equals(FVector(Row.Scale)));
    TestFalse(TEXT("Unreal visual does not add a second collider"), Actor->GetActorEnableCollision());

    for (const EShipEmpire Empire : {EShipEmpire::Marakan, EShipEmpire::Terellian})
    {
        FShipDesign Generic = Row;
        Generic.ShipEmpire = Empire;
        Generic.DeathSpiral[0].Debris[0].ModelName = TEXT("Debris0A");
        const bool bMarakan = Empire == EShipEmpire::Marakan;
        const FName GenericKey(bMarakan ? TEXT("MarakanDebrisFixture") : TEXT("TerellianDebrisFixture"));
        ShipDesignRegistry::RegisterDesign(GenericKey, Generic);
        const ShipDesign* GenericDesign = ShipDesignRegistry::FindLegacy(GenericKey);
        if (!TestNotNull(TEXT("Faction debris design"), GenericDesign)) continue;
        const FString Expected = FString(TEXT("/Game/Models/Debris/")) +
            (bMarakan ? TEXT("Marakan") : TEXT("Terellian")) + TEXT("/Debris0A/BP_Debris0A");
        TestEqual(TEXT("Shared fragment resolves to the owning faction"), GenericDesign->debris[0].VisualModel, FName(*Expected));
        TestNotNull(TEXT("Faction fragment Blueprint loads"), ADebrisActor::ResolveVisualClass(FName(*Expected)).Get());
    }
    TestNotNull(TEXT("Orion-specific debris Blueprint loads"), ADebrisActor::ResolveVisualClass(
        FName(TEXT("/Game/Models/Orion/Debris/OrionDebris1/BP_OrionDebris1"))).Get());
    return true;
}

#endif
