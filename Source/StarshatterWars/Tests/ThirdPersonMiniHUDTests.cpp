#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "FighterHUDDetails.h"
#include "FighterHUDPanels.h"
#include "Widgets/SOverlay.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "AssetCompilingManager.h"
#include "ContentStreaming.h"
#include "ShaderCompiler.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FThirdPersonMiniHUDTest,"Starshatter.HUD.ThirdPersonMiniHUD",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FThirdPersonMiniHUDTest::RunTest(const FString&)
{
    if(GUsingNullRHI){AddWarning(TEXT("Render comparison requires an RHI."));return true;}
    bool ThirdPerson=false;
    auto Details=SNew(SFighterHUDDetails).ShowHUD(true).ThirdPerson_Lambda([&]{return ThirdPerson;});
    Details->NextSample=DBL_MAX;Details->bHasPlayer=true;Details->PlayerName=TEXT("Delta 1");Details->PlayerClass=TEXT("Falcon");
    Details->Primary=TEXT("GUNS: DELTA");Details->Secondary=TEXT("SRM 4");Details->FlightSpeed=500;Details->HeadingDegrees=90;
    Details->NavReadouts={TEXT("AUTO NAV 1"),TEXT("VECTOR SPD 500")};Details->CompactNavLabel=TEXT("NAV 01  JANEK");Details->StatusRows={{TEXT("THRUST"),50},{TEXT("FUEL"),100},{TEXT("HULL"),100},{TEXT("GUNS"),99}};
    auto Root=SNew(SOverlay)
        +SOverlay::Slot()[SNew(SFighterHUDPanels).ShowPanels_Lambda([&]{return !ThirdPerson;})]
        +SOverlay::Slot()[Details];
    FlushAsyncLoading();FAssetCompilingManager::Get().FinishAllCompilation();
    if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
    IStreamingManager::Get().StreamAllResources(0);FlushRenderingCommands();
    FWidgetRenderer* Renderer=new FWidgetRenderer(true);
    TArray<FColor> Frames[2];
    FString Capture;FParse::Value(FCommandLine::Get(),TEXT("MiniHUDCapture="),Capture);
    for(int32 I=0;I<2;++I){
        ThirdPerson=I==1;
        TStrongObjectPtr<UTextureRenderTarget2D> RT(FWidgetRenderer::CreateTargetFor(FVector2D(1280,720),TF_Bilinear,true));RT->ClearColor=FLinearColor::Transparent;
        Renderer->DrawWidget(RT.Get(),Root,FVector2D(1280,720),0);FlushRenderingCommands();
        FReadSurfaceDataFlags Flags;Flags.SetLinearToGamma(false);RT->GameThread_GetRenderTargetResource()->ReadPixels(Frames[I],Flags);
        if(!Capture.IsEmpty()){TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(1280,720,Frames[I],PNG);TestTrue(TEXT("HUD preview saved"),FFileHelper::SaveArrayToFile(PNG,*(Capture+(I?TEXT("thirdperson.png"):TEXT("full.png")))));}
    }
    int32 FullCentre=0,MiniCentre=0,MiniReadout=0,BottomDifferences=0;
    for(int32 Y=0;Y<720;++Y)for(int32 X=0;X<1280;++X){const int32 I=Y*1280+X;
        if(X>380 && X<900 && Y>180 && Y<490){if(Frames[0][I].A>10)++FullCentre;if(Frames[1][I].A>10)++MiniCentre;}
        if(X>350 && X<930 && Y<180 && Frames[1][I].A>10)++MiniReadout;
        if(Y>=505 && Frames[0][I]!=Frames[1][I])++BottomDifferences;
    }
    TestTrue(TEXT("Original central cockpit/virtual HUD is retained"),FullCentre>1000);
    TestEqual(TEXT("External view clears large central HUD"),MiniCentre,0);
    TestTrue(TEXT("External mini flight deck is visible at top center"),MiniReadout>100);
    TestEqual(TEXT("Bottom panels remain pixel-identical"),BottomDifferences,0);
    BeginCleanup(Renderer);return true;
}
#endif
