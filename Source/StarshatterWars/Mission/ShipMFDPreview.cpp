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

FShipMFDPreview::FShipMFDPreview(bool bInPlayerPanel):bPlayerPanel(bInPlayerPanel){}
FShipMFDPreview::~FShipMFDPreview(){Clear();}
void FShipMFDPreview::Clear()
{
    if(Capture.IsValid())Capture->Destroy();
    if(Visual.IsValid())Visual->Destroy();
    Capture.Reset();Visual.Reset();Brush.SetResourceObject(nullptr);
    Material.Reset();Target.Reset();Scene.Reset();Key.Empty();bHasLiveView=false;
}
bool FShipMFDPreview::SetDesign(const FShipDesign* Design)
{
    if(!Design){Clear();return false;}
    const FString NextKey=Design->ShipName+Design->Mesh.ToSoftObjectPath().ToString();
    if(Key==NextKey && IsReady())
    {
        // Refresh at the HUD sampling rate after streaming/shader warmup.
        // Both captures stay in their own preview worlds.
        if(!GUsingNullRHI && Capture.IsValid() && !bHasLiveView)
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
    Target->ClearColor=bPlayerPanel ? FLinearColor(0,0,0,1) : FLinearColor::Black;
    Target->InitAutoFormat(bPlayerPanel ? 512 : 256,256);
    Target->UpdateResourceImmediate(true);
    FActorSpawnParameters Params;Params.ObjectFlags|=RF_Transient;
    Capture=World->SpawnActor<ASceneCapture2D>(Params);
    if(!Capture.IsValid()){Clear();return false;}
    const FVector CameraLocation=bPlayerPanel ? FVector(0.f,-350.f,180.f) : FVector(-290.f,-290.f,170.f);
    Capture->SetActorLocationAndRotation(CameraLocation,(-CameraLocation).Rotation());
    USceneCaptureComponent2D* Component=Capture->GetCaptureComponent2D();
    Component->TextureTarget=Target.Get();Component->FOVAngle=40.f;
    Component->CaptureSource=SCS_SceneColorHDR;
    if (bPlayerPanel)
    {
        // Fit the projected hull bounds to the entire 2:1 player image area.
        // Orthographic framing avoids perspective-dependent cropping.
        const FBox Bounds=BriefingShipPreview::CalculateBounds(Parts);
        const FQuat View=Capture->GetActorQuat();
        FVector2D Min(DBL_MAX,DBL_MAX),Max(-DBL_MAX,-DBL_MAX);
        for(int32 Corner=0;Corner<8;++Corner)
        {
            const FVector V=Bounds.GetCenter()+Bounds.GetExtent()*FVector((Corner&1)?1:-1,(Corner&2)?1:-1,(Corner&4)?1:-1);
            const FVector2D Projected(FVector::DotProduct(V,View.GetAxisY()),FVector::DotProduct(V,View.GetAxisZ()));
            Min.X=FMath::Min(Min.X,Projected.X);Min.Y=FMath::Min(Min.Y,Projected.Y);
            Max.X=FMath::Max(Max.X,Projected.X);Max.Y=FMath::Max(Max.Y,Projected.Y);
        }
        const FVector2D Extent=Max-Min;
        Component->ProjectionType=ECameraProjectionMode::Orthographic;
        Component->OrthoWidth=FMath::Max(Extent.X,Extent.Y*2.0)*1.08;
    }
    Component->bCaptureEveryFrame=false;Component->bCaptureOnMovement=false;
    World->UpdateWorldComponents(false,false);
    World->SendAllEndOfFrameUpdates();
    if(!GUsingNullRHI)Component->CaptureScene();
    UMaterialInterface* Base=LoadObject<UMaterialInterface>(nullptr,bPlayerPanel ? TEXT("/Game/UI/Materials/M_UIPlayerShipMFD.M_UIPlayerShipMFD") : TEXT("/Game/UI/Materials/M_UIShipMFD.M_UIShipMFD"));
    if(!Base){Clear();return false;}
    Material.Reset(UMaterialInstanceDynamic::Create(Base,GetTransientPackage()));
    Material->SetTextureParameterValue(TEXT("ShipTexture"),Target.Get());
    Brush.SetResourceObject(Material.Get());Brush.ImageSize=FVector2D(bPlayerPanel ? 512 : 256,256);
    Brush.DrawAs=ESlateBrushDrawType::Image;Key=NextKey;
    return true;
}

void FShipMFDPreview::SetView(const FQuat& TargetRotation,const FVector& ToObserver,const FVector& ObserverUp)
{
    if(!IsReady() || !Capture.IsValid() || TargetRotation.ContainsNaN() || ToObserver.ContainsNaN() || ToObserver.IsNearlyZero())return;
    bHasLiveView=true;
    Visual->SetActorRotation(TargetRotation.GetNormalized());
    const FVector Direction=ToObserver.GetSafeNormal();
    FVector Up=ObserverUp.GetSafeNormal();
    if(Up.IsNearlyZero() || FMath::Abs(FVector::DotProduct(Up,Direction))>0.99)
        Up=FMath::Abs(Direction.Z)<0.99 ? FVector::UpVector : FVector::RightVector;
    const FQuat CameraRotation=FRotationMatrix::MakeFromXZ(-Direction,Up).ToQuat();
    // The preview contains only copied mesh components, never the live ship.
    const FBox Bounds=Visual->GetComponentsBoundingBox(true);
    if(!Bounds.IsValid)return;
    const FVector Center=Bounds.GetCenter();
    double Width=0,Height=0;
    for(int32 Corner=0;Corner<8;++Corner)
    {
        const FVector V=Bounds.GetExtent()*FVector((Corner&1)?1:-1,(Corner&2)?1:-1,(Corner&4)?1:-1);
        Width=FMath::Max(Width,FMath::Abs(FVector::DotProduct(V,CameraRotation.GetAxisY()))*2.0);
        Height=FMath::Max(Height,FMath::Abs(FVector::DotProduct(V,CameraRotation.GetAxisZ()))*2.0);
    }
    Capture->SetActorLocationAndRotation(Center+Direction*400.f,CameraRotation);
    auto* Component=Capture->GetCaptureComponent2D();
    Component->ProjectionType=ECameraProjectionMode::Orthographic;
    Component->OrthoWidth=FMath::Max(Width,Height*2.0)*1.08;
    Scene->GetWorld()->UpdateWorldComponents(false,false);
    Scene->GetWorld()->SendAllEndOfFrameUpdates();
    if(!GUsingNullRHI)Component->CaptureScene();
}
