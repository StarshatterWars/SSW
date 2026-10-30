#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "ShipBriefingPreview.h"
#include "GameStructs_System.h"
#include "ShipActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBriefingPreviewFramingTest,
    "Starshatter.Phase4.PreviewFraming",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBriefingPreviewFramingTest::RunTest(const FString& Parameters)
{
    for (const FVector2D Size : { FVector2D(500,800), FVector2D(800,500), FVector2D(600,600), FVector2D(2560,720) })
    {
        const double Distance = BriefingShipPreview::FitDistance(100.f, Size);
        const double Aspect = Size.X / Size.Y;
        const double HalfAngle = FMath::Atan(FMath::Min(Aspect, 1. / Aspect));
        TestTrue(TEXT("The entire bounding sphere fits inside both viewport axes"), FMath::Asin(100. / Distance) < HalfAngle);
        TestTrue(TEXT("Camera stays outside the model and near plane"), Distance > 110.);
    }
    TestTrue(TEXT("Unmeasured layout has a finite camera distance"),
        FMath::IsFinite(BriefingShipPreview::FitDistance(100.f, FVector2D::ZeroVector)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBriefingPreviewIsolationTest,
    "Starshatter.Phase4.PreviewWorldIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBriefingPreviewIsolationTest::RunTest(const FString& Parameters)
{
    FShipDesign Design;
    Design.ShipName = TEXT("Preview cube fixture");
    Design.Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    TArray<FBriefingShipPreviewPart> Parts;
    if (!TestTrue(TEXT("Authored mesh resolves without any mission/simulation"), BriefingShipPreview::ResolveParts(Design, Parts))) return false;
    const FBox Bounds = BriefingShipPreview::CalculateBounds(Parts);
    TestTrue(TEXT("Mesh is centered for orbit"), Bounds.GetCenter().IsNearlyZero(0.01));
    TestTrue(TEXT("Mesh is normalized uniformly for fighter/capital size independence"), FMath::IsNearlyEqual(Bounds.GetExtent().Size(), 100., 0.01));
    const auto Options = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* MainWorld = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Options);
    TestTrue(TEXT("Factory refuses to spawn in the gameplay world"), BriefingShipPreview::CreateVisual(MainWorld, Parts) == nullptr);
    MainWorld->DestroyWorld(false);
    UWorld* PreviewWorld = UWorld::CreateWorld(EWorldType::EditorPreview, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Options);
    AActor* Visual = BriefingShipPreview::CreateVisual(PreviewWorld, Parts);
    if (TestNotNull(TEXT("Preview visual is created in an isolated world"), Visual))
    {
        TestTrue(TEXT("Visual is an inert AActor, not a gameplay ship"), Visual->GetClass() == AActor::StaticClass());
        TestFalse(TEXT("No actor tick"), Visual->IsActorTickEnabled());
        TestFalse(TEXT("No collision"), Visual->GetActorEnableCollision());
        TArray<UStaticMeshComponent*> Meshes;
        Visual->GetComponents(Meshes);
        TestEqual(TEXT("One authored mesh copied"), Meshes.Num(), 1);
        if (Meshes.Num()) TestTrue(TEXT("Mesh collision remains disabled"), Meshes[0]->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
        Visual->Destroy();
    }
    PreviewWorld->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBriefingPreviewAssetsTest,
    "Starshatter.Phase4.AuthoredShipPreviewAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBriefingPreviewAssetsTest::RunTest(const FString& Parameters)
{
    for (const TCHAR* Name : { TEXT("Stormhawk"), TEXT("Devastator") })
    {
        FShipDesign Design;
        Design.ShipName = Name;
        TArray<FBriefingShipPreviewPart> Parts;
        const bool Resolved = BriefingShipPreview::ResolveParts(Design, Parts);
        TestTrue(FString::Printf(TEXT("%s authored visual resolves"), Name), Resolved);
        if (Resolved)
        {
            TestTrue(TEXT("Ship has actual static mesh parts"), Parts.Num() > 0);
            const FBox Bounds = BriefingShipPreview::CalculateBounds(Parts);
            TestTrue(TEXT("Ship bounds are centered and normalized"), Bounds.GetCenter().IsNearlyZero(0.05) && FMath::IsNearlyEqual(Bounds.GetExtent().Size(), 100., 0.05));
        }
    }
    FShipDesign Missing;
    Missing.ShipName = TEXT("Phase4_Missing_Preview_Asset");
    TArray<FBriefingShipPreviewPart> Parts;
    TestFalse(TEXT("Missing design returns unavailable rather than inventing a ship"), BriefingShipPreview::ResolveParts(Missing, Parts));
    TestEqual(TEXT("Missing design clears previous preview parts"), Parts.Num(), 0);
    return true;
}
#endif
