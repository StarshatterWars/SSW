#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "FighterActor.h"
#include "FighterLandingGearComponent.h"
#include "Components/ChildActorComponent.h"
#include "Engine/World.h"
#include "LandingGear.h"
#include "AvengerShipActor.h"
#include "CobraShipActor.h"
#include "CutlassShipActor.h"
#include "EagleShipActor.h"
#include "FalconShipActor.h"
#include "IntruderShipActor.h"
#include "JackalShipActor.h"
#include "RaptorShipActor.h"
#include "RazorShipActor.h"
#include "SaberShipActor.h"
#include "StormhawkShipActor.h"
#include "ThunderboltShipActor.h"
#include "ViperShipActor.h"
#include "ZodiacShipActor.h"
#include "ZZAShipActor.h"
#include "ZZFShipActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFighterNativeAttachmentsTest, "Starshatter.Fighter.NativeAttachments",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFighterNativeAttachmentsTest::RunTest(const FString& Parameters)
{
    struct FExpected { UClass* Class; int32 GearCount; };
    const FExpected Designs[] = {
        { AAvengerShipActor::StaticClass(), 3 },
        { ACobraShipActor::StaticClass(), 3 },
        { ACutlassShipActor::StaticClass(), 2 },
        { AEagleShipActor::StaticClass(), 3 },
        { AFalconShipActor::StaticClass(), 3 },
        { AIntruderShipActor::StaticClass(), 3 },
        { AJackalShipActor::StaticClass(), 4 },
        { ARaptorShipActor::StaticClass(), 3 },
        { ARazorShipActor::StaticClass(), 3 },
        { ASaberShipActor::StaticClass(), 2 },
        { AStormhawkShipActor::StaticClass(), 3 },
        { AThunderboltShipActor::StaticClass(), 3 },
        { AViperShipActor::StaticClass(), 3 },
        { AZodiacShipActor::StaticClass(), 4 },
        { AZZAShipActor::StaticClass(), 0 },
        { AZZFShipActor::StaticClass(), 0 },
    };
    for (const FExpected& Design : Designs)
    {
        AFighterActor* Defaults = CastChecked<AFighterActor>(Design.Class->GetDefaultObject());
        const FString Prefix = Design.Class->GetName() + TEXT(": ");
        TestEqual(*(Prefix+TEXT("exact authored gear count")), Defaults->LandingGearActors.Num(), Design.GearCount);
        TestNotNull(*(Prefix+TEXT("one cockpit child actor component")), Defaults->CockpitActor.Get());
        TestTrue(*(Prefix+TEXT("cockpit is native and BP editable")),
            Defaults->CockpitActor->CreationMethod == EComponentCreationMethod::Native && Defaults->CockpitActor->bEditableWhenInherited);
        for (int32 Index=0; Index<Defaults->LandingGearActors.Num(); ++Index)
        {
            UFighterLandingGearComponent* Gear = Defaults->LandingGearActors[Index];
            TestTrue(*(Prefix+TEXT("native BP-editable gear")), Gear &&
                Gear->CreationMethod == EComponentCreationMethod::Native && Gear->bEditableWhenInherited);
            if (!Gear) continue;
            TestEqual(*(Prefix+TEXT("definition order preserved")), Gear->AuthoredIndex, Index);
            TestEqual(*(Prefix+TEXT("gear model preserved")), Gear->AuthoredModel, Defaults->GearDefinitions[Index].Model);
            TestEqual(*(Prefix+TEXT("authored start preserved")), Gear->GetRelativeLocation(), Defaults->GearDefinitions[Index].RetractedPosition);
        }
    }
    const UWorld::InitializationValues WorldOptions = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false, NAME_None, nullptr, true,
        ERHIFeatureLevel::Num, &WorldOptions);
    if (!TestNotNull(TEXT("Preview world"), World)) return false;
    ARaptorShipActor* Fighter = World->SpawnActor<ARaptorShipActor>();
    if (!TestNotNull(TEXT("Raptor preview"), Fighter)) { World->DestroyWorld(false); return false; }
    UFighterLandingGearComponent* Nose = Fighter->LandingGearActors[0];
    TestEqual(TEXT("Raptor DEF start axis conversion"), Nose->RetractedPosition, FVector(134,0,-25));
    TestEqual(TEXT("Raptor DEF end axis conversion"), Nose->ExtendedPosition, FVector(148,0,-50));
    Nose->SetChildActorClass(AActor::StaticClass());
    Fighter->CockpitActor->SetChildActorClass(AActor::StaticClass());
    Fighter->PreviewGearDeployment = 0.5f;
    Fighter->SynchronizeLandingGearMarkers();
    TestEqual(TEXT("Half deployment follows legacy start/end"), Nose->GetRelativeLocation(), FVector(141,0,-37.5));
    Fighter->RerunConstructionScripts();
    Fighter->RerunConstructionScripts();
    TestEqual(TEXT("Reconstruction retains assigned gear class"), Nose->GetChildActorClass(), TSubclassOf<AActor>(AActor::StaticClass()));
    TestEqual(TEXT("Reconstruction retains assigned cockpit class"), Fighter->CockpitActor->GetChildActorClass(), TSubclassOf<AActor>(AActor::StaticClass()));
    TArray<UFighterLandingGearComponent*> GearComponents;
    Fighter->GetComponents(GearComponents);
    TestEqual(TEXT("Repeated construction creates no gear duplicates"), GearComponents.Num(),3);
    TArray<USceneComponent*> Components;
    Fighter->GetComponents(Components);
    for (USceneComponent* Component : Components)
        TestFalse(TEXT("No extra gear endpoint marker"), Component->GetName().StartsWith(TEXT("LandingGearExtended_")));
    Fighter->PreviewGearDeployment = 1;
    Fighter->SynchronizeLandingGearMarkers();
    TestEqual(TEXT("Full deployment reaches authored end"), Nose->GetRelativeLocation(), Nose->ExtendedPosition);
    Fighter->PreviewGearDeployment = 0;
    Fighter->SynchronizeLandingGearMarkers();
    TestTrue(TEXT("Retracted gear child is hidden"), Nose->GetChildActor() && Nose->GetChildActor()->IsHidden());
    TestFalse(TEXT("Retracted component is invisible"), Nose->IsVisible());
    Nose->bAnimateTranslation = false;
    const FVector CustomPosition(1,2,3);
    Nose->SetRelativeLocation(CustomPosition);
    Nose->ApplyDeployment(0.6f);
    TestEqual(TEXT("Custom BP animation can retain its own transform"), Nose->GetRelativeLocation(), CustomPosition);
    TestEqual(TEXT("Custom animation still receives deployment"), Nose->Deployment,0.6f);
    Nose->ApplyDeployment(2);
    TestEqual(TEXT("Presentation clamps deployment"), Nose->Deployment,1.f);
    LandingGear Legacy;
    Legacy.SetState(LandingGear::GEAR_LOWER);
    Legacy.ExecFrame(0.25);
    TestTrue(TEXT("Reader uses actual legacy transit"), FMath::IsNearlyEqual(Legacy.GetDeployment(),0.25));
    Legacy.SetState(LandingGear::GEAR_RAISE);
    Legacy.ExecFrame(0.1);
    TestTrue(TEXT("Legacy reversal remains continuous"), FMath::IsNearlyEqual(Legacy.GetDeployment(),0.15));
    World->DestroyWorld(false);
    return true;
}
#endif


