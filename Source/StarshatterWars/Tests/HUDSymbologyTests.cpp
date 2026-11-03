#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "NativeHUDSymbology.h"
#include "WaypointActor.h"
#include "ProceduralMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHUDSymbologyMathTest,"Starshatter.HUD.SharedSymbology",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHUDSymbologyMathTest::RunTest(const FString&){
 TestEqual(TEXT("In-range primary"),FNativeHUDSymbology::CrosshairIndex(1,1000,1000),1);TestEqual(TEXT("Out-of-range primary"),FNativeHUDSymbology::CrosshairIndex(1,1001,1000),4);TestEqual(TEXT("No primary"),FNativeHUDSymbology::CrosshairIndex(1,10,1000,false),4);TestEqual(TEXT("Capital secondary"),FNativeHUDSymbology::CrosshairIndex(2,20000,1000),2);TestEqual(TEXT("Capital third target"),FNativeHUDSymbology::CrosshairIndex(3,20000,1000),3);
 TestEqual(TEXT("North wrap forward"),FNativeHUDSymbology::CompassDelta(350,10),20.0);TestEqual(TEXT("North wrap back"),FNativeHUDSymbology::CompassDelta(10,350),-20.0);TestFalse(TEXT("Legacy five-degree deadband"),FNativeHUDSymbology::HelmCueVisible(0,5,0,0));TestTrue(TEXT("Helm ordered heading cue"),FNativeHUDSymbology::HelmCueVisible(0,6,0,0));TestTrue(TEXT("Helm pitch cue"),FNativeHUDSymbology::HelmCueVisible(0,0,0,6));
 const FVector T(0,0,1000),TV(100,0,0);TestTrue(TEXT("Legacy lead indicator"),FNativeHUDSymbology::LeadPoint(FVector::ZeroVector,FVector::ZeroVector,FVector(0,0,1),T,TV,1000,false).Equals(FVector(100,0,1000)));TestTrue(TEXT("Legacy LCOS sight"),FNativeHUDSymbology::LeadPoint(FVector::ZeroVector,FVector::ZeroVector,FVector(0,0,1),T,TV,1000,true).Equals(FVector(-100,0,1000)));TestEqual(TEXT("Target art retains 64px logical size"),FNativeHUDSymbology::TargetSize,64.0);TestEqual(TEXT("Doubled lead texture retains 32px logical size"),FNativeHUDSymbology::LeadSize,32.0);
 FNativeHUDSymbology Art;return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaypointPlumbobTest,"Starshatter.HUD.PhysicalWaypoint",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWaypointPlumbobTest::RunTest(const FString&){
 TArray<FVector> V,N;TArray<int32>T;TArray<FVector2D>UV;TArray<FLinearColor>C;AWaypointActor::BuildPlumbob(V,T,N,UV,C,240,100);TestEqual(TEXT("Eight independent triangular faces"),V.Num(),24);TestEqual(TEXT("Eight triangles"),T.Num(),24);TestEqual(TEXT("Facet normals"),N.Num(),24);FBox Bounds(V);TestEqual(TEXT("Marker height"),Bounds.GetSize().Z,240.0);for(const auto& Normal:N)TestTrue(TEXT("Face normal normalized"),Normal.IsNormalized());
 auto* World=UWorld::CreateWorld(EWorldType::Game,false);auto* Actor=World->SpawnActor<AWaypointActor>();if(TestNotNull(TEXT("Physical waypoint spawns"),Actor)){
 Actor->WaypointIndex=1;Actor->SectorName=TEXT("Janek");Actor->Description=TEXT("Go to the Janek sector");Actor->bNextWaypoint=true;Actor->RefreshAppearance(true);TestEqual(TEXT("Concise label"),Actor->CompactLabel(),FString(TEXT("NAV 01\nJanek")));TestFalse(TEXT("Physical marker visible"),Actor->IsHidden());TestEqual(TEXT("Marker does not collide"),Actor->WaypointMarker->GetCollisionEnabled(),ECollisionEnabled::NoCollision);TestEqual(TEXT("Arrival radius remains simulation-only"),Actor->ArrivalSphere->GetCollisionEnabled(),ECollisionEnabled::NoCollision);Actor->SetArrivalHullRadius(25);TestEqual(TEXT("Legacy arrival rule unchanged"),Actor->ArrivalRadiusMeters,AWaypointActor::LegacyArrivalRadius(25));TestNotNull(TEXT("Green marker material assigned"),Actor->WaypointMarker->GetMaterial(0));Actor->RefreshAppearance(false);TestTrue(TEXT("Out-of-sector markers can be hidden"),Actor->IsHidden());}
 World->DestroyWorld(false);return true;
}
#endif
