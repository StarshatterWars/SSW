#include "ShipMFDPreview.h"
#include "ShipBriefingPreview.h"
#include "GameStructs_System.h"
#include "PreviewScene.h"
#include "Engine/World.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "RHI.h"

FShipMFDPreview::FShipMFDPreview()=default;
FShipMFDPreview::~FShipMFDPreview(){Clear();}
void FShipMFDPreview::Clear()
{
    if(Capture.IsValid())Capture->Destroy();
    if(Visual.IsValid())Visual->Destroy();
    Capture.Reset();Visual.Reset();Brush.SetResourceObject(nullptr);
    Material.Reset();Target.Reset();Scene.Reset();Key.Empty();
}
bool FShipMFDPreview::SetDesign(const FShipDesign* Design)
{
    if(!Design){Clear();return false;}
    const FString NextKey=Design->ShipName+Design->Mesh.ToSoftObjectPath().ToString();
    if(Key==NextKey && IsReady())
    {
        // Refresh at the HUD sampling rate after streaming/shader warmup.
        // Both captures stay in their own preview worlds.
        if(!GUsingNullRHI && Capture.IsValid())
        {
            Scene->GetWorld()->SendAllEndOfFrameUpdates();
            Capture->GetCaptureComponent2D()->CaptureScene();
        }
        return true;
    }
    Clear();
    TArray<FBriefingShipPreviewPart> Parts;
    if(!BriefingShipPreview::ResolveParts(*Design,Parts))return false;
    Scene=MakeUnique<FPreviewScene>(FPreviewScene::ConstructionValues().SetEditor(false)
        .SetCreatePhysicsScene(false).SetLightBrightness(4.f).SetSkyBrightness(0.f));
    UWorld* World=Scene->GetWorld();
    Visual=BriefingShipPreview::CreateVisual(World,Parts);
    if(!Visual.IsValid()){Clear();return false;}
    Target.Reset(NewObject<UTextureRenderTarget2D>(GetTransientPackage(),NAME_None,RF_Transient));
    Target->RenderTargetFormat=RTF_RGBA16f;
    Target->ClearColor=FLinearColor::Black;
    Target->InitAutoFormat(256,256);
    Target->UpdateResourceImmediate(true);
    FActorSpawnParameters Params;Params.ObjectFlags|=RF_Transient;
    Capture=World->SpawnActor<ASceneCapture2D>(Params);
    if(!Capture.IsValid()){Clear();return false;}
    const FVector CameraLocation(-290.f,-290.f,170.f);
    Capture->SetActorLocationAndRotation(CameraLocation,(-CameraLocation).Rotation());
    USceneCaptureComponent2D* Component=Capture->GetCaptureComponent2D();
    Component->TextureTarget=Target.Get();Component->FOVAngle=40.f;
    Component->CaptureSource=SCS_SceneColorHDR;
    Component->bCaptureEveryFrame=false;Component->bCaptureOnMovement=false;
    World->UpdateWorldComponents(false,false);
    World->SendAllEndOfFrameUpdates();
    if(!GUsingNullRHI)Component->CaptureScene();
    UMaterialInterface* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/UI/Materials/M_UIShipMFD.M_UIShipMFD"));
    if(!Base){Clear();return false;}
    Material.Reset(UMaterialInstanceDynamic::Create(Base,GetTransientPackage()));
    Material->SetTextureParameterValue(TEXT("ShipTexture"),Target.Get());
    Brush.SetResourceObject(Material.Get());Brush.ImageSize=FVector2D(256,256);
    Brush.DrawAs=ESlateBrushDrawType::Image;Key=NextKey;
    return true;
}
