#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "CarrierInteriorBuilderComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCarrierInteriorBlockoutTest, "Starshatter.Carrier.Interior.Blockout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCarrierInteriorBlockoutTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues WorldOptions = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false, NAME_None, nullptr, true,
        ERHIFeatureLevel::Num, &WorldOptions);
    if (!TestNotNull(TEXT("Preview world"), World)) return false;
    AActor* Owner = World->SpawnActor<AActor>();
    UCarrierInteriorBuilderComponent* Builder = NewObject<UCarrierInteriorBuilderComponent>(Owner);
    Owner->SetRootComponent(Builder);
    Builder->RegisterComponent();
    Builder->LoadOrionStarterLayout();
    TestEqual(TEXT("Exactly four Orion starter bays"), Builder->Bays.Num(), 4);
    TestEqual(TEXT("Preview disabled by default"), Builder->GetGeneratedPieceCount(), 0);
    TestEqual(TEXT("Port launch lane"), Builder->Bays[0].FloorTransform.GetLocation(), FVector(650,60,-20));
    TestEqual(TEXT("Starboard recovery lane"), Builder->Bays[3].FloorTransform.GetLocation(), FVector(-650,-305,-12));
    Builder->bBuildInterior = true;
    Builder->RebuildInterior();
    TestEqual(TEXT("Floor, ceiling, sides and two framed openings per bay"), Builder->GetGeneratedPieceCount(), 40);
    Builder->RebuildInterior();
    TArray<UStaticMeshComponent*> Pieces;
    Owner->GetComponents(Pieces);
    TestEqual(TEXT("Repeated rebuild creates no duplicates"), Pieces.Num(), 40);
    bool FoundFloor = false;
    Owner->SetActorScale3D(FVector(4));
    for (UStaticMeshComponent* Piece : Pieces)
    {
        if (Piece->GetName().Contains(TEXT("Deck_1_LaunchHangar_Floor")))
        {
            FoundFloor = true;
            const FBox Box = Piece->GetStaticMesh()->GetBoundingBox().TransformBy(Piece->GetComponentTransform());
            TestTrue(TEXT("Floor top includes Orion scale once"), FMath::IsNearlyEqual(Box.Max.Z, -80.0, 0.01));
            TestTrue(TEXT("Unscaled floor length 400 becomes 1600"), FMath::IsNearlyEqual(Box.GetSize().X,1600.0,0.01));
            TestEqual(TEXT("Collision is opt-in"), Piece->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
        }
    }
    TestTrue(TEXT("Generated launch floor located"), FoundFloor);
    Builder->bShowCeilings = false;
    Builder->RebuildInterior();
    TestEqual(TEXT("Ceiling cutaway removes four pieces"), Builder->GetGeneratedPieceCount(), 36);
    FCarrierInteriorModule Module;
    Module.Name = TEXT("Maintenance_Platform");
    Builder->Modules.Add(Module);
    Builder->RebuildInterior();
    TestEqual(TEXT("Custom module supported"), Builder->GetGeneratedPieceCount(), 37);
    Builder->ClearInterior();
    Owner->GetComponents(Pieces);
    TestEqual(TEXT("Clear removes only generated static meshes"), Pieces.Num(), 0);
    TestFalse(TEXT("Clear disables generation"), Builder->bBuildInterior);
    Builder->RebuildInterior();
    TestEqual(TEXT("Disabled preview remains empty"), Builder->GetGeneratedPieceCount(), 0);
    Builder->LoadOrionFullInteriorLayout();
    TestEqual(TEXT("Full preset keeps four flight bays"), Builder->Bays.Num(), 4);
    TestEqual(TEXT("Full preset adds twelve room/connection anchors and retains custom module"), Builder->Modules.Num(), 13);
    Builder->LoadOrionFullInteriorLayout();
    TestEqual(TEXT("Repeated full preset does not duplicate anchors"), Builder->Modules.Num(), 13);
    World->DestroyWorld(false);
    return true;
}
#endif
