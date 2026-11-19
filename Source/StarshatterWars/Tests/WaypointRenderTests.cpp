#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "WaypointActor.h"
#include "PreviewScene.h"
#include "Engine/World.h"
#include "Components/SceneCaptureComponent2D.h"
#include "ProceduralMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/SceneCapture2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#include "ContentStreaming.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
class FWaypointCaptureCommand : public IAutomationLatentCommand
{
public:
    explicit FWaypointCaptureCommand(FAutomationTestBase* InTest):Test(InTest)
    {
        Scene=MakeUnique<FPreviewScene>(FPreviewScene::ConstructionValues{}.SetEditor(false).SetCreatePhysicsScene(false));
        auto* World=Scene->GetWorld();
        Marker=World->SpawnActor<AWaypointActor>();
        Marker->WaypointIndex=1;Marker->SectorName=TEXT("Janek");Marker->bNextWaypoint=true;Marker->RefreshAppearance(true);
        Camera=World->SpawnActor<ASceneCapture2D>();
        Camera->SetActorLocation(FVector(-450,-550,200));Camera->SetActorRotation((-Camera->GetActorLocation()).Rotation());
        Target.Reset(NewObject<UTextureRenderTarget2D>());Target->ClearColor=FLinearColor::Black;
        Target->RenderTargetFormat=RTF_RGBA16f;Target->InitAutoFormat(512,512);Target->UpdateResourceImmediate();
        auto* Capture=Camera->GetCaptureComponent2D();Capture->TextureTarget=Target.Get();Capture->CaptureSource=SCS_SceneColorHDR;
        Capture->FOVAngle=35;Capture->bCaptureEveryFrame=false;Capture->bCaptureOnMovement=false;
        Capture->ShowFlags.SetSeparateTranslucency(true);Capture->ShowFlags.SetEyeAdaptation(false);
        FlushAsyncLoading();FAssetCompilingManager::Get().FinishAllCompilation();
        if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
        IStreamingManager::Get().StreamAllResources(0);FlushRenderingCommands();
    }
    bool Update() override
    {
        // A world Tick alone does not advance the renderer's global frame or deferred proxy caches.
        if(LastFrame==GFrameCounter)return false;
        LastFrame=GFrameCounter;++Frames;
        auto* World=Scene->GetWorld();World->Tick(LEVELTICK_All,.016f);World->SendAllEndOfFrameUpdates();
        if(Frames<3)return false;
        Camera->GetCaptureComponent2D()->CaptureScene();FlushRenderingCommands();
        if(Frames<6)return false;
        TArray<FLinearColor> Pixels;Target->GameThread_GetRenderTargetResource()->ReadLinearColorPixels(Pixels);
        int32 Green=0,BrightEdges=0;TArray<FColor> Output;Output.Reserve(Pixels.Num());
        for(const auto& Pixel:Pixels)
        {
            if(Pixel.G>.1 && Pixel.G>Pixel.R*1.3 && Pixel.G>Pixel.B*1.1)++Green;
            if(Pixel.G>3)++BrightEdges;
            Output.Add(Pixel.ToFColor(true));
        }
        Test->TestTrue(TEXT("Green plumbob is actually rendered"),Green>100);
        Test->TestTrue(TEXT("Edge traces are brighter than facet fill"),BrightEdges>25);
        Test->AddInfo(FString::Printf(TEXT("Marker green pixels=%d, bright edge pixels=%d"),Green,BrightEdges));
        const FString Folder=FPaths::ProjectSavedDir()/TEXT("Review");IFileManager::Get().MakeDirectory(*Folder,true);
        TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(512,512,Output,PNG);
        Test->TestTrue(TEXT("Waypoint preview saved"),FFileHelper::SaveArrayToFile(PNG,*(Folder/TEXT("waypoint.png"))));
        return true;
    }
private:
    FAutomationTestBase* Test;
    TUniquePtr<FPreviewScene> Scene;
    TStrongObjectPtr<UTextureRenderTarget2D> Target;
    AWaypointActor* Marker=nullptr;
    ASceneCapture2D* Camera=nullptr;
    uint64 LastFrame=MAX_uint64;
    int32 Frames=0;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaypointRenderedTest,"Starshatter.HUD.WaypointRendered",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWaypointRenderedTest::RunTest(const FString&)
{
    if(GUsingNullRHI){AddWarning(TEXT("Physical marker pixel check requires RHI; topology/collision checks run in PhysicalWaypoint."));return true;}
    ADD_LATENT_AUTOMATION_COMMAND(FWaypointCaptureCommand(this));return true;
}
#endif
