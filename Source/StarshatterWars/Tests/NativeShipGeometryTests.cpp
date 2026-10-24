#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "NativeShipGeometry.h"
#include "ShipActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Camera.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNativeHullGeometryTest,"Starshatter.Audit.F17.NativeHullGeometry",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNativeHullGeometryTest::RunTest(const FString& Parameters)
{
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    auto* Actor=World->SpawnActor<AShipActor>();
    Actor->HullMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Actor->HullMesh->SetWorldTransform(Actor->GetActorTransform());
    Camera Frame;Frame.MoveTo(FVector(1000,2000,3000));
    FNativeShipGeometry Geometry;FVector Hit;Weapon* W=nullptr;
    TestEqual(TEXT("Native mesh stops center ray"),Geometry.Ray(*Actor,Frame,Frame.Pos()+FVector(0,0,-200),FVector(0,0,1),400,Hit,W),1);
    TestTrue(TEXT("Hit uses simulation frame rather than visual actor origin"),Hit.Equals(Frame.Pos()+FVector(0,0,-50),0.01));
    TestEqual(TEXT("A sphere-only corner hit misses the native hull"),Geometry.Ray(*Actor,Frame,Frame.Pos()+FVector(60,60,-200),FVector(0,0,1),400,Hit,W),0);
    Actor->SetActorLocation(FVector(90000,70000,-40000));
    TestEqual(TEXT("Visual scene translation cannot move simulation collision"),Geometry.Ray(*Actor,Frame,Frame.Pos()+FVector(0,0,-200),FVector(0,0,1),400,Hit,W),1);
    World->DestroyWorld(false);return true;
}
#endif
