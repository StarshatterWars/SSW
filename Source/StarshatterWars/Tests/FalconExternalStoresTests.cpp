#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "FighterActor.h"
#include "ShipWeaponMountComponent.h"
#include "ShipDesignRegistry.h"
#include "WeaponDesignRegistry.h"
#include "Ship.h"
#include "Weapon.h"
#include "MissileActor.h"
#include "Components/StaticMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFalconExternalStoresTest, "Starshatter.Weapons.FalconExternalStores",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFalconExternalStoresTest::RunTest(const FString& Parameters)
{
    const auto Original = ShipDesignRegistry::GetAll();
    const auto OriginalWeapons = WeaponDesignRegistry::GetAll();
    ON_SCOPE_EXIT {
        ShipDesignRegistry::Clear(); WeaponDesignRegistry::Clear();
        for (const auto& P : OriginalWeapons) WeaponDesignRegistry::RegisterDesign(P.Key,P.Value);
        for (const auto& P : Original) ShipDesignRegistry::RegisterDesign(P.Key,P.Value);
    };
    auto* Ships = LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
    auto* Weapons = LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_WeaponDesign.DT_WeaponDesign"));
    if (!TestNotNull(TEXT("Ship definitions"),Ships) || !TestNotNull(TEXT("Weapon definitions"),Weapons)) return false;
    for (const auto& P : Weapons->GetRowMap()) WeaponDesignRegistry::RegisterDesign(P.Key,*reinterpret_cast<const FWeaponDesign*>(P.Value));
    const auto* Row = Ships->FindRow<FShipDesign>(TEXT("Falcon"),TEXT("External stores"));
    if (!TestNotNull(TEXT("Falcon definition"),Row)) return false;
    ShipDesignRegistry::RegisterDesign(FName(TEXT("Falcon")),*Row);
    const auto Options = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World = UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    UClass* Class = LoadClass<AFighterActor>(nullptr,TEXT("/Game/Models/Falcon/BP_Falcon.BP_Falcon_C"));
    if (!TestNotNull(TEXT("Saved Falcon Blueprint"),Class)) return false;
    auto* Falcon = World->SpawnActor<AFighterActor>(Class);
    if (!TestNotNull(TEXT("Falcon actor"),Falcon)) return false;
    // T1's authored ACM Medium Range loadout: Javelin/Vector/Vector/Javelin.
    const int Stations[] = {0,1,1,0};
    Ship Runtime("StoreTest","",ShipDesignRegistry::FindLegacy(FName(TEXT("Falcon"))),1,0,Stations,false);
    Falcon->BindRuntimeShip(&Runtime);
    UClass* DeltaBolt=LoadClass<AActor>(nullptr,TEXT("/Game/Models/Weapons/Bolt/Delta/BP_DeltaBolt.BP_DeltaBolt_C"));
    TestNotNull(TEXT("Delta projectile Blueprint exists"),DeltaBolt);
    TestEqual(TEXT("Falcon gun uses its component projectile class"),Falcon->GetRuntimeWeaponProjectileClass(Runtime.GetPrimary()),DeltaBolt);
    int32 Checked = 0;
    for (const auto& Base : Falcon->NativeWeaponHardpoints)
    {
        auto* Mount = Cast<UShipWeaponMountComponent>(Base.Get());
        if (!Mount || !Mount->bLoadoutHardpoint) continue;
        ++Checked;
        if (!TestNotNull(TEXT("Station has runtime missile"),Mount->RuntimeWeapon)) continue;
        TestNotNull(TEXT("Missile Blueprint spawned at station"),Mount->GetChildActor());
        if (Mount->GetChildActor()) TestEqual(TEXT("Fired missile Blueprint agrees with the loaded station"),Mount->ResolveProjectileClass(),Mount->GetChildActor()->GetClass());
        TestTrue(TEXT("Station identity survives weapon grouping"),Mount->LegacyLocation.Equals(Mount->RuntimeWeapon->GetMountRelativeLocation(),0.001));
        if (Mount->GetChildActor()) TestTrue(TEXT("Missile is attached to its loadout point"),Mount->GetChildActor()->GetRootComponent()->GetAttachParent() == Mount);
        if (auto* Missile=Cast<AMissileActor>(Mount->GetChildActor()))
        {
            TestTrue(TEXT("Missile actor forward matches fighter forward"),Missile->GetActorForwardVector().Equals(Falcon->GetActorForwardVector(),.0001));
            TestTrue(TEXT("Hardpoint position is unchanged"),Missile->GetActorLocation().Equals(Mount->GetComponentLocation(),.001));
            TArray<UStaticMeshComponent*> Bodies;Missile->GetComponents(Bodies);
            for(auto* Body:Bodies)if(Body->GetStaticMesh())
                TestTrue(TEXT("Carried missile nose matches fighter forward"),Body->GetComponentTransform().TransformVector(FVector::RightVector).GetSafeNormal().Equals(Falcon->GetActorForwardVector(),.0001));
        }
        Mount->RuntimeWeapon->SetAmmo(0);
        Mount->UpdateRuntimeStore();
        TestNull(TEXT("Expended carried missile is destroyed"),Mount->GetChildActor());
        Mount->RuntimeWeapon->SetAmmo(1);
        Mount->UpdateRuntimeStore();
        TestNotNull(TEXT("Rearmed station recreates its carried missile"),Mount->GetChildActor());
    }
    TestEqual(TEXT("All four Falcon loadout stations checked"),Checked,4);
    const int32 Count = Falcon->NativeWeaponHardpoints.Num();
    Falcon->ConfigureWeaponHardpointsFromShipDef(TEXT("Falcon"));
    TestEqual(TEXT("Repeated configuration retains stations without duplicates"),Falcon->NativeWeaponHardpoints.Num(),Count);
    Falcon->BindRuntimeShip(nullptr);
    return true;
}
#endif
