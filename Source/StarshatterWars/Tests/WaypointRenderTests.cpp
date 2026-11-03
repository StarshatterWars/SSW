#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "WaypointActor.h"
#include "PreviewScene.h"
#include "Engine/World.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/SceneCapture2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#include "ContentStreaming.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaypointRenderedTest,"Starshatter.HUD.WaypointRendered",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWaypointRenderedTest::RunTest(const FString&){
 if(GUsingNullRHI){AddWarning(TEXT("Physical marker pixel check requires RHI; topology/collision checks run in PhysicalWaypoint."));return true;}
 FPreviewScene Preview(FPreviewScene::ConstructionValues{});UWorld* World=Preview.GetWorld();auto* Marker=World->SpawnActor<AWaypointActor>();Marker->RegisterAllComponents();Marker->OnConstruction(FTransform::Identity);Marker->WaypointIndex=1;Marker->SectorName=TEXT("Janek");Marker->bNextWaypoint=true;Marker->SetActorHiddenInGame(false);Marker->RefreshAppearance(true);
 auto* Camera=World->SpawnActor<ASceneCapture2D>();Camera->RegisterAllComponents();Camera->SetActorLocation(FVector(-450,-550,200));Camera->SetActorRotation((-Camera->GetActorLocation()).Rotation());
 auto* RT=NewObject<UTextureRenderTarget2D>();RT->ClearColor=FLinearColor::Black;RT->InitCustomFormat(512,512,PF_B8G8R8A8,false);RT->UpdateResourceImmediate();
 auto* Capture=Camera->GetCaptureComponent2D();Capture->TextureTarget=RT;Capture->CaptureSource=ESceneCaptureSource::SCS_FinalColorLDR;Capture->FOVAngle=35;Capture->bCaptureEveryFrame=false;Capture->bCaptureOnMovement=false;Capture->ShowOnlyActors.Add(Marker);Capture->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
 Capture->PostProcessSettings.bOverride_AutoExposureMinBrightness=true;Capture->PostProcessSettings.bOverride_AutoExposureMaxBrightness=true;Capture->PostProcessSettings.AutoExposureMinBrightness=1;Capture->PostProcessSettings.AutoExposureMaxBrightness=1;
 FlushAsyncLoading();FAssetCompilingManager::Get().FinishAllCompilation();if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();IStreamingManager::Get().StreamAllResources(0);World->UpdateWorldComponents(true,false);World->Tick(LEVELTICK_All,.016f);World->SendAllEndOfFrameUpdates();FlushRenderingCommands();Capture->CaptureScene();FlushRenderingCommands();Capture->CaptureScene();FlushRenderingCommands();
 TArray<FColor> Pixels;RT->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);int Green=0;for(const auto& P:Pixels)if(P.G>80&&P.G>P.R*1.3&&P.G>P.B*1.1)++Green;TestTrue(TEXT("Green plumbob is actually rendered"),Green>100);
 const FString File=FPaths::ProjectSavedDir()/TEXT("Review/waypoint.png");IFileManager::Get().MakeDirectory(*FPaths::GetPath(File),true);TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(512,512,Pixels,PNG);TestTrue(TEXT("Waypoint preview saved"),FFileHelper::SaveArrayToFile(PNG,*File));return true;
}
#endif
