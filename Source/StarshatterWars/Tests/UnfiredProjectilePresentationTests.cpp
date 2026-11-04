#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "ShipActor.h"
#include "ShipWeaponMountComponent.h"
#include "BoltWeaponActor.h"
#include "BeamWeaponActor.h"
#include "Engine/World.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnfiredProjectilePresentationTest,"Starshatter.Weapons.UnfiredProjectilesDoNotGlow",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FUnfiredProjectilePresentationTest::RunTest(const FString&)
{
 const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
 ON_SCOPE_EXIT{World->DestroyWorld(false);};
 auto* Bolt=World->SpawnActor<ABoltWeaponActor>();Bolt->DispatchBeginPlay();
 TestTrue(TEXT("Unfired game-world bolt is hidden"),Bolt->IsHidden());
 Bolt->Launch(FVector::ZeroVector,FVector::ForwardVector,8000,1.6f);
 TestFalse(TEXT("Fired bolt becomes visible"),Bolt->IsHidden());
 auto* SimulationBolt=World->SpawnActor<ABoltWeaponActor>();SimulationBolt->DispatchBeginPlay();SimulationBolt->BeginSimulationFlight();
 TestFalse(TEXT("Simulation-owned shot becomes visible"),SimulationBolt->IsHidden());
 auto* ShipVisual=World->SpawnActor<AShipActor>();
 for(UClass* ProjectileClass:{ABoltWeaponActor::StaticClass(),ABeamWeaponActor::StaticClass()})
 {
  auto* Mount=NewObject<UShipWeaponMountComponent>(ShipVisual);Mount->SetupAttachment(ShipVisual->GetRootComponent());Mount->RegisterComponent();
  Mount->SetRelativeLocation(FVector(120,30,5));const FVector MuzzleBefore=Mount->GetComponentLocation();
  Mount->SetChildActorClass(ProjectileClass);Mount->ConfigureTurretVisual(TEXT("Delta Cannon"));
  TestTrue(TEXT("Old mounted projectile choice is retained as spawn class"),Mount->ResolveProjectileClass()==ProjectileClass);
  TestTrue(TEXT("Projectile actor is removed from the mounted visual slot"),!Mount->GetChildActorClass() || (!Mount->GetChildActorClass()->IsChildOf(ABoltWeaponActor::StaticClass())&&!Mount->GetChildActorClass()->IsChildOf(ABeamWeaponActor::StaticClass())));
  TestTrue(TEXT("Migration preserves muzzle position"),Mount->GetComponentLocation().Equals(MuzzleBefore));
 }
 return true;
}
#endif
