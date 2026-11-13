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
    auto HUD=SNew(SFighterHUDDetails).ShowHUD(true);
    TestTrue(TEXT("Information starts active"),HUD->bShowInformation);TestFalse(TEXT("Caution starts hidden"),HUD->bShowCaution);
    HUD->ToggleCautionPanel();TestTrue(TEXT("Caution shows"),HUD->bShowCaution);TestFalse(TEXT("Caution hides information"),HUD->bShowInformation);
    HUD->ToggleCautionPanel();TestFalse(TEXT("Caution toggles off"),HUD->bShowCaution);TestFalse(TEXT("Both can remain off"),HUD->bShowInformation);
    HUD->ToggleInformationPanel();TestTrue(TEXT("Information toggles on"),HUD->bShowInformation);
    HUD->ToggleCautionPanel();HUD->ToggleInformationPanel();TestFalse(TEXT("Information hides caution"),HUD->bShowCaution);
    HUD->ToggleInformationPanel();TestFalse(TEXT("Information toggles off"),HUD->bShowInformation);
    TestNotNull(TEXT("Instruction scroll box exists"),HUD->InformationScroll.Get());
    TestNotNull(TEXT("Supplied left artwork loads"),HUD->PanelTextures[7].Get());TestNotNull(TEXT("Supplied right artwork loads"),HUD->PanelTextures[8].Get());
    auto* Action=LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/IA_HUDInformation.IA_HUDInformation"));
    auto* Context=LoadObject<UInputMappingContext>(nullptr,TEXT("/Game/Input/IMC_FighterHUD.IMC_FighterHUD"));
    if(TestNotNull(TEXT("Information action exists"),Action) && TestNotNull(TEXT("HUD mapping context exists"),Context))
    {
        TestTrue(TEXT("Information toggle is Boolean"),Action->ValueType==EInputActionValueType::Boolean);
        bool Mapped=false;for(const auto& Mapping:Context->GetMappings())if(Mapping.Action==Action)Mapped=true;
        TestTrue(TEXT("Information toggle participates in remappable HUD bindings"),Mapped);
    }
    RadioView::ClearMessages();ON_SCOPE_EXIT {RadioView::ClearMessages();};
    RadioView::Message("Delta 2 is attacking target. This longer status message must wrap within the instruction frame.");
    TArray<FRadioHUDLine> Lines;RadioView::GetHUDMessages(Lines);
    TestTrue(TEXT("Legacy message lifetime retained"),Lines.Num()==1 && Lines[0].ExpiresAt-FPlatformTime::Seconds()>9.0);
    HUD->ToggleInformationPanel();HUD->UpdateInformationMessages();
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
    RadioView::ClearMessages();HUD->UpdateInformationMessages();TestEqual(TEXT("Expired/cleared messages leave no stale rows"),HUD->InformationRows->GetChildren()->Num(),0);
    return true;
}
#endif
