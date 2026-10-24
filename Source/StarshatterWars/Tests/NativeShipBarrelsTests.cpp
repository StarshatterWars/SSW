#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "ShipActor.h"
#include "ShipWeaponMountComponent.h"
#include "Components/ChildActorComponent.h"
#include "Engine/World.h"
#include "AdventureShipActor.h"
#include "AsherShipActor.h"
#include "AvengerShipActor.h"
#include "BaikalShipActor.h"
#include "BerentsShipActor.h"
#include "BroadswordShipActor.h"
#include "CobraShipActor.h"
#include "CourageousShipActor.h"
#include "CourierShipActor.h"
#include "CutlassShipActor.h"
#include "DevastatorShipActor.h"
#include "DragonShipActor.h"
#include "EagleShipActor.h"
#include "FalconShipActor.h"
#include "FarcasterShipActor.h"
#include "FortressShipActor.h"
#include "Gnd_aaa1ShipActor.h"
#include "Gnd_aaa2ShipActor.h"
#include "Gnd_sam1ShipActor.h"
#include "GoliathShipActor.h"
#include "HulkBroadswordShipActor.h"
#include "HulkDragonShipActor.h"
#include "HulkImperialShipActor.h"
#include "HulkTigerShipActor.h"
#include "HulkVendettaShipActor.h"
#include "HulkVolnarisShipActor.h"
#include "IM2800ShipActor.h"
#include "IM3500ShipActor.h"
#include "IM4300ShipActor.h"
#include "ImperialShipActor.h"
#include "IntruderShipActor.h"
#include "JackalShipActor.h"
#include "MarauderShipActor.h"
#include "Mine1ShipActor.h"
#include "ObservatoryShipActor.h"
#include "OrionShipActor.h"
#include "QShipShipActor.h"
#include "RaptorShipActor.h"
#include "RazorShipActor.h"
#include "S5ShipActor.h"
#include "S6ShipActor.h"
#include "S7ShipActor.h"
#include "SaberShipActor.h"
#include "SpectreShipActor.h"
#include "StormhawkShipActor.h"
#include "ThunderboltShipActor.h"
#include "TigerShipActor.h"
#include "VendettaShipActor.h"
#include "ViperShipActor.h"
#include "VolnarisShipActor.h"
#include "WolfShipActor.h"
#include "ZodiacShipActor.h"
#include "ZZAShipActor.h"
#include "ZZCAShipActor.h"
#include "ZZCSShipActor.h"
#include "ZZDDShipActor.h"
#include "ZZFFShipActor.h"
#include "ZZFShipActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNativeShipBarrelsTest,"Starshatter.Ships.NativeBarrels",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNativeShipBarrelsTest::RunTest(const FString& Parameters)
{
    struct FExpected { UClass* Class; int32 Weapons; int32 Barrels; int32 Loadout; };
    const FExpected Designs[] = {
        { AAdventureShipActor::StaticClass(), 6, 6, 0 },
        { AAsherShipActor::StaticClass(), 8, 11, 0 },
        { AAvengerShipActor::StaticClass(), 1, 4, 8 },
        { ABaikalShipActor::StaticClass(), 4, 5, 0 },
        { ABerentsShipActor::StaticClass(), 6, 9, 0 },
        { ABroadswordShipActor::StaticClass(), 12, 16, 0 },
        { ACobraShipActor::StaticClass(), 1, 2, 8 },
        { ACourageousShipActor::StaticClass(), 13, 24, 0 },
        { ACourierShipActor::StaticClass(), 0, 0, 0 },
        { ACutlassShipActor::StaticClass(), 1, 4, 2 },
        { ADevastatorShipActor::StaticClass(), 10, 16, 0 },
        { ADragonShipActor::StaticClass(), 12, 22, 0 },
        { AEagleShipActor::StaticClass(), 1, 2, 8 },
        { AFalconShipActor::StaticClass(), 1, 2, 4 },
        { AFarcasterShipActor::StaticClass(), 0, 0, 0 },
        { AFortressShipActor::StaticClass(), 6, 12, 0 },
        { AGnd_aaa1ShipActor::StaticClass(), 1, 2, 0 },
        { AGnd_aaa2ShipActor::StaticClass(), 1, 4, 0 },
        { AGnd_sam1ShipActor::StaticClass(), 1, 8, 0 },
        { AGoliathShipActor::StaticClass(), 4, 8, 0 },
        { AHulkBroadswordShipActor::StaticClass(), 12, 16, 0 },
        { AHulkDragonShipActor::StaticClass(), 12, 22, 0 },
        { AHulkImperialShipActor::StaticClass(), 12, 16, 0 },
        { AHulkTigerShipActor::StaticClass(), 5, 6, 0 },
        { AHulkVendettaShipActor::StaticClass(), 11, 16, 0 },
        { AHulkVolnarisShipActor::StaticClass(), 6, 10, 0 },
        { AIM2800ShipActor::StaticClass(), 0, 0, 0 },
        { AIM3500ShipActor::StaticClass(), 0, 0, 0 },
        { AIM4300ShipActor::StaticClass(), 0, 0, 0 },
        { AImperialShipActor::StaticClass(), 12, 16, 0 },
        { AIntruderShipActor::StaticClass(), 1, 2, 4 },
        { AJackalShipActor::StaticClass(), 2, 2, 0 },
        { AMarauderShipActor::StaticClass(), 4, 4, 0 },
        { AMine1ShipActor::StaticClass(), 1, 4, 0 },
        { AObservatoryShipActor::StaticClass(), 0, 0, 0 },
        { AOrionShipActor::StaticClass(), 10, 20, 0 },
        { AQShipShipActor::StaticClass(), 6, 6, 0 },
        { ARaptorShipActor::StaticClass(), 1, 2, 8 },
        { ARazorShipActor::StaticClass(), 1, 2, 8 },
        { AS5ShipActor::StaticClass(), 8, 16, 0 },
        { AS6ShipActor::StaticClass(), 4, 8, 0 },
        { AS7ShipActor::StaticClass(), 12, 20, 0 },
        { ASaberShipActor::StaticClass(), 1, 2, 4 },
        { ASpectreShipActor::StaticClass(), 10, 14, 0 },
        { AStormhawkShipActor::StaticClass(), 1, 2, 6 },
        { AThunderboltShipActor::StaticClass(), 2, 3, 6 },
        { ATigerShipActor::StaticClass(), 5, 6, 0 },
        { AVendettaShipActor::StaticClass(), 11, 16, 0 },
        { AViperShipActor::StaticClass(), 1, 2, 6 },
        { AVolnarisShipActor::StaticClass(), 6, 10, 0 },
        { AWolfShipActor::StaticClass(), 7, 12, 0 },
        { AZodiacShipActor::StaticClass(), 2, 6, 0 },
        { AZZAShipActor::StaticClass(), 1, 1, 0 },
        { AZZCAShipActor::StaticClass(), 4, 4, 0 },
        { AZZCSShipActor::StaticClass(), 4, 4, 0 },
        { AZZDDShipActor::StaticClass(), 2, 2, 0 },
        { AZZFFShipActor::StaticClass(), 4, 4, 0 },
        { AZZFShipActor::StaticClass(), 1, 1, 0 },
    };
    for (const FExpected& Design : Designs)
    {
        auto* Ship = CastChecked<AShipActor>(Design.Class->GetDefaultObject());
        const FString Prefix = Design.Class->GetName()+TEXT(": ");
        TestEqual(*(Prefix+TEXT("exact authored weapon count")), Ship->HullWeaponMounts.Num(), Design.Weapons);
        TestEqual(*(Prefix+TEXT("exact authored loadout count")), Ship->LoadoutHardpoints.Num(), Design.Loadout);
        int32 Barrels = 0;
        for (const TObjectPtr<USceneComponent>& Component : Ship->HullWeaponMounts)
        {
            auto* Mount = CastChecked<UShipWeaponMountComponent>(Component.Get());
            TestEqual(*(Prefix+TEXT("hull parent")), Mount->GetAttachParent(), Ship->HullWeaponRoot.Get());
            Barrels += Mount->BarrelComponents.Num();
            for (UChildActorComponent* Barrel : Mount->BarrelComponents)
            {
                TestTrue(*(Prefix+TEXT("native BP-editable barrel")), Barrel && Barrel->bEditableWhenInherited && Barrel->CreationMethod==EComponentCreationMethod::Native);
                if (Barrel) TestEqual(*(Prefix+TEXT("barrel is a subcomponent of its weapon")), Barrel->GetAttachParent(), static_cast<USceneComponent*>(Mount));
            }
        }
        TestEqual(*(Prefix+TEXT("exact ship/weapon DEF barrel count")),Barrels,Design.Barrels);
        for (const TObjectPtr<USceneComponent>& Component : Ship->LoadoutHardpoints)
            TestEqual(*(Prefix+TEXT("loadout stores do not acquire gun barrel slots")),CastChecked<UShipWeaponMountComponent>(Component.Get())->BarrelComponents.Num(),0);
    }
    const UWorld::InitializationValues Options = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    AOrionShipActor* Orion = World->SpawnActor<AOrionShipActor>();
    TestEqual(TEXT("Orion retains ten weapons"), Orion->HullWeaponMounts.Num(),10);
    TestEqual(TEXT("Orion retains ten turret mounts"),Orion->AuthoredTurretPointComponents.Num(),10);
    auto* Vanguard = CastChecked<UShipWeaponMountComponent>(Orion->HullWeaponMounts[0].Get());
    TestEqual(TEXT("Vanguard uses both Wep.def muzzles"),Vanguard->BarrelComponents.Num(),2);
    const FVector ExpectedLocal(18,2.1,0); // (60,7,0) * weapon scale 1.2 / Orion scale 4.
    TestTrue(TEXT("Weapon scale is compensated once under Orion scale"),Vanguard->BarrelComponents[0]->GetRelativeLocation().Equals(ExpectedLocal,0.00001));
    const FVector ExpectedWorld = Vanguard->GetComponentTransform().TransformPosition(ExpectedLocal);
    TestTrue(TEXT("Runtime muzzle getter uses exposed barrel"),Vanguard->GetMuzzleWorldLocation(0).Equals(ExpectedWorld,0.0001));
    UChildActorComponent* Barrel = Vanguard->BarrelComponents[0];
    Barrel->SetChildActorClass(AActor::StaticClass());
    Barrel->SetRelativeLocation(FVector(21,4,2));
    TestEqual(TEXT("Edited barrel is the firing transform source"),Vanguard->GetMuzzleWorldLocation(0),Barrel->GetComponentLocation());
    Orion->RerunConstructionScripts();
    Orion->RerunConstructionScripts();
    TestEqual(TEXT("Barrel class survives reconstruction"),Barrel->GetChildActorClass(),TSubclassOf<AActor>(AActor::StaticClass()));
    TestEqual(TEXT("Barrel placement survives reconstruction"),Barrel->GetRelativeLocation(),FVector(21,4,2));
    int32 Count=0;
    for (const auto& Component : Orion->HullWeaponMounts)
        Count += CastChecked<UShipWeaponMountComponent>(Component.Get())->BarrelComponents.Num();
    TestEqual(TEXT("Reconstruction does not duplicate Orion barrels"),Count,20);
    World->DestroyWorld(false);
    return true;
}
#endif
