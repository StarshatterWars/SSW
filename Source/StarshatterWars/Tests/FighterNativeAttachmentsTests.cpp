#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "FighterActor.h"
#include "FighterLandingGearComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Engine/World.h"
#include "LandingGear.h"
#include "Ship.h"
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

class FGearTestShip : public Ship
{
public:
    FGearTestShip() : Ship("GearTest", "", static_cast<ShipDesign*>(nullptr), 1, 0, nullptr, false) {}
    void SetTestGear(LandingGear* InGear) { if (gear) systems.remove(gear); gear = InGear; if (gear) systems.append(gear); }
};

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
        TestEqual(*(Prefix+TEXT("cockpit mesh belongs to cockpit component")), Defaults->Cockpit->GetAttachParent(), static_cast<USceneComponent*>(Defaults->CockpitComponent.Get()));
        TestNotNull(*(Prefix+TEXT("one cockpit child actor component")), Defaults->CockpitComponent.Get());
        TestTrue(*(Prefix+TEXT("cockpit is native and BP editable")),
            Defaults->CockpitComponent->CreationMethod == EComponentCreationMethod::Native && Defaults->CockpitComponent->bEditableWhenInherited);
        for (int32 Index=0; Index<Defaults->LandingGearActors.Num(); ++Index)
        {
            UFighterLandingGearComponent* Gear = Defaults->LandingGearActors[Index];
            TestTrue(*(Prefix+TEXT("native BP-editable gear")), Gear &&
                Gear->CreationMethod == EComponentCreationMethod::Native && Gear->bEditableWhenInherited);
            if (!Gear) continue;
            TestEqual(*(Prefix+TEXT("gear attaches directly to landing gear root")), Gear->GetAttachParent(), Defaults->FighterLandingGearRoot.Get());
            TestTrue(*(Prefix+TEXT("gear yaw is minus ninety")), Gear->GetRelativeRotation().Equals(FRotator(0,-90,0)));
            UStaticMeshComponent* Mesh = Index == 0 ? Defaults->NoseGear.Get() : Index == 1 ? Defaults->LeftGear.Get() : Index == 2 ? Defaults->RightGear.Get() : nullptr;
            if (Mesh) TestEqual(*(Prefix+TEXT("mesh belongs to matching gear actor")), Mesh->GetAttachParent(), static_cast<USceneComponent*>(Gear));
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
    Fighter->CockpitComponent->SetChildActorClass(AActor::StaticClass());
    Fighter->PreviewGearDeployment = 0.5f;
    Fighter->SynchronizeLandingGearMarkers();
    TestEqual(TEXT("Half deployment follows legacy start/end"), Nose->GetRelativeLocation(), FVector(141,0,-37.5));
    Fighter->RerunConstructionScripts();
    Fighter->RerunConstructionScripts();
    TestEqual(TEXT("Reconstruction retains assigned gear class"), Nose->GetChildActorClass(), TSubclassOf<AActor>(AActor::StaticClass()));
    TestEqual(TEXT("Reconstruction retains assigned cockpit class"), Fighter->CockpitComponent->GetChildActorClass(), TSubclassOf<AActor>(AActor::StaticClass()));
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
    Nose->ApplyDeployment(1.f, true, false);
    TestFalse(TEXT("Simulation visibility hides each gear even at full extension"), Nose->bGearVisible);
    TestTrue(TEXT("Simulation visibility hides the child actor"), Nose->GetChildActor()->IsHidden());
    Nose->ApplyDeployment(1.f, true, true);
    TestTrue(TEXT("Down visibility restores the gear"), Nose->bGearVisible);
    LandingGear Legacy;
    Legacy.SetState(LandingGear::GEAR_LOWER);
    Legacy.ExecFrame(0.25);
    TestTrue(TEXT("Reader uses actual legacy transit"), FMath::IsNearlyEqual(Legacy.GetDeployment(),0.25));
    Legacy.SetState(LandingGear::GEAR_RAISE);
    Legacy.ExecFrame(0.1);
    TestTrue(TEXT("Legacy reversal remains continuous"), FMath::IsNearlyEqual(Legacy.GetDeployment(),0.15));
    UClass* FalconBP = LoadClass<AFighterActor>(nullptr, TEXT("/Game/Models/Falcon/BP_Falcon.BP_Falcon_C"));
    if (TestNotNull(TEXT("Actual Falcon Blueprint loads"), FalconBP))
    {
        AFighterActor* LiveFalcon = World->SpawnActor<AFighterActor>(FalconBP);
        if (TestNotNull(TEXT("Actual Falcon Blueprint spawns"), LiveFalcon))
        {
            UClass* FalconPit = LoadClass<AActor>(nullptr,TEXT("/Game/Models/Cockpit/Falcon/BP_FalconPit.BP_FalconPit_C"));
            TestNotNull(TEXT("FalconPit Blueprint exists"),FalconPit);
            TestEqual(TEXT("Saved Falcon uses FalconPit cockpit Blueprint"),LiveFalcon->CockpitComponent->GetChildActorClass().Get(),FalconPit);
            TestNotNull(TEXT("Falcon cockpit child is instantiated"),LiveFalcon->CockpitComponent->GetChildActor());
            auto CanopyCenter=[](UStaticMeshComponent* Component,FName SlotName,FVector& Center)
            {
                if (!Component || !Component->GetStaticMesh() || !Component->GetStaticMesh()->GetRenderData()) return false;
                UStaticMesh* Mesh=Component->GetStaticMesh();
                const int32 Material=Mesh->GetMaterialIndex(SlotName);
                const auto& LOD=Mesh->GetRenderData()->LODResources[0];
                FBox Bounds(ForceInit);
                for (const auto& Section:LOD.Sections)
                    if (Section.MaterialIndex==Material)
                        for (uint32 I=Section.FirstIndex;I<Section.FirstIndex+Section.NumTriangles*3;++I)
                            Bounds+=FVector(LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(LOD.IndexBuffer.GetIndex(I)));
                if (!Bounds.IsValid) return false;
                Center=Component->GetComponentTransform().TransformPosition(Bounds.GetCenter()); return true;
            };
            FVector HullCanopy,CockpitCanopy;
            TestTrue(TEXT("Hull glass section exists"),CanopyCenter(LiveFalcon->HullMesh,TEXT("glass"),HullCanopy));
            TArray<UStaticMeshComponent*> CockpitMeshes;
            LiveFalcon->CockpitComponent->GetChildActor()->GetComponents(CockpitMeshes);
            bool FoundCockpitCanopy=false;
            for (auto* Mesh:CockpitMeshes)
                if (CanopyCenter(Mesh,TEXT("CANOPY_D"),CockpitCanopy)) { FoundCockpitCanopy=true; break; }
            TestTrue(TEXT("FalconPit canopy section exists"),FoundCockpitCanopy);
            TestTrue(TEXT("Imported cockpit canopy aligns with actual hull canopy"),HullCanopy.Equals(CockpitCanopy,0.05));

            TestTrue(TEXT("Falcon cockpit mount converts DEF bridge forward and height"),
                LiveFalcon->FighterCockpitRoot->GetRelativeLocation().Equals(FVector(131.f,0.f,23.f)));
            TestTrue(TEXT("Falcon cockpit yaw matches gear minus ninety"),
                LiveFalcon->CockpitComponent->GetRelativeRotation().Equals(FRotator(0.f,-90.f,0.f)));
            TestTrue(TEXT("Falcon cockpit uses independent legacy model scale"),
                LiveFalcon->CockpitComponent->GetRelativeScale3D().Equals(FVector(0.02f/0.27f),0.0001f));
            LiveFalcon->RerunConstructionScripts();
            TestTrue(TEXT("Cockpit scale is not compounded by reconstruction"),
                LiveFalcon->CockpitComponent->GetRelativeScale3D().Equals(FVector(0.02f/0.27f),0.0001f));
            AddExpectedError(TEXT("No ShipDesign for 'GearTest'"), EAutomationExpectedErrorFlags::Contains, 1);
            FGearTestShip RuntimeShip;
            LandingGear RuntimeGear;
            RuntimeGear.AddGear(nullptr, FVector::ZeroVector, FVector(0,-1,0));
            RuntimeGear.AddGear(nullptr, FVector::ZeroVector, FVector(-2,-1,0));
            RuntimeGear.AddGear(nullptr, FVector::ZeroVector, FVector(2,-1,0));
            RuntimeShip.SetTestGear(&RuntimeGear);
            LiveFalcon->BindRuntimeShip(&RuntimeShip);
            RuntimeGear.SetState(LandingGear::GEAR_DOWN);
            RuntimeShip.ExecSystems(0);
            LiveFalcon->Tick(0.01f);
            for (UFighterLandingGearComponent* Gear : LiveFalcon->LandingGearActors)
                TestTrue(TEXT("Bound runtime gear shows down state"), Gear->bGearVisible);
            TArray<UChildActorComponent*> DeployedAssemblies;
            LiveFalcon->GetComponents(DeployedAssemblies);
            for (auto* Assembly : DeployedAssemblies)
            {
                const FString Name = Assembly->GetName();
                if (!Name.Contains(TEXT("Gear"))) continue;
                TestTrue(TEXT("Saved gear assembly follows fighter gear root"),Assembly->GetAttachParent()==LiveFalcon->FighterLandingGearRoot);
                if (Name.Contains(TEXT("LeftGear")))
                {
                    TestTrue(TEXT("Left gear follows legacy negative lateral coordinate"),Assembly->GetRelativeLocation().Y < 0);
                    TestTrue(TEXT("Left station uses swapped RightGear Blueprint"),GetNameSafe(Assembly->GetChildActorClass()).Contains(TEXT("BP_RightGear")));
                }
                if (Name.Contains(TEXT("RightGear")))
                {
                    TestTrue(TEXT("Right gear follows legacy positive lateral coordinate"),Assembly->GetRelativeLocation().Y > 0);
                    TestTrue(TEXT("Right station uses swapped LeftGear Blueprint"),GetNameSafe(Assembly->GetChildActorClass()).Contains(TEXT("BP_LeftGear")));
                }
                if (AActor* GearActor = Assembly->GetChildActor())
                {
                    FBox Bounds(ForceInit);
                    TArray<UStaticMeshComponent*> GearMeshes;
                    GearActor->GetComponents(GearMeshes);
                    for (auto* Mesh : GearMeshes)
                        if (Mesh->GetStaticMesh()) Bounds += Mesh->CalcBounds(Mesh->GetComponentTransform().GetRelativeTransform(GearActor->GetActorTransform())).GetBox();
                    TestTrue(TEXT("Wheel bottom aligns with legacy deployed endpoint"),Bounds.IsValid && FMath::IsNearlyZero(Bounds.Min.Z,0.05));
                }
            }
            RuntimeGear.SetState(LandingGear::GEAR_RAISE);
            RuntimeShip.ExecSystems(1.1);
            RuntimeShip.ExecSystems(0);
            LiveFalcon->Tick(0.01f);
            for (UFighterLandingGearComponent* Gear : LiveFalcon->LandingGearActors)
                TestFalse(TEXT("Bound runtime gear hides after retraction"), Gear->bGearVisible);
            TArray<UChildActorComponent*> BoundAssemblies;
            LiveFalcon->GetComponents(BoundAssemblies);
            int32 CheckedAssemblies = 0;
            for (UChildActorComponent* Assembly : BoundAssemblies)
            {
                if (!GetNameSafe(Assembly->GetChildActorClass()).Contains(TEXT("Gear"))) continue;
                ++CheckedAssemblies;
                TestFalse(TEXT("Saved Blueprint gear assembly retracts"), Assembly->IsVisible());
                if (AActor* Child = Assembly->GetChildActor())
                    TestTrue(TEXT("Saved Blueprint gear actor is hidden"), Child->IsHidden());
            }
            TestTrue(TEXT("Actual Falcon gear assemblies were checked"), CheckedAssemblies >= 3);
            LiveFalcon->BindRuntimeShip(nullptr);
            RuntimeShip.SetTestGear(nullptr);
            LiveFalcon->PreviewGearDeployment = 0;
            LiveFalcon->SynchronizeLandingGearMarkers();
            TArray<UStaticMeshComponent*> FalconMeshes;
            LiveFalcon->GetComponents(FalconMeshes);
            for (UStaticMeshComponent* Mesh : FalconMeshes)
            {
                if (Mesh->GetStaticMesh() && Mesh->GetStaticMesh()->GetPathName().Contains(TEXT("/LandingGear/")))
                {
                    AddInfo(FString::Printf(TEXT("Falcon gear mesh %s Parent=%s Visible=%d"),
                        *Mesh->GetName(), *GetNameSafe(Mesh->GetAttachParent()), Mesh->IsVisible()));
                    TestFalse(TEXT("All Falcon gear meshes retract, including saved BP components"), Mesh->IsVisible());
                }
            }
            for (UFighterLandingGearComponent* Gear : LiveFalcon->LandingGearActors)
            {
                TestFalse(TEXT("Falcon Blueprint retracts native gear"), Gear->bGearVisible);
                if (AActor* Child = Gear->GetChildActor())
                {
                    TArray<UStaticMeshComponent*> Meshes;
                    Child->GetComponents(Meshes);
                    for (UStaticMeshComponent* Mesh : Meshes)
                        TestFalse(TEXT("Falcon Blueprint gear mesh is hidden when up"), Mesh->IsVisible());
                }
            }
        }
    }
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
    // Keep the DEF barrel order and mirror its lateral axis for the imported hull.
    TestEqual(TEXT("Raptor first authored muzzle in reflected hull"), Cannon->GetMuzzleWorldLocation(0), FVector(42.5,4.25,-4.75));
    TestEqual(TEXT("Raptor second authored muzzle in reflected hull"), Cannon->GetMuzzleWorldLocation(1), FVector(42.5,-4.25,-4.75));
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
