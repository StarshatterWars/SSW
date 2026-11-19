#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "RecordDetails.h"
#include "RecordedCutsceneDlg.h"
#include "MediaPlayer.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectHash.h"
#include "UObject/GarbageCollection.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "RenderingThread.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Styling/CoreStyle.h"
#include "DynamicRHI.h"
namespace {
bool CaptureWidget(TSharedRef<SWidget> Widget,FVector2D Size,const TCHAR* Filename) {
    FWidgetRenderer Renderer(false,false);
    UTextureRenderTarget2D* Target=nullptr;
    // Auto-wrapped text needs a layout pass after receiving its actual width.
    for(int32 Pass=0;Pass<3;++Pass){Widget->SlatePrepass();Target=Renderer.DrawWidget(Widget,Size);FlushRenderingCommands();}
    TArray<FColor> Pixels;FReadSurfaceDataFlags Flags(RCM_UNorm);Flags.SetLinearToGamma(false);
    if(!Target || !Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels,Flags))return false;
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("Automation/PanelPolish");IFileManager::Get().MakeDirectory(*Folder,true);
    TArray64<uint8> Png;FImageUtils::PNGCompressImageArray(int32(Size.X),int32(Size.Y),Pixels,Png);
    return FFileHelper::SaveArrayToFile(Png,*(Folder/Filename));
}
class FRecordedReplayPlaybackCommand : public IAutomationLatentCommand {
public:
    explicit FRecordedReplayPlaybackCommand(FAutomationTestBase* InTest):Test(InTest){}
    bool Update() override {
        const double Now=FPlatformTime::Seconds();
        if(!World.IsValid()) {
            const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
            World.Reset(UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options));
            if(!World.IsValid()){Test->AddError(TEXT("Replay world unavailable"));return true;}
            World->SetGameInstance(NewObject<UGameInstance>());
            Panel.Reset(NewObject<URecordedCutsceneDlg>(World.Get()));Panel->UUserWidget::Initialize();Widget=Panel->TakeWidget();
            if(!Panel->LoadMovie(URecordedCutsceneDlg::MoviePath(1,TEXT("Intro")))){Test->AddError(TEXT("Intro replay failed to open"));Cleanup();return true;}
            TArray<UObject*> Owned;GetObjectsWithOuter(Panel.Get(),Owned,false);
            for(UObject* Object:Owned)if(auto* P=Cast<UMediaPlayer>(Object))Player=P;
            Started=Now;NextCollection=Now+2;return false;
        }
        if(Player.IsValid() && Player->IsPlaying() && Player->GetTime().GetTotalSeconds()>1 && Now>=NextCollection){
            CollectGarbage(RF_NoFlags);
            Test->TestTrue(TEXT("Media player survives forced GC"),Player.IsValid());
            Test->TestTrue(TEXT("Replay paints after forced GC"),CaptureWidget(Widget.ToSharedRef(),FVector2D(960,600),TEXT("IntelReplayAfterGC.png")));
            NextCollection=Now+2;
            if(++Collections==3){
                Test->TestTrue(TEXT("Intro reports its full duration"),Player->GetDuration().GetTotalSeconds()>120);
                Test->TestTrue(TEXT("Replay seek remains available after collection"),Player->Seek(FTimespan::FromSeconds(90)));
                Cleanup();return true;
            }
        }
        if(Now-Started>40){Test->AddError(TEXT("Replay did not complete three live playback/GC/paint cycles"));Cleanup();return true;}
        return false;
    }
private:
    void Cleanup(){Widget.Reset();if(Panel.IsValid())static_cast<UWidget*>(Panel.Get())->ReleaseSlateResources(true);Panel.Reset();if(World.IsValid())World->DestroyWorld(false);World.Reset();}
    FAutomationTestBase* Test;TStrongObjectPtr<UWorld> World;TStrongObjectPtr<URecordedCutsceneDlg> Panel;TWeakObjectPtr<UMediaPlayer> Player;TSharedPtr<SWidget> Widget;double Started=0,NextCollection=0;int32 Collections=0;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecordedReplayPlaybackTest,"Starshatter.Intel.RecordedReplayPlaybackLifetime",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecordedReplayPlaybackTest::RunTest(const FString&) {
    if(GUsingNullRHI){AddError(TEXT("This test needs a render device for video and Slate painting."));return false;}
    ADD_LATENT_AUTOMATION_COMMAND(FRecordedReplayPlaybackCommand(this));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecordDetailsRenderTest,"Starshatter.UI.RecordDetailsRendering",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecordDetailsRenderTest::RunTest(const FString&) {
    if(GUsingNullRHI){AddError(TEXT("This visual test needs a render device."));return false;}
    FRecordDetails Factory;Factory.Icon=EShipSystemIcon::Production;Factory.Title=INVTEXT("Janek Orbital Materials Works");Factory.Subtitle=INVTEXT("JANUS / JANEK / ORBITAL INDUSTRY");Factory.Status=INVTEXT("PRODUCING");Factory.Description=INVTEXT("Live refinery inventory and batch requirements.");
    Factory.Fact(INVTEXT("MAX STORAGE"),INVTEXT("40,000 T")).Fact(INVTEXT("BATCH TIME"),INVTEXT("60 S")).Fact(INVTEXT("INSTALLATION"),INVTEXT("1,250 CR + 50 CR / RUN")).Fact(INVTEXT("FACILITY"),INVTEXT("ORBITAL"));
    Factory.Section(INVTEXT("REQUIRED INPUT / BATCH"),INVTEXT("IRON ORE / 250 T\nNICKEL ORE / 50 T")).Section(INVTEXT("FINISHED OUTPUT / BATCH"),INVTEXT("METAL CONCENTRATE / 200 T"));
    FRecordDetails Contract;Contract.Icon=EShipSystemIcon::Contracts;Contract.Title=INVTEXT("Industrial supply run");Contract.Subtitle=INVTEXT("COURIER CONTRACT");Contract.Status=INVTEXT("OPEN");Contract.Description=INVTEXT("Deliver sealed refinery components to the orbital works. The cargo package remains sealed until delivery.");
    Contract.Fact(INVTEXT("PICKUP"),INVTEXT("JANUS TRANSFER STATION")).Fact(INVTEXT("DESTINATION"),INVTEXT("JANEK ORBITAL MATERIALS WORKS")).Fact(INVTEXT("REWARD"),INVTEXT("7,500 CR")).Fact(INVTEXT("COLLATERAL"),INVTEXT("15,000 CR"));Contract.Section(INVTEXT("OFFERED CARGO"),INVTEXT("SEALED MACHINERY / 2 UNITS"));
    auto Tile=[](const FRecordDetails& Data){return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.025f,.045f,.06f,1)).Padding(20)[SNew(SScrollBox)+SScrollBox::Slot()[SNew(SRecordDetails).Record(Data)]];};
    auto Layout=SNew(SHorizontalBox)+SHorizontalBox::Slot().FillWidth(1).Padding(8)[Tile(Factory)]+SHorizontalBox::Slot().FillWidth(1).Padding(8)[Tile(Contract)];
    TestTrue(TEXT("Wide detail cards render"),CaptureWidget(Layout,FVector2D(1440,520),TEXT("RecordDetailsWide.png")));
    TestTrue(TEXT("Narrow detail card renders"),CaptureWidget(Tile(Contract),FVector2D(480,620),TEXT("RecordDetailsNarrow.png")));
    return true;
}
#endif