#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "ShipWeaponMountComponent.h"
#include "OrionShipActor.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFighterWeaponRolesTest, "Starshatter.Fighter.WeaponRoles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFighterWeaponRolesTest::RunTest(const FString& Parameters)
{
    struct FExpected { UClass* Class; int32 Hull; int32 Loadout; };
    const FExpected Designs[] = {
        { AAvengerShipActor::StaticClass(), 1, 8 },
        { ACobraShipActor::StaticClass(), 1, 8 },
        { ACutlassShipActor::StaticClass(), 1, 2 },
        { AEagleShipActor::StaticClass(), 1, 8 },
        { AFalconShipActor::StaticClass(), 1, 4 },
        { AIntruderShipActor::StaticClass(), 1, 4 },
        { AJackalShipActor::StaticClass(), 2, 0 },
        { ARaptorShipActor::StaticClass(), 1, 8 },
        { ARazorShipActor::StaticClass(), 1, 8 },
        { ASaberShipActor::StaticClass(), 1, 4 },
        { AStormhawkShipActor::StaticClass(), 1, 6 },
        { AThunderboltShipActor::StaticClass(), 2, 6 },
        { AViperShipActor::StaticClass(), 1, 6 },
        { AZodiacShipActor::StaticClass(), 2, 0 },
        { AZZAShipActor::StaticClass(), 1, 0 },
        { AZZFShipActor::StaticClass(), 1, 0 },
    };
    for (const FExpected& Design : Designs)
    {
        AFighterActor* Defaults = CastChecked<AFighterActor>(Design.Class->GetDefaultObject());
        const FString Prefix = Design.Class->GetName()+TEXT(": ");
        TestEqual(*(Prefix+TEXT("authored hull weapon count")), Defaults->HullWeaponMounts.Num(), Design.Hull);
        TestEqual(*(Prefix+TEXT("authored loadout count")), Defaults->LoadoutHardpoints.Num(), Design.Loadout);
        TestEqual(*(Prefix+TEXT("combined station count retained")), Defaults->AuthoredWeaponPointComponents.Num(), Design.Hull+Design.Loadout);
        for (const TObjectPtr<USceneComponent>& Component : Defaults->HullWeaponMounts)
        {
            auto* Mount = CastChecked<UShipWeaponMountComponent>(Component.Get());
            TestFalse(*(Prefix+TEXT("hull metadata available before runtime binding")), Mount->bLoadoutHardpoint);
            TestEqual(*(Prefix+TEXT("hull weapons have their own root")), Mount->GetAttachParent(), Defaults->HullWeaponRoot.Get());
            TestTrue(*(Prefix+TEXT("hull component is clearly named")), Mount->GetName().StartsWith(TEXT("HullWeapon_")));
            TestTrue(*(Prefix+TEXT("hull weapon type metadata")), Mount->AllowedWeaponTypes.Num()>0);
            TestTrue(*(Prefix+TEXT("native component editable in BP")), Mount->bEditableWhenInherited && Mount->CreationMethod==EComponentCreationMethod::Native);
        }
        for (const TObjectPtr<USceneComponent>& Component : Defaults->LoadoutHardpoints)
        {
            auto* Mount = CastChecked<UShipWeaponMountComponent>(Component.Get());
            TestTrue(*(Prefix+TEXT("loadout role available before runtime binding")), Mount->bLoadoutHardpoint);
            TestEqual(*(Prefix+TEXT("loadout parent remains separate")), Mount->GetAttachParent(), Defaults->WeaponHardpointRoot.Get());
            TestFalse(*(Prefix+TEXT("hull/loadout arrays do not duplicate components")), Defaults->HullWeaponMounts.Contains(Component));
        }
    }
    auto* Carrier = GetDefault<AOrionShipActor>();
    TestNotNull(TEXT("Orion exposes the shared hull weapon hierarchy"), Carrier->HullWeaponRoot.Get());
    TestEqual(TEXT("Orion weapon count preserved"), Carrier->AuthoredWeaponPointComponents.Num(),10);
    const UWorld::InitializationValues Options = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    ARaptorShipActor* Fighter = World->SpawnActor<ARaptorShipActor>();
    auto* Cannon = CastChecked<UShipWeaponMountComponent>(Fighter->HullWeaponMounts[0].Get());
    TestEqual(TEXT("Raptor twin cannon remains one authored weapon"), Fighter->HullWeaponMounts.Num(),1);
    TestEqual(TEXT("Raptor cannon has both authored muzzles before runtime binding"), Cannon->GetMuzzleCount(),2);
    TestEqual(TEXT("Raptor first muzzle"), Cannon->GetMuzzleWorldLocation(0), FVector(42.5,-4.25,-4.75));
    TestEqual(TEXT("Raptor second muzzle"), Cannon->GetMuzzleWorldLocation(1), FVector(42.5,4.25,-4.75));
    Cannon->SetChildActorClass(AActor::StaticClass());
    auto* Store = CastChecked<UShipWeaponMountComponent>(Fighter->LoadoutHardpoints[0].Get());
    Store->SetChildActorClass(AActor::StaticClass());
    Fighter->RerunConstructionScripts();
    Fighter->RerunConstructionScripts();
    TestEqual(TEXT("Hull child class retained"), Cannon->GetChildActorClass(), TSubclassOf<AActor>(AActor::StaticClass()));
    TestEqual(TEXT("Loadout child class retained"), Store->GetChildActorClass(), TSubclassOf<AActor>(AActor::StaticClass()));
    TestEqual(TEXT("Reconstruction preserves one hull weapon and eight loadout stations"), Fighter->AuthoredWeaponPointComponents.Num(),9);
    World->DestroyWorld(false);
    return true;
}
#endif
