#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "WeaponBlueprintResolver.h"
#include "MissileFlightPresentation.h"
#include "MissileActor.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponBlueprintResolverTest, "Starshatter.Weapons.BlueprintModels",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWeaponBlueprintResolverTest::RunTest(const FString& Parameters)
{
    UClass* RocketClass = LoadClass<AMissileActor>(nullptr,TEXT("/Script/StarshatterWars.RocketMissileActor"));
    TestNotNull(TEXT("Rocket native missile actor exists for its Blueprint"),RocketClass);
    UClass* ReconClass = LoadClass<AMissileActor>(nullptr,TEXT("/Script/StarshatterWars.ReconPodActor"));
    if (TestNotNull(TEXT("Recon pod native actor exists for its Blueprint"),ReconClass))
        TestEqual(TEXT("Recon pod directly inherits MissileActor"),ReconClass->GetSuperClass(),AMissileActor::StaticClass());
    const auto Options = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World = UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    auto* Fighter = World->SpawnActor<AActor>();
    auto* Root = NewObject<USceneComponent>(Fighter);
    Fighter->SetRootComponent(Root);
    Root->RegisterComponent();
    for (const TCHAR* Name : {TEXT("Archer"), TEXT("Hammer"), TEXT("Javelin"), TEXT("Torpedo"), TEXT("Vector"), TEXT("Harpoon")})
    {
        UClass* Class = WeaponBlueprintResolver::Resolve(Name, TEXT("Unused"));
        TestNotNull(*FString::Printf(TEXT("%s model resolves to Blueprint"), Name), Class);
        TestTrue(TEXT("Missile Blueprint uses native missile actor"),Class && Class->IsChildOf(AMissileActor::StaticClass()));
        UClass* ExpectedParent = LoadClass<AMissileActor>(nullptr,
            *FString::Printf(TEXT("/Script/StarshatterWars.%sMissileActor"),Name));
        TestNotNull(TEXT("Missile-specific native actor exists"),ExpectedParent);
        if (Class) TestEqual(*FString::Printf(TEXT("%s Blueprint has matching native parent"),Name),Class->GetSuperClass(),ExpectedParent);
        if (Class)
        {
            AActor* Missile = World->SpawnActor<AActor>(Class);
            if (TestNotNull(TEXT("Existing missile actor spawns"), Missile))
            {
                TArray<UStaticMeshComponent*> Meshes;
                Missile->GetComponents(Meshes);
                int32 AssignedMeshes = 0;
                for (const auto* Mesh : Meshes) if (Mesh->GetStaticMesh()) ++AssignedMeshes;
                TestTrue(*FString::Printf(TEXT("%s retains its assigned mesh"),Name),AssignedMeshes > 0);
                Missile->AttachToActor(Fighter,FAttachmentTransformRules::KeepWorldTransform);
                Missile->SetOwner(Fighter);
                const FTransform LaunchTransform = Missile->GetActorTransform();
                if (auto* NativeMissile = Cast<AMissileActor>(Missile))
                {
                    TestFalse(TEXT("Carried actor starts out of flight"),NativeMissile->bInFlight);
                    TestNotNull(TEXT("Missile inherits trail component"),NativeMissile->TrailComponent.Get());
                    TestNotNull(TEXT("Missile trail has exhaust system"),NativeMissile->TrailComponent->GetAsset());
                    TestFalse(TEXT("Mounted missile trail stays off"),NativeMissile->TrailComponent->IsVisible());
                    TestFalse(TEXT("Mounted missile trail stays inactive"),NativeMissile->TrailComponent->IsActive());
                    NativeMissile->BeginSimulationFlight();
                    TestTrue(TEXT("Released actor enters flight"),NativeMissile->bInFlight);
                    TestTrue(TEXT("Launched missile trail is visible"),NativeMissile->TrailComponent->IsVisible());
                }
                TestNull(TEXT("Fired missile has no attachment parent"),Missile->GetAttachParentActor());
                TestNull(TEXT("Fired missile has no fighter owner"),Missile->GetOwner());
                TestTrue(TEXT("Release preserves launch transform"),Missile->GetActorTransform().Equals(LaunchTransform));
                TestTrue(TEXT("Used missile can be destroyed independently"),Missile->Destroy());
                TestFalse(TEXT("Destroying missile preserves fighter"),Fighter->IsActorBeingDestroyed());
            }
        }
        TestEqual(TEXT("Weapon name fallback resolves the same asset"), WeaponBlueprintResolver::Resolve(TEXT("MissingLegacyModel"), Name), Class);
    }
    TestNull(TEXT("Unknown model retains presentation fallback"), WeaponBlueprintResolver::Resolve(TEXT("UnknownMissile"), TEXT("UnknownWeapon")));
    World->DestroyWorld(false);
    return true;
}
#endif
