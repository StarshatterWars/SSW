#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "ScopedTestWorldContext.h"
#include "NativeTerrainActor.h"
#include "JanekRunwayClearanceComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Async/TaskGraphInterfaces.h"
#include "ProceduralMeshComponent.h"
#include "Terrain.h"
#include "TerrainRegion.h"
#include "StarSystem.h"
#include "OrbitalBody.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TerrainPatch.h"
#include "TerrainLayer.h"
#include "Engine/World.h"
#include "Components/DirectionalLightComponent.h"
namespace {
class FPresentationTerrain : public Terrain {
public:
    FPresentationTerrain():Terrain(nullptr){
        layers.append(new TerrainLayer);subdivisions=1;patch_size=16;scale=1;
        TArray<FColor> Colors;Colors.SetNum(17*17);for(int I=0;I<Colors.Num();++I)Colors[I]=FColor(uint8(I%17+I/17),0,0);
        Bitmap Image;Image.SetCPUImage(17,17,Colors.GetData(),Bitmap::BMP_SOLID,false);
        patches=new TerrainPatch*[1];patches[0]=new TerrainPatch(this,&Image,Rect(0,0,16,16),FVector::ZeroVector,FVector(16,2,16));
    }
};}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNativeTerrainPresentationTest,"Starshatter.Audit.F26.NativeTerrainPresentation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNativeTerrainPresentationTest::RunTest(const FString& Parameters)
{
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    FScopedSSWTestWorldContext TestWorldContext1(World);
    auto* Actor=World->SpawnActor<ANativeTerrainActor>();FPresentationTerrain Model;
    Actor->UpdateTerrain(&Model,FVector::ZeroVector,FVector(100,200,300));
    const auto* Section=Actor->Mesh->GetProcMeshSection(0);
    if(TestNotNull(TEXT("Native terrain creates a renderable mesh section"),Section))
    {
        TestEqual(TEXT("Terrain grid is bounded"),Section->ProcVertexBuffer.Num(),257*257);
        TestEqual(TEXT("Native center vertex uses the simulation ground height"),double(Section->ProcVertexBuffer[128*257+128].Position.Z),Model.Height(0,0));
        TestTrue(TEXT("Native section also supplies collision geometry"),Section->bEnableCollision);
        const FVector A=Section->ProcVertexBuffer[Section->ProcIndexBuffer[0]].Position,B=Section->ProcVertexBuffer[Section->ProcIndexBuffer[1]].Position,C=Section->ProcVertexBuffer[Section->ProcIndexBuffer[2]].Position;
        TestTrue(TEXT("Terrain uses clockwise winding for an upward Unreal front face"),FVector::CrossProduct(B-A,C-A).Z<0);
        TestTrue(TEXT("Terrain vertex normals point up"),Section->ProcVertexBuffer[0].Normal.Z>0);
    }
    TestTrue(TEXT("Terrain uses the mission region presentation offset"),Actor->GetActorLocation().Equals(FVector(100,200,300)));
    Actor->UpdateTerrain(nullptr,FVector::ZeroVector,FVector::ZeroVector);
    TestTrue(TEXT("Leaving terrain hides the native surface"),Actor->IsHidden());
    Actor->SetSurfaceSky(true, false);
    Actor->UpdateTerrain(nullptr,FVector::ZeroVector,FVector::ZeroVector);
    TestFalse(TEXT("WorldScape surface retains the sunlight actor without a fallback mesh"), Actor->IsHidden());
    TestFalse(TEXT("WorldScape does not duplicate fallback terrain"), Actor->Mesh->IsVisible());
    auto* Sun=Actor->FindComponentByClass<UDirectionalLightComponent>();
    if(TestNotNull(TEXT("WorldScape surface has a directional sun"),Sun))
        TestTrue(TEXT("Surface sunlight is enabled"),Sun->IsVisible() && Sun->Intensity>0);
    Actor->SetSurfaceSky(false);
    Actor->UpdateTerrain(nullptr,FVector::ZeroVector,FVector::ZeroVector);
    TestTrue(TEXT("Surface sunlight actor is hidden after returning to space"), Actor->IsHidden());
    World->DestroyWorld(false);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTerrainCatalogJsonTest,"Starshatter.Data.Terrain.JsonGroundStations",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTerrainCatalogJsonTest::RunTest(const FString&)
{
    const FString Path=FPaths::ProjectContentDir()/TEXT("GameData/Galaxy/terrain_regions.json");FString Json;TSharedPtr<FJsonObject> Root;
    if(!FFileHelper::LoadFileToString(Json,*Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root))return false;
    const auto& Rows=Root->GetArrayField(TEXT("regions"));TestEqual(TEXT("All authored ground regions converted"),Rows.Num(),9);
    for(const auto& V:Rows)
    {
        const auto Row=V->AsObject();const FString Name=Row->GetStringField(TEXT("name"));
        StarSystem System(TCHAR_TO_UTF8(*Row->GetStringField(TEXT("system"))),FVector::ZeroVector);
        auto* Planet=new OrbitalBody(&System,TCHAR_TO_UTF8(*Row->GetStringField(TEXT("parent"))),Orbital::PLANET,6e24,5.4e6,1e11);
        System.AddBody(Planet);
        auto* Region=System.LoadTerrainRegion(Path,TCHAR_TO_UTF8(*Name),Planet);
        if(!TestNotNull(*Name,Region))continue;
        TestEqual(TEXT("JSON creates a native surface region"),Region->GetType(),int(Orbital::TERRAIN));
        auto* Surface=static_cast<TerrainRegion*>(Region);
        TestEqual(TEXT("All authored terrain layers survive"),Surface->GetLayers().size(),3);
        if(Name==TEXT("Janek Station"))
        {
            Terrain Ground(Surface);
            const FVector Station=TerrainRegion::JanekStationLocalPosition();
            for(const FVector2D Offset : {FVector2D(0,0),FVector2D(2500,0),FVector2D(-2500,2000),FVector2D(0,-3000),FVector2D(7500,7500),FVector2D(-8000,-8000)})
                TestEqual(TEXT("Runway and apron collision remain below the pavement"),Ground.Height(Station.X+Offset.X,Station.Z+Offset.Y),-2.0);
        }
        TestFalse(TEXT("Ground material is data-driven"),Surface->SurfaceMaterial().IsEmpty());
        TestTrue(TEXT("Surface sunlight is data-driven"),Surface->SurfaceSunIntensity()>0);
        TestFalse(TEXT("Biome profile is retained for the weather adapter"),Surface->BiomeProfile().IsEmpty());
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJanekFoliageClearanceTest,"Starshatter.Terrain.JanekFoliageClearance",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FJanekFoliageClearanceTest::RunTest(const FString&)
{
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    FScopedSSWTestWorldContext Context(World);ON_SCOPE_EXIT{World->DestroyWorld(false);};
    auto* Owner=World->SpawnActor<AActor>();
    auto* Mesh=NewObject<UInstancedStaticMeshComponent>(Owner);Owner->AddInstanceComponent(Mesh);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Mesh->RegisterComponent();
    Mesh->AddInstance(FTransform(FQuat::Identity,FVector(0,0,500)),true);
    Mesh->AddInstance(FTransform(FQuat::Identity,FVector(13000,0,500),FVector(100)),true);
    Mesh->AddInstance(FTransform(FQuat::Identity,FVector(40000,0,500)),true);
    auto* Guard=NewObject<UJanekRunwayClearanceComponent>(Owner);Owner->AddInstanceComponent(Guard);Guard->RegisterComponent();
    Guard->Protect(FVector::ZeroVector,12000);
    FTransform Pose;Mesh->GetInstanceTransform(0,Pose,true);TestTrue(TEXT("Rock on runway suppressed"),Pose.GetScale3D().IsNearlyZero());
    Mesh->GetInstanceTransform(1,Pose,true);TestTrue(TEXT("Rock with origin outside mask but overlapping bounds suppressed"),Pose.GetScale3D().IsNearlyZero());
    Mesh->GetInstanceTransform(2,Pose,true);TestFalse(TEXT("Distant scenery preserved"),Pose.GetScale3D().IsNearlyZero());
    TestEqual(TEXT("Generator indices remain stable"),Mesh->GetInstanceCount(),3);
    Mesh->AddInstance(FTransform(FQuat::Identity,FVector(100,0,1000)),true);
    FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
    Mesh->GetInstanceTransform(3,Pose,true);TestTrue(TEXT("Late asynchronous foliage cannot return over runway"),Pose.GetScale3D().IsNearlyZero());
    Guard->StopProtection();
    Mesh->AddInstance(FTransform(FQuat::Identity,FVector(0,0,500)),true);
    FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
    Mesh->GetInstanceTransform(4,Pose,true);TestFalse(TEXT("Surface exit releases local protection"),Pose.GetScale3D().IsNearlyZero());
    return true;
}
#endif
