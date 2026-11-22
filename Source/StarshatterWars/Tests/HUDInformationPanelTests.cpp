#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "FighterHUDDetails.h"
#include "RadioView.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Misc/ScopeExit.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "RenderingThread.h"
#include "RHI.h"
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#include "ContentStreaming.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHUDInformationPanelTest,"Starshatter.Menu.HUDInformationAndCaution",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHUDInformationPanelTest::RunTest(const FString& Parameters)
{
    RadioView::ClearMessages();ON_SCOPE_EXIT {RadioView::ClearMessages();};
    auto HUD=SNew(SFighterHUDDetails).ShowHUD(true);
    TestFalse(TEXT("Messages start hidden until the game supplies text"),HUD->bShowInformation);
    HUD->ToggleInformationPanel();TestFalse(TEXT("Pilot cannot open the game-controlled message panel"),HUD->bShowInformation);
    HUD->ToggleCautionPanel();TestTrue(TEXT("Caution can be inspected when no messages are active"),HUD->bShowCaution);
    TestNotNull(TEXT("Instruction scroll box exists"),HUD->InformationScroll.Get());
    TestNotNull(TEXT("Supplied left artwork loads"),HUD->PanelTextures[7].Get());TestNotNull(TEXT("Supplied right artwork loads"),HUD->PanelTextures[8].Get());
    RadioView::Message("Delta 2 is attacking target. This longer status message must wrap within the instruction frame.");
    TArray<FRadioHUDLine> Lines;RadioView::GetHUDMessages(Lines);
    TestTrue(TEXT("Legacy message lifetime retained"),Lines.Num()==1 && Lines[0].ExpiresAt-FPlatformTime::Seconds()>9.0);
    HUD->UpdateInformationMessages();
    TestTrue(TEXT("Drone mission radio update opens the panel automatically"),HUD->bShowInformation);
    TestFalse(TEXT("Messages temporarily replace caution"),HUD->bShowCaution);
    HUD->ToggleInformationPanel();TestTrue(TEXT("Pilot cannot dismiss active messages"),HUD->bShowInformation);
    TestEqual(TEXT("Live updates populate the bounded scroll content"),HUD->InformationRows->GetChildren()->Num(),1);
    if (!GUsingNullRHI)
    {
        FAssetCompilingManager::Get().FinishAllCompilation();if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();IStreamingManager::Get().StreamAllResources(0);FlushRenderingCommands();
        FWidgetRenderer Renderer(true,false);HUD->SlatePrepass();Renderer.DrawWidget(HUD,FVector2D(1280,720));FlushRenderingCommands();
        auto* Target=Renderer.DrawWidget(HUD,FVector2D(1280,720));FlushRenderingCommands();
        const FVector2D ScrollSize=HUD->InformationScroll->GetCachedGeometry().GetLocalSize();
        TestTrue(TEXT("Scroll area fits inside native caution extents"),ScrollSize.X>200 && ScrollSize.X<=512 && ScrollSize.Y>40 && ScrollSize.Y<=256);
        TArray<FColor> Pixels;FReadSurfaceDataFlags Flags(RCM_UNorm);Flags.SetLinearToGamma(false);Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels,Flags);
        TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(1280,720,Pixels,PNG);const FString Folder=FPaths::ProjectSavedDir()/TEXT("Automation/FirstRunAndLogo");IFileManager::Get().MakeDirectory(*Folder,true);
        TestTrue(TEXT("Instruction HUD capture saved"),FFileHelper::SaveArrayToFile(PNG,*(Folder/TEXT("HUDInformation.png"))));
    }
    RadioView::ClearMessages();HUD->UpdateInformationMessages();
    TestFalse(TEXT("The game hides the panel when its message queue drains"),HUD->bShowInformation);
    TestTrue(TEXT("Caution returns after the queue drains"),HUD->bShowCaution);
    RadioView::Message("Training control: engage the next drone.");HUD->UpdateInformationMessages();
    TestTrue(TEXT("Later drone instructions reopen the panel"),HUD->bShowInformation);
    HUD->bShowInformation=false;HUD->bShowCaution=true;
    RadioView::Message("NO MISSION ASSIGNED. CLEARANCE DECLINED.",2);HUD->UpdateInformationMessages();
    TestTrue(TEXT("Clearance refusal opens Information panel"),HUD->bShowInformation);
    TestFalse(TEXT("Clearance refusal replaces caution panel"),HUD->bShowCaution);
    TestTrue(TEXT("Exact clearance message reaches visible information rows"),HUD->InformationCaptions.Contains(TEXT("NO MISSION ASSIGNED. CLEARANCE DECLINED.")));
    RadioView::ClearMessages();HUD->UpdateInformationMessages();TestEqual(TEXT("Expired/cleared messages leave no stale rows"),HUD->InformationRows->GetChildren()->Num(),0);
    return true;
}
#endif
