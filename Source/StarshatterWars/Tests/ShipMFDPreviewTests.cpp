#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "ShipMFDPreview.h"
#include "GameStructs_System.h"
#include "Engine/StaticMesh.h"
#include "Engine/DataTable.h"
#include "Sim.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RenderingThread.h"
#include "RHI.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShipMFDisolationTest,"Starshatter.MFD.PreviewIsolationAndTargetReplacement",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShipMFDisolationTest::RunTest(const FString& Parameters)
{
    Sim* Before=Sim::GetSim();
    FShipMFDPreview Player(true),Target(true);
    auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
    if(!TestNotNull(TEXT("Ship design table"),Table))return false;
    for(const TCHAR* Name:{TEXT("Falcon"),TEXT("Thunderbolt"),TEXT("Eagle"),TEXT("Orion")})
    {
        const auto* Design=Table->FindRow<FShipDesign>(FName(Name),TEXT("MFD tests"));
        if(!TestNotNull(Name,Design))continue;
        TestTrue(TEXT("Actual player model creates an isolated 3D preview"),Player.SetDesign(Design));
        TestTrue(TEXT("Target replacement resolves the actual model"),Target.SetDesign(Design));
        TestTrue(TEXT("Target capture is ready"),Target.IsReady());
        TestTrue(TEXT("Repeated samples reuse the existing preview"),Target.SetDesign(Design));
        TestTrue(TEXT("Preview does not create or replace gameplay Sim"),Sim::GetSim()==Before);
    }
    TestFalse(TEXT("Lost target clears its preview"),Target.SetDesign(nullptr));
    TestFalse(TEXT("Lost target retains no stale model"),Target.IsReady());
    TestTrue(TEXT("Clearing target preserves player preview"),Player.IsReady());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerMFDRenderTest,"Starshatter.MFD.PlayerTransparentPanel",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPlayerMFDRenderTest::RunTest(const FString& Parameters)
{
    auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
    const auto* Design=Table?Table->FindRow<FShipDesign>(TEXT("Falcon"),TEXT("Player MFD render")):nullptr;
    if(!TestNotNull(TEXT("Falcon design"),Design))return false;
    FShipMFDPreview Player(true),Target(true);
    if(!TestTrue(TEXT("Player preview created"),Player.SetDesign(Design)))return false;
    TestTrue(TEXT("Player uses full-width capture"),Player.GetBrush().ImageSize.Equals(FVector2D(512,256)));
    auto* Material=Cast<UMaterialInstanceDynamic>(Player.GetBrush().GetResourceObject());
    TestTrue(TEXT("Player UI blends transparent background"),Material && Material->GetBlendMode()==BLEND_Translucent);
    TestTrue(TEXT("Target remains available"),Target.SetDesign(Design));
    TestTrue(TEXT("Target uses full panel dimensions"),Target.GetBrush().ImageSize.Equals(FVector2D(512,256)));
    auto* TargetMaterial=Cast<UMaterialInstanceDynamic>(Target.GetBrush().GetResourceObject());
    TestTrue(TEXT("Target uses transparent UI material"),TargetMaterial && TargetMaterial->GetBlendMode()==BLEND_Translucent);
    if(GUsingNullRHI){AddWarning(TEXT("No GPU: rendered pixel verification skipped"));return true;}
    FlushRenderingCommands();
    TArray<FLinearColor> Pixels;
    Player.Target->GameThread_GetRenderTargetResource()->ReadLinearColorPixels(Pixels);
    int32 Hull=0,Empty=0,MinX=512,MinY=256,MaxX=-1,MaxY=-1;
    TArray<FColor> Output;Output.Reserve(Pixels.Num());
    for(int32 I=0;I<Pixels.Num();++I)
    {
        const auto& Pixel=Pixels[I];const int32 X=I%512,Y=I/512;
        if(Pixel.A<0.5f){++Hull;MinX=FMath::Min(MinX,X);MaxX=FMath::Max(MaxX,X);MinY=FMath::Min(MinY,Y);MaxY=FMath::Max(MaxY,Y);}
        else ++Empty;
        FLinearColor Display=Pixel;Display.A=1.f-Pixel.A;Output.Add(Display.ToFColor(true));
    }
    TestTrue(TEXT("Capture contains visible ship geometry"),Hull>1000);
    TestTrue(TEXT("Capture retains transparent background"),Empty>1000);
    const float Fill=FMath::Max(float(MaxX-MinX+1)/512.f,float(MaxY-MinY+1)/256.f);
    TestTrue(TEXT("Hull fills the panel without clipping"),Fill>0.8f && MinX>0 && MinY>0 && MaxX<511 && MaxY<255);
    AddInfo(FString::Printf(TEXT("Player MFD: hull=%d background=%d bounds=(%d,%d)-(%d,%d) fill=%.3f"),Hull,Empty,MinX,MinY,MaxX,MaxY,Fill));
    TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(512,256,Output,PNG);
    TestTrue(TEXT("Preview image exported"),FFileHelper::SaveArrayToFile(PNG,*(FPaths::ProjectSavedDir()/TEXT("Automation/PlayerMFDFalcon.png"))));
    return true;
}
#endif
