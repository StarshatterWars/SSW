#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Components/StaticMeshComponent.h"
#include "DataLoader.h"
#include "Debris.h"
#include "DebrisActor.h"
#include "Engine/World.h"
#include "Ship.h"
#include "ShipDesignRegistry.h"
#include "ShipKiller.h"
#include "Sim.h"
#include "SimRegion.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNativeDebrisTest, "Starshatter.VisualPass.NativeDebris",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNativeDebrisTest::RunTest(const FString&)
{
    const bool bOwnLoader = !DataLoader::GetLoader();
    if (bOwnLoader) DataLoader::Initialize();
    ON_SCOPE_EXIT { if (bOwnLoader) DataLoader::Close(); };
    TUniquePtr<Sim> OwnedSim;
    if (!Sim::GetSim())
    {
        // The fixture needs no effects. Standalone editor tests run outside the game data directory.
        if (!FPaths::FileExists(TEXT("Explosions/Explosions.def")))
            AddExpectedError(TEXT("could notxparse 'Explosions.def'"), EAutomationExpectedErrorFlags::Contains, 1);
        OwnedSim = MakeUnique<Sim>(nullptr);
    }

    FShipDesign Row;
    Row.ShipName = TEXT("Broadsword");
    Row.ShipEmpire = EShipEmpire::Marakan;
    Row.Scale = 3.0;
    Row.Mass = 10000;
    Row.Integrity = 1000;
    FShipDeathSpiral& Spiral = Row.DeathSpiral.AddDefaulted_GetRef();
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

    SimRegion Region(nullptr, "Wreckage fixture", SimRegion::REAL_SPACE);
    Ship Unit("Wreckage fixture", "", Design, 2, 0, nullptr, false);
    Unit.SetRegion(&Region);
    Unit.MoveTo(FVector(100, 200, 300));
    Unit.SetVelocity(FVector(10, 20, 30));
    ShipKiller Killer(&Unit);
    Killer.ExecFrame(0.1);
    TestEqual(TEXT("Death spiral spawns the authored fragment count"), Region.GetRocks().size(), Entry.Count);
    if (Region.GetRocks().size() != Entry.Count) return false;
    const Debris* First = Region.GetRocks()[0];
    TestEqual(TEXT("Fragment lifetime comes from DEF"), First->GetLife(), double(Entry.Life));
    TestTrue(TEXT("First fragment retains authored offset"), First->GetLocation().Equals(FVector(100, 200, 500)));
    TestTrue(TEXT("First fragment inherits ship velocity and authored ejection speed"), First->GetVelocity().Equals(FVector(10, 20, 105)));
    TestEqual(TEXT("Source ship scale is retained"), First->GetVisualScale(), double(Row.Scale));
    TestTrue(TEXT("UE fragment has a simulation collision radius"), First->GetRadius() > 0);

    const auto Options = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Options);
    if (!TestNotNull(TEXT("Debris test world"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
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
