#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "MenuDlg.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "MediaTexture.h"
#include "MediaPlayer.h"
#include "UObject/StrongObjectPtr.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"
#include "HAL/PlatformTime.h"
#include "HAL/FileManager.h"
#include "DynamicRHI.h"

namespace
{
class FMenuMovieCommand : public IAutomationLatentCommand
{
public:
    explicit FMenuMovieCommand(FAutomationTestBase* InTest):Test(InTest){}
    bool Update() override
    {
        if (!World.IsValid())
        {
            const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
            World.Reset(UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options));
            if (!World.IsValid()) {Test->AddError(TEXT("Menu preview world unavailable"));return true;}
            World->SetGameInstance(NewObject<UGameInstance>());
            Menu.Reset(NewObject<UMenuDlg>(World.Get()));Menu->Initialize();Menu->SetVisibility(ESlateVisibility::Collapsed);Widget=Menu->TakeWidget();Menu->SetVisibility(ESlateVisibility::Visible);
            Started=FPlatformTime::Seconds();
            return false;
        }
        auto* Picture=Cast<UImage>(Menu->WidgetTree->FindWidget(TEXT("SSWMenu_AttractVideo")));
        auto* Texture=Picture?Cast<UMediaTexture>(Picture->GetBrush().GetResourceObject()):nullptr;
        auto* Player=Texture?Texture->GetMediaPlayer():nullptr;
        if (Player && Player->IsPlaying() && Player->GetTime().GetTotalSeconds()>2)
        {
            Test->TestTrue(TEXT("Attract footage loops"),Player->IsLooping());
            Test->TestTrue(TEXT("Attract window has visible video dimensions"),Texture->GetWidth()>0 && Texture->GetHeight()>0);
            Widget->SlatePrepass();FWidgetRenderer Renderer(true,false);
            Renderer.DrawWidget(Widget.ToSharedRef(),FVector2D(1920,1080));FlushRenderingCommands();
            auto* Target=Renderer.DrawWidget(Widget.ToSharedRef(),FVector2D(1920,1080));FlushRenderingCommands();
            TArray<FColor> Pixels;FReadSurfaceDataFlags Flags(RCM_UNorm);Flags.SetLinearToGamma(false);
            if (Target && Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels,Flags))
            {
                const FString Folder=FPaths::ProjectSavedDir()/TEXT("Automation/FirstRunAndLogo");
                IFileManager::Get().MakeDirectory(*Folder,true);TArray64<uint8> Png;FImageUtils::PNGCompressImageArray(1920,1080,Pixels,Png);
                Test->TestTrue(TEXT("Playing attract video capture saved"),FFileHelper::SaveArrayToFile(Png,*(Folder/TEXT("MainMenuAttract.png"))));
            }
            else Test->AddError(TEXT("Could not capture attract video menu"));
            Menu->Hide();Test->TestFalse(TEXT("Hiding menu closes video playback"),Player->IsPlaying());Cleanup();return true;
        }
        if (FPlatformTime::Seconds()-Started>25) {Test->AddError(TEXT("Attract movie did not reach playback within25seconds"));Cleanup();return true;}
        return false;
    }
private:
    void Cleanup(){if(Menu.IsValid())Menu->Hide();Widget.Reset();Menu.Reset();if(World.IsValid())World->DestroyWorld(false);World.Reset();}
    FAutomationTestBase* Test;TStrongObjectPtr<UWorld> World;TStrongObjectPtr<UMenuDlg> Menu;TSharedPtr<SWidget> Widget;double Started=0;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMenuAttractPlaybackTest,"Starshatter.Menu.AttractVideoPlayback",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMenuAttractPlaybackTest::RunTest(const FString&)
{
    if(GUsingNullRHI){AddInfo(TEXT("Video playback requires RHI; run this case in the visual verification pass."));return true;}
    ADD_LATENT_AUTOMATION_COMMAND(FMenuMovieCommand(this));return true;
}
#endif
