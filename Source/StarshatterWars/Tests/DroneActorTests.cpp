#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "DroneActor.h"
#include "FighterActor.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDroneActorDefaultsTest, "Starshatter.Drone.NativeDefaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDroneActorDefaultsTest::RunTest(const FString& Parameters)
{
    const ADroneActor* Drone = GetDefault<ADroneActor>();
    TestTrue(TEXT("Drone supports existing ship presentation"), Drone->IsA<AShipActor>());
    TestFalse(TEXT("Drone has no fighter gear/cockpit hierarchy"), Drone->IsA<AFighterActor>());
    TestEqual(TEXT("DEF authoring scale"), Drone->ShipRoot->GetRelativeScale3D(), FVector(0.08f));
    TestEqual(TEXT("DEF bridge conversion"), Drone->BridgePointOffset, FVector(8,0,145));
    TestEqual(TEXT("Unarmed reference"), Drone->WeaponMountPointDefs.Num(), 0);
    TestNotNull(TEXT("Drone exhaust asset is assigned"), Drone->MainEngineEmitterSystem.Get());
    TestEqual(TEXT("No generic running lights"), Drone->NavLightDefs.Num(), 0);
    TestEqual(TEXT("One DEF drive port"), Drone->MainEnginePointDefs.Num(), 1);
    if (Drone->MainEnginePointDefs.Num() == 1)
        TestEqual(TEXT("Drive port converted once"), Drone->MainEnginePointDefs[0].LocalOffset, FVector(-292,0,-58));
    const UWorld::InitializationValues Options = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false, NAME_None, nullptr, true,
        ERHIFeatureLevel::Num, &Options);
    if (TestNotNull(TEXT("Drone preview world"), World))
    {
        ADroneActor* Spawned = World->SpawnActor<ADroneActor>();
        if (TestNotNull(TEXT("Drone actor spawns"), Spawned))
        {
            TestEqual(TEXT("Spawned DEF scale"), Spawned->GetActorScale3D(), FVector(0.08f));
            Spawned->SetActorScale3D(FVector(0.2));
            Spawned->RerunConstructionScripts();
            TestEqual(TEXT("Reconstruction preserves edited scale"), Spawned->GetActorScale3D(), FVector(0.2));
        }
        World->DestroyWorld(false);
    }
    return true;
}
#endif
