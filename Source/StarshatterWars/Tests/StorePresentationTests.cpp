#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "ShipWeaponMountComponent.h"
#include "Weapon.h"
#include "WeaponDesign.h"
#include "Engine/World.h"
#include "Engine/TargetPoint.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStorePresentationTest,"Starshatter.Audit.F13.StorePresentation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStorePresentationTest::RunTest(const FString& Parameters)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    AActor* Owner=World->SpawnActor<AActor>();
    auto* Mount=NewObject<UShipWeaponMountComponent>(Owner);Owner->AddInstanceComponent(Mount);
    Mount->bLoadoutHardpoint=true;Mount->RegisterComponent();Mount->SetChildActorClass(ATargetPoint::StaticClass());Mount->CreateChildActor();
    WeaponDesign Design;Design.primary=false;Design.ammo=4;Design.visible_stores=false;
    FVector Muzzle=FVector::ZeroVector;Weapon WeaponSystem(&Design,1,&Muzzle);WeaponSystem.SetAmmo(4);
    Mount->BindRuntimeWeapon(&WeaponSystem,TEXT("Audit"),TEXT("Missile"));
    TestEqual(TEXT("BP capacity follows authored weapon"),Mount->GetStoreCapacity(),4);
    AActor* Child=Mount->GetChildActor();TestNotNull(TEXT("BP-compatible native child exists"),Child);
    if(Child)
    {
        TestFalse(TEXT("Loaded store is visible"),Child->IsHidden());
        WeaponSystem.SetAmmo(3); Mount->UpdateRuntimeStore();
        TestNull(TEXT("Fired missile leaves its rail with reserve ammo remaining"), Mount->GetChildActor());
        WeaponSystem.SetAmmo(0);Mount->UpdateRuntimeStore();TestNull(TEXT("Spent store child is released"),Mount->GetChildActor());
        WeaponSystem.SetAmmo(2);Mount->UpdateRuntimeStore();Child=Mount->GetChildActor();TestNotNull(TEXT("Rearm creates carried store"),Child);if(Child)TestFalse(TEXT("Rearmed store is visible"),Child->IsHidden());
        TestEqual(TEXT("BP sees actual remaining rounds"),Mount->GetRemainingRounds(),2);
        Mount->ClearRuntimeWeapon();TestNull(TEXT("Unbound station releases stale BP store"),Mount->GetChildActor());
    }
    Mount->RuntimeWeapon=nullptr;World->DestroyWorld(false);
    return true;
}
#endif
