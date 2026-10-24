#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "BerentsShipActor.h"
#include "ShipWeaponMountComponent.h"
#include "Components/ChildActorComponent.h"
#include "Engine/World.h"
#include "Weapon.h"
#include "WeaponDesign.h"
#include "Camera.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNativeWeaponMuzzleTest,"Starshatter.Audit.F18.NativeMuzzle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNativeWeaponMuzzleTest::RunTest(const FString& Parameters)
{
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    auto* Actor=World->SpawnActor<ABerentsShipActor>();
    WeaponDesign Design;FVector Original=FVector::ZeroVector;Weapon Gun(&Design,1,&Original);
    auto* Mount=Cast<UShipWeaponMountComponent>(Actor->AuthoredWeaponPointComponents[0].Get());Mount->BindRuntimeWeapon(&Gun,NAME_None,NAME_None);
    if(!TestTrue(TEXT("Native barrel exists"),Mount->BarrelComponents.Num()>0)){World->DestroyWorld(false);return false;}
    auto* Barrel=Mount->BarrelComponents[0].Get();Barrel->SetWorldLocation(FVector(120,30,45));
    Camera Frame;Frame.MoveTo(FVector(1000,2000,3000));FVector Position;Camera Rail;
    TestTrue(TEXT("Live weapon resolves native barrel"),Actor->GetSimulationWeaponMuzzle(Frame,&Gun,0,Position,Rail));
    const FVector Local=Actor->GetActorQuat().UnrotateVector(Barrel->GetComponentLocation()-Actor->GetActorLocation());
    TestTrue(TEXT("Firing origin uses BP barrel in simulation coordinates"),Position.Equals(Frame.Pos()+Frame.vpn()*Local.X+Frame.vrt()*Local.Y+Frame.vup()*Local.Z,0.001));
    Barrel->AddWorldOffset(FVector(10,0,0));FVector Moved;
    Actor->GetSimulationWeaponMuzzle(Frame,&Gun,0,Moved,Rail);
    TestTrue(TEXT("Moving BP barrel changes physical firing origin"),!Moved.Equals(Position));
    Mount->ClearRuntimeWeapon();World->DestroyWorld(false);return true;
}
#endif
