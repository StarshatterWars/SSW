#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "ScopedTestWorldContext.h"
#include "SystemSceneBuilder.h"
#include "StarSystem.h"
#include "OrbitalBody.h"
#include "OrbitalRegion.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "RuntimeShipVisualLifetime.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTravelSceneStartupTest,"Starshatter.Travel.ColdDestinationScene",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTravelSceneStartupTest::RunTest(const FString&)
{
 StarSystem Solus("Solus",FVector::ZeroVector);
 auto* Sun=new OrbitalBody(&Solus,"Sol",Orbital::STAR,2e30,696340000,0);Solus.AddBody(Sun);
 auto* Planet=new OrbitalBody(&Solus,"Meridian",Orbital::PLANET,6e24,6400000,1.5e11,Sun);Sun->AddSatellite(Planet);
 Solus.AddRegion(new OrbitalRegion(&Solus,"Meridian",0,1000000,6700000,Planet));
 const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
 FScopedSSWTestWorldContext Context(World);ON_SCOPE_EXIT{World->DestroyWorld(false);};
 auto* Class=LoadClass<ASystemSceneBuilder>(nullptr,TEXT("/Game/Actors/BP_SystemSceneBuilder.BP_SystemSceneBuilder_C"));
 if(!TestNotNull(TEXT("Actual destination builder Blueprint exists"),Class))return false;
 auto* Builder=World->SpawnActor<ASystemSceneBuilder>(Class);
 if(!TestNotNull(TEXT("Destination builder spawns without authored system names"),Builder))return false;
 TestTrue(TEXT("Cold destination starts without generated bodies"),Builder->GetSpawnedBodies().IsEmpty());
 Builder->EnsureMissionSystem(&Solus);
 TestTrue(TEXT("Arrived simulation initializes the cold destination scene"),!Builder->GetSpawnedBodies().IsEmpty());
 const int Count=Builder->GetSpawnedBodies().Num();
 AActor* First=Count?Builder->GetSpawnedBodies()[0].Actor.Get():nullptr;
 Builder->EnsureMissionSystem(&Solus);
 TestEqual(TEXT("Repeated arrival checks do not rebuild the destination"),Builder->GetSpawnedBodies().Num(),Count);
 if(Count)TestTrue(TEXT("Live destination actors remain stable"),Builder->GetSpawnedBodies()[0].Actor==First);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTravelShipLifetimeTest,"Starshatter.Travel.ShipsSurviveSystemSceneryUnload",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTravelShipLifetimeTest::RunTest(const FString&)
{
 const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
 FScopedSSWTestWorldContext Context(World);ON_SCOPE_EXIT{World->DestroyWorld(false);};
 auto* OldLevel=NewObject<ULevel>(World,NAME_None,RF_Transient);
 OldLevel->OwningWorld=World;World->AddLevel(OldLevel);
 FActorSpawnParameters Scenery;Scenery.OverrideLevel=OldLevel;
 auto* Builder=World->SpawnActor<AActor>(Scenery);
 FActorSpawnParameters Vessel;Vessel.Owner=Builder;Vessel.OverrideLevel=OldLevel;
 RuntimeShipVisualLifetime::ConfigureSpawn(Vessel,*World);
 auto* Carrier=World->SpawnActor<AActor>(Vessel);
 auto* Fighter=World->SpawnActor<AActor>(Vessel);
 if(!TestNotNull(TEXT("Carrier proxy"),Carrier) || !TestNotNull(TEXT("Fighter proxy"),Fighter))return false;
 TestTrue(TEXT("Carrier belongs to persistent simulation scene"),Carrier->GetLevel()==World->PersistentLevel);
 TestTrue(TEXT("Fighter belongs to persistent simulation scene"),Fighter->GetLevel()==World->PersistentLevel);
 TestNull(TEXT("Departing scenery cannot own carrier"),Carrier->GetOwner());
 World->DestroyActor(Builder);
 World->RemoveFromWorld(OldLevel);
 World->RemoveLevel(OldLevel);
 TestFalse(TEXT("Unloading origin scenery does not destroy carrier"),Carrier->IsActorBeingDestroyed());
 TestFalse(TEXT("Unloading origin scenery does not destroy fighter"),Fighter->IsActorBeingDestroyed());
 return true;
}
#endif
