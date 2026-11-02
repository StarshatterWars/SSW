#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "MissileActor.h"
#include "MissileFlightPresentation.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissileForwardAlignmentTest,"Starshatter.Weapons.MissileForwardAlignment",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMissileForwardAlignmentTest::RunTest(const FString&)
{
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    if(!TestNotNull(TEXT("Preview world"),World))return false;
    ON_SCOPE_EXIT{World->DestroyWorld(false);};
    for(const TCHAR* Name:{TEXT("Archer"),TEXT("Harpoon"),TEXT("Javelin"),TEXT("Vector"),TEXT("Hammer"),TEXT("Torpedo"),TEXT("Rocket")})
    {
        const FString Path=FString::Printf(TEXT("/Game/Models/Weapons/%s/BP_%s.BP_%s_C"),Name,Name,Name);
        UClass* Class=LoadClass<AMissileActor>(nullptr,*Path);
        if(!TestNotNull(*(FString(Name)+TEXT(" native missile Blueprint")),Class))continue;
        const FTransform Pose(FRotator(13,47,-9),FVector(110,220,330),FVector(.27));
        auto* Missile=World->SpawnActor<AMissileActor>(Class,Pose);
        if(!TestNotNull(TEXT("Saved missile spawns"),Missile))continue;
        TArray<UStaticMeshComponent*> Meshes;Missile->GetComponents(Meshes);int32 Bodies=0;
        for(auto* Mesh:Meshes)if(Mesh->GetStaticMesh()){
            ++Bodies;
            TestTrue(*(FString(Name)+TEXT(" nose agrees with actor forward")),Mesh->GetComponentTransform().TransformVector(FVector::RightVector).GetSafeNormal().Equals(Missile->GetActorForwardVector(),.0001));
            TestTrue(TEXT("Authored centre remains at attachment origin"),Mesh->GetRelativeLocation().IsNearlyZero(.001));
            const FTransform Before=Mesh->GetRelativeTransform();
            Missile->OnConstruction(Missile->GetActorTransform());Missile->OnConstruction(Missile->GetActorTransform());
            TestTrue(TEXT("Repeated construction does not compound -90 rotation"),Mesh->GetRelativeTransform().Equals(Before,.001));
        }
        TestTrue(TEXT("Actual body mesh checked"),Bodies>0);
        TestTrue(TEXT("Visual correction preserves actor pose and scale"),Missile->GetActorTransform().Equals(Pose,.001));
        TestTrue(TEXT("Exhaust remains behind missile centre"),Missile->TrailComponent->GetRelativeLocation().X<0);
        Missile->BeginSimulationFlight();
        TestTrue(TEXT("Release preserves launch pose"),Missile->GetActorTransform().Equals(Pose,.001));
        TestNull(TEXT("Fired missile is detached"),Missile->GetAttachParentActor());
        Missile->Destroy();
    }
    return true;
}
#endif
