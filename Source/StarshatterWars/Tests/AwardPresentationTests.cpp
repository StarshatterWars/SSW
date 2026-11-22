#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "ScopedTestWorldContext.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "MissionAwardDlg.h"
#include "AwardShowDlg.h"
#include "AwardsJsonCatalog.h"
#include "Misc/Paths.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Components/NativeWidgetHost.h"
#include "Components/ScaleBox.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopeExit.h"
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#include "ContentStreaming.h"
#include "Engine/DataTable.h"
#include "GameStructs.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAwardPresentationTest,"Starshatter.Mission.AwardPresentation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAwardPresentationTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false);
    FScopedSSWTestWorldContext TestWorldContext1(World);auto* GI=NewObject<UGameInstance>();World->SetGameInstance(GI);
    ON_SCOPE_EXIT{World->DestroyWorld(false);};
    auto* Mission=NewObject<UMissionAwardDlg>(World);Mission->Initialize();auto Widget=Mission->TakeWidget();Mission->EnsureUi();Mission->EnsureUi();
    auto* Logbook=NewObject<UAwardShowDlg>(World);Logbook->Initialize();Logbook->TakeWidget();Logbook->EnsureUi();Logbook->EnsureUi();
    for(auto* Button:{Mission->btn_close,Logbook->btn_close}){
        if(!TestNotNull(TEXT("Standard red close button"),Button))continue;
        TestEqual(TEXT("Award close uses standard tooltip"),Button->GetToolTipText().ToString(),FString(TEXT("Close and return to the live session")));
        auto* Box=Cast<USizeBox>(Button->GetContent());TestNotNull(TEXT("No text on accept button"),Box);
        auto* Icon=Box?Cast<UNativeWidgetHost>(Box->GetContent()):nullptr;
        if(TestNotNull(TEXT("Font-independent close icon"),Icon)){Icon->TakeWidget();TestTrue(TEXT("Close glyph is mounted"),Icon->GetContent().IsValid());}
    }
    for(auto* Image:{Mission->img_rank,Logbook->img_rank})
    {
        auto* Fit=Image?Cast<UScaleBox>(Image->GetParent()):nullptr;
        if(TestNotNull(TEXT("Rank and medal images preserve their aspect ratio"),Fit))
            TestEqual(TEXT("Insignia is fitted, never stretched"),Fit->GetStretch(),EStretch::ScaleToFit);
    }
    Logbook->SetVisibility(ESlateVisibility::Visible);
    const FKeyEvent CloseLogbook(EKeys::Escape,FModifierKeysState(),0,false,0,0);
    Logbook->NativeOnKeyDown(FGeometry(),CloseLogbook);
    TestFalse(TEXT("First Escape dismisses the logbook award without a latch"),Logbook->IsVisible());
    if(!TestNotNull(TEXT("Award title exists"),Mission->lbl_name) || !TestNotNull(TEXT("Award text exists"),Mission->lbl_info))return false;
    Mission->lbl_name->SetText(FText::FromString(TEXT("Rank of Ensign")));
    Mission->lbl_info->SetText(FText::FromString(TEXT("Flight training completed. You are promoted to Ensign.")));
    TArray<FRankInfo> Ranks;TArray<FMedalInfo> Medals;FString Error;
    if(!TestTrue(TEXT("Promotion images load from the native awards catalog"),AwardsJsonCatalog::Load(FPaths::ProjectContentDir()/TEXT("GameData/Awards/awards.json"),Ranks,Medals,Error)))return false;
    bool FoundEnsign=false;
    for(const auto& Row:Ranks)if(Row.Name.Equals(TEXT("Ensign"),ESearchCase::IgnoreCase))
        if(auto* Texture=Row.LargeImage.LoadSynchronous())
        {
            FoundEnsign=true;Mission->img_rank->SetBrushFromTexture(Texture,true);
            TestTrue(TEXT("Insignia uses its texture's natural aspect"),Mission->img_rank->GetBrush().ImageSize.X==Texture->GetSizeX() && Mission->img_rank->GetBrush().ImageSize.Y==Texture->GetSizeY());
        }
    TestTrue(TEXT("Ensign insignia is available"),FoundEnsign);
    FString Capture;
    if(!GUsingNullRHI && FParse::Value(FCommandLine::Get(),TEXT("AwardCapture="),Capture)){
        FlushAsyncLoading();FAssetCompilingManager::Get().FinishAllCompilation();
        if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
        IStreamingManager::Get().StreamAllResources(0);
        FWidgetRenderer* Renderer=new FWidgetRenderer(true);TStrongObjectPtr<UTextureRenderTarget2D> RT(FWidgetRenderer::CreateTargetFor(FVector2D(1280,720),TF_Bilinear,true));
        Renderer->DrawWidget(RT.Get(),Widget,FVector2D(1280,720),0);FlushRenderingCommands();
        Renderer->DrawWidget(RT.Get(),Widget,FVector2D(1280,720),.016f);FlushRenderingCommands();
        TestTrue(TEXT("Accept button has visible layout width"),Mission->btn_close->GetCachedGeometry().GetLocalSize().X>=28);
        TestNotNull(TEXT("Accept button background texture"),Mission->btn_close->GetStyle().Normal.GetResourceObject());
        auto* RenderedIcon=Cast<UNativeWidgetHost>(CastChecked<USizeBox>(Mission->btn_close->GetContent())->GetContent());
        TestTrue(TEXT("Close icon has visible rendered geometry"),RenderedIcon && RenderedIcon->GetCachedGeometry().GetLocalSize().X>=18);
        TArray<FColor> Pixels;RT->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(1280,720,Pixels,PNG);
        TestTrue(TEXT("Award preview saved"),FFileHelper::SaveArrayToFile(PNG,*Capture));BeginCleanup(Renderer);
    }
    if (!TestNotNull(TEXT("Mission award accept button"), Mission->btn_close))
        return false;

    Mission->SetVisibility(ESlateVisibility::Visible);
    Mission->btn_close->OnClicked.Broadcast();
    TestTrue(TEXT("Close dismisses the mission award"), Mission->GetVisibility() == ESlateVisibility::Collapsed);

    Mission->SetVisibility(ESlateVisibility::Visible);
    const FKeyEvent RepeatEnter(EKeys::Enter, FModifierKeysState(), 0, true, 0, 0);
    Mission->NativeOnKeyDown(FGeometry(), RepeatEnter);
    TestTrue(TEXT("Held Enter leaves the award open"), Mission->GetVisibility() == ESlateVisibility::Visible);

    for (const FKey Key : {EKeys::Enter, EKeys::Escape})
    {
        Mission->SetVisibility(ESlateVisibility::Visible);
        const FKeyEvent KeyPress(Key, FModifierKeysState(), 0, false, 0, 0);
        Mission->NativeOnKeyDown(FGeometry(), KeyPress);
        TestTrue(TEXT("Fresh Enter or Escape dismisses the mission award"), Mission->GetVisibility() == ESlateVisibility::Collapsed);
    }

    return true;
}
#endif
