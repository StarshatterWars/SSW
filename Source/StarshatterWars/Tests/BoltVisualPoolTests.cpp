#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "ScopedTestWorldContext.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "BoltVisualPoolSubsystem.h"
#include "BoltWeaponActor.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBoltVisualPoolTest,"Starshatter.Weapons.BoltPoolLifecycle",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBoltVisualPoolTest::RunTest(const FString&)
{
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    FScopedSSWTestWorldContext Context(World);if(!TestNotNull(TEXT("Pool world"),World))return false;
    ON_SCOPE_EXIT{World->DestroyWorld(false);};
    auto* Pool=World->GetSubsystem<UBoltVisualPoolSubsystem>();if(!TestNotNull(TEXT("World owns presentation pool"),Pool))return false;
    auto* First=Pool->Acquire(ABoltWeaponActor::StaticClass(),FVector(1,2,3),FRotator::ZeroRotator);
    if(!TestNotNull(TEXT("First bolt allocated"),First))return false;
    First->BeginSimulationFlight(false);Pool->Release(First);Pool->Release(First);
    TestEqual(TEXT("Double release cannot duplicate idle entries"),Pool->GetIdleCount(),1);
    TestTrue(TEXT("Dormant bolt is hidden and noncolliding"),First->IsHidden() && !First->GetActorEnableCollision());
    auto* Second=Pool->Acquire(ABoltWeaponActor::StaticClass(),FVector(4,5,6),FRotator(0,90,0));
    TestEqual(TEXT("Next shot reuses the same actor"),Second,First);
    TestTrue(TEXT("Reused bolt has the new muzzle position"),Second->GetActorLocation().Equals(FVector(4,5,6)));
    TestEqual(TEXT("Reuse removes idle ownership"),Pool->GetIdleCount(),0);
    TestEqual(TEXT("Only one spawn required"),Pool->GetSpawnCount(),uint64(1));
    TestEqual(TEXT("Reuse recorded"),Pool->GetReuseCount(),uint64(1));
    Second->BeginSimulationFlight(false);TestFalse(TEXT("Activation restores visibility"),Second->IsHidden());
    Pool->Release(Second);return true;
}
#endif
