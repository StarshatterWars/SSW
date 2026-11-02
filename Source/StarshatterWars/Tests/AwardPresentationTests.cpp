#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "MissionAwardDlg.h"
#include "AwardShowDlg.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
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
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);auto* GI=NewObject<UGameInstance>();World->SetGameInstance(GI);
    ON_SCOPE_EXIT{World->DestroyWorld(false);};
    auto* Mission=NewObject<UMissionAwardDlg>(World);Mission->Initialize();auto Widget=Mission->TakeWidget();Mission->EnsureUi();Mission->EnsureUi();
    auto* Logbook=NewObject<UAwardShowDlg>(World);Logbook->Initialize();Logbook->TakeWidget();Logbook->EnsureUi();Logbook->EnsureUi();
    for(auto* Button:{Mission->btn_close,Logbook->btn_close}){
        if(!TestNotNull(TEXT("Modern checkmark accept button"),Button))continue;
        TestEqual(TEXT("Award close uses standard tooltip"),Button->GetToolTipText().ToString(),FString(TEXT("Accept award and return")));
        auto* Box=Cast<USizeBox>(Button->GetContent());TestNotNull(TEXT("No text on accept button"),Box);
        auto* Icon=Box?Cast<UImage>(Box->GetContent()):nullptr;
        if(TestNotNull(TEXT("Checkmark icon"),Icon))TestNotNull(TEXT("Icon asset loads"),Icon->GetBrush().GetResourceObject());
    }
    if(!TestNotNull(TEXT("Award title exists"),Mission->lbl_name) || !TestNotNull(TEXT("Award text exists"),Mission->lbl_info))return false;
    Mission->lbl_name->SetText(FText::FromString(TEXT("Rank of Ensign")));
    Mission->lbl_info->SetText(FText::FromString(TEXT("Flight training completed. You are promoted to Ensign.")));
    if(auto* Ranks=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/FRankInfo.FRankInfo")))
    {
        TArray<FRankInfo*> Rows;Ranks->GetAllRows(TEXT("Award preview"),Rows);
        for(auto* Row:Rows)if(Row->Name.Equals(TEXT("Ensign"),ESearchCase::IgnoreCase))
            if(auto* Texture=Row->LargeImage.LoadSynchronous())Mission->img_rank->SetBrushFromTexture(Texture);
    }
    FString Capture;
    if(!GUsingNullRHI && FParse::Value(FCommandLine::Get(),TEXT("AwardCapture="),Capture)){
        FlushAsyncLoading();FAssetCompilingManager::Get().FinishAllCompilation();
        if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
        IStreamingManager::Get().StreamAllResources(0);
        FWidgetRenderer* Renderer=new FWidgetRenderer(true);TStrongObjectPtr<UTextureRenderTarget2D> RT(FWidgetRenderer::CreateTargetFor(FVector2D(1280,720),TF_Bilinear,true));
        Renderer->DrawWidget(RT.Get(),Widget,FVector2D(1280,720),0);FlushRenderingCommands();
        Renderer->DrawWidget(RT.Get(),Widget,FVector2D(1280,720),.016f);FlushRenderingCommands();
        TestTrue(TEXT("Accept button has visible layout width"),Mission->btn_close->GetCachedGeometry().GetLocalSize().X>40);
        TestNotNull(TEXT("Accept button background texture"),Mission->btn_close->GetStyle().Normal.GetResourceObject());
        auto* RenderedIcon=Cast<UImage>(CastChecked<USizeBox>(Mission->btn_close->GetContent())->GetContent());
        TestTrue(TEXT("Checkmark has nonzero rendered geometry"),RenderedIcon->GetCachedGeometry().GetLocalSize().X>30 && RenderedIcon->GetCachedGeometry().GetLocalSize().Y>30);
        UE_LOG(LogTemp,Display,TEXT("[AwardIconTest] Size=%s DrawAs=%d"),*RenderedIcon->GetCachedGeometry().GetLocalSize().ToString(),int32(RenderedIcon->GetBrush().DrawAs));
        TArray<FColor> Pixels;RT->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(1280,720,Pixels,PNG);
        TestTrue(TEXT("Award preview saved"),FFileHelper::SaveArrayToFile(PNG,*Capture));BeginCleanup(Renderer);
    }
    return true;
}
#endif
