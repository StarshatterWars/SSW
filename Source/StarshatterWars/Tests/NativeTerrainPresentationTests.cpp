#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "NativeTerrainActor.h"
#include "ProceduralMeshComponent.h"
#include "Terrain.h"
#include "TerrainPatch.h"
#include "TerrainLayer.h"
#include "Engine/World.h"
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
    auto* Actor=World->SpawnActor<ANativeTerrainActor>();FPresentationTerrain Model;
    Actor->UpdateTerrain(&Model,FVector::ZeroVector,FVector(100,200,300));
    const auto* Section=Actor->Mesh->GetProcMeshSection(0);
    if(TestNotNull(TEXT("Native terrain creates a renderable mesh section"),Section))
    {
        TestEqual(TEXT("Terrain grid is bounded"),Section->ProcVertexBuffer.Num(),257*257);
        TestEqual(TEXT("Native center vertex uses the simulation ground height"),double(Section->ProcVertexBuffer[128*257+128].Position.Z),Model.Height(0,0));
        TestTrue(TEXT("Native section also supplies collision geometry"),Section->bEnableCollision);
        const FVector A=Section->ProcVertexBuffer[Section->ProcIndexBuffer[0]].Position,B=Section->ProcVertexBuffer[Section->ProcIndexBuffer[1]].Position,C=Section->ProcVertexBuffer[Section->ProcIndexBuffer[2]].Position;
        TestTrue(TEXT("Terrain triangle faces upward"),FVector::CrossProduct(B-A,C-A).Z>0);
    }
    TestTrue(TEXT("Terrain uses the mission region presentation offset"),Actor->GetActorLocation().Equals(FVector(100,200,300)));
    Actor->UpdateTerrain(nullptr,FVector::ZeroVector,FVector::ZeroVector);
    TestTrue(TEXT("Leaving terrain hides the native surface"),Actor->IsHidden());
    World->DestroyWorld(false);return true;
}
#endif
