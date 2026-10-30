#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "OptionsScreen.h"
#include "StarshatterSettingsSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "KeyMap.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Components/TextBlock.h"
#include "Components/Slider.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "RenderingThread.h"
#include "DynamicRHI.h"
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#include "ContentStreaming.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOptionsSaveRoundTripTest,"Starshatter.Options.SaveRoundTrip",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOptionsSaveRoundTripTest::RunTest(const FString& Parameters)
{
    auto* Saved=NewObject<UStarshatterSettingsSaveGame>();
    Saved->Audio.MasterVolume=0.37f;Saved->Audio.MusicVolume=0.21f;
    Saved->Video.Width=2560;Saved->Video.GammaLevel=160;
    Saved->Controls.MouseSensitivity=39;Saved->Controls.bMouseInvert=true;
    Saved->KeyboardConfig.RemappedKeys.Add(static_cast<EStarshatterInputAction>(0),EKeys::F8);
    Saved->JoystickBindings.Add(KEY_AXIS_YAW,KEY_JOY_AXIS_X+3);
    Saved->JoystickBindings.Add(KEY_AXIS_YAW_INVERT,1);
    Saved->bHasGameOptions=true;
    Saved->GameOptions.FlightModel=1;Saved->GameOptions.LandingModel=1;Saved->GameOptions.FlyingStart=1;
    Saved->GameOptions.AILevel=2;Saved->GameOptions.HUDMode=1;Saved->GameOptions.HUDColor=2;
    Saved->GameOptions.FriendlyFire=3;Saved->GameOptions.GridMode=1;Saved->GameOptions.Gunsight=1;
    TestEqual(TEXT("Settings filename is options.sav"),Saved->GetDefaultSlotName(),FString(TEXT("options")));
    const FString Slot=TEXT("Automation/Task5/")+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT("/options");
    ON_SCOPE_EXIT { UGameplayStatics::DeleteGameInSlot(Slot,0); };
    if (!TestTrue(TEXT("Save all settings to isolated options.sav fixture"),UGameplayStatics::SaveGameToSlot(Saved,Slot,0))) return false;
    auto* Loaded=Cast<UStarshatterSettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot,0));
    if (!TestNotNull(TEXT("Reload options.sav"),Loaded)) return false;
    TestEqual(TEXT("Audio persisted"),Loaded->Audio.MasterVolume,0.37f);
    TestEqual(TEXT("Music persisted"),Loaded->Audio.MusicVolume,0.21f);
    TestEqual(TEXT("Video persisted"),Loaded->Video.Width,2560);
    TestEqual(TEXT("Gamma persisted"),Loaded->Video.GammaLevel,160);
    TestEqual(TEXT("Controls persisted"),Loaded->Controls.MouseSensitivity,39);
    TestTrue(TEXT("Mouse invert persisted"),Loaded->Controls.bMouseInvert);
    TestEqual(TEXT("Keyboard binding persisted"),Loaded->KeyboardConfig.RemappedKeys.FindRef(static_cast<EStarshatterInputAction>(0)),EKeys::F8);
    TestEqual(TEXT("Joystick axis persisted"),Loaded->JoystickBindings.FindRef(KEY_AXIS_YAW),KEY_JOY_AXIS_X+3);
    TestEqual(TEXT("Joystick invert persisted"),Loaded->JoystickBindings.FindRef(KEY_AXIS_YAW_INVERT),1);
    TestTrue(TEXT("All nine game options persisted"),FSSWGameOptions::StaticStruct()->CompareScriptStruct(&Saved->GameOptions,&Loaded->GameOptions,0));
    TestTrue(TEXT("Game options validity persisted"),Loaded->bHasGameOptions);
    Saved->SaveVersion=1;Saved->bHasGameOptions=false;
    TestTrue(TEXT("Previous schema migrates"),Saved->MigrateToCurrentVersion());
    TestEqual(TEXT("Migration keeps audio"),Saved->Audio.MasterVolume,0.37f);
    TestFalse(TEXT("Migration does not invent gameplay choices"),Saved->bHasGameOptions);
    TestEqual(TEXT("Current schema version"),Saved->SaveVersion,UStarshatterSettingsSaveGame::CurrentVersion);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOptionsModernizationTest,"Starshatter.Options.AuthoredPagesAndHeader",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOptionsModernizationTest::RunTest(const FString& Parameters)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    UGameInstance* GI=NewObject<UGameInstance>();World->SetGameInstance(GI);
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    UClass* ScreenClass=LoadClass<UOptionsScreen>(nullptr,TEXT("/Game/Screens/Options/WBP_OptionsScreen.WBP_OptionsScreen_C"));
    if (!TestNotNull(TEXT("Authored Options Blueprint loads"),ScreenClass)) return false;
    auto* Screen=NewObject<UOptionsScreen>(World,ScreenClass);Screen->Initialize();
    UWidgetSwitcher* Switcher=Screen->OptionsSwitcher;
    UUserWidget* Pages[]={Screen->AudioDlg,Screen->ControlsDlg,Screen->GameDlg,Screen->JoystickDlg,Screen->KeyboardDlg,Screen->ModsDlg,Screen->VideoDlg};
    for (UUserWidget* Page:Pages) if (!TestNotNull(TEXT("Original settings page bound"),Page)) return false;
    auto Widget=Screen->TakeWidget(); Widget->SlatePrepass();
    TestNotNull(TEXT("Modern shell built"),Screen->ModernShell.Get());
    TestTrue(TEXT("Original switcher retained"),Switcher==Screen->OptionsSwitcher);
    TestEqual(TEXT("Seven original pages plus Credits, no duplication"),Switcher->GetNumWidgets(),8);
    for (UUserWidget* Page:Pages) TestTrue(TEXT("Original page identity retained"),Switcher->GetChildIndex(Page)>=0);
    TestNotNull(TEXT("Red back arrow"),Screen->BtnCancel.Get());TestNotNull(TEXT("Green apply arrow"),Screen->BtnSave.Get());
    TestTrue(TEXT("Header back arrow visible"),Screen->BtnCancel->GetVisibility()==ESlateVisibility::Visible);
    TestTrue(TEXT("Header apply arrow visible"),Screen->BtnSave->GetVisibility()==ESlateVisibility::Visible);
    Screen->ShowGameDlg();TestTrue(TEXT("Game tab selects original page"),Switcher->GetActiveWidget()==Pages[2]);
    TestTrue(TEXT("Selected tab remains keyboard focusable"),Screen->BtnGame->GetIsEnabled());
    Screen->FocusTabByDelta(1);TestTrue(TEXT("Forward tab navigation"),Switcher->GetActiveWidget()==Pages[3]);
    Screen->FocusTabByDelta(-1);TestTrue(TEXT("Reverse tab navigation"),Switcher->GetActiveWidget()==Pages[2]);
    Screen->ShowCreditsDlg();TestTrue(TEXT("Credits retained"),Switcher->GetActiveWidget()==Screen->CreditsPage);
    Screen->ShowAudDlg();
    FString CaptureDir;
    if (!GUsingNullRHI && FParse::Value(FCommandLine::Get(),TEXT("OptionsCaptureDir="),CaptureDir))
    {
        IFileManager::Get().MakeDirectory(*CaptureDir,true);
        FlushAsyncLoading();
        FAssetCompilingManager::Get().FinishAllCompilation();
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
        IStreamingManager::Get().StreamAllResources(0.f);
        FlushRenderingCommands();
        FWidgetRenderer Renderer(true,false);
        // First paint requests UI material shader resources; capture after they finish.
        Renderer.DrawWidget(Widget,FVector2D(1920,1080));FlushRenderingCommands();
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
        FlushRenderingCommands();
        const TCHAR* Names[]={TEXT("Audio"),TEXT("Controls"),TEXT("Game"),TEXT("Joystick"),TEXT("Keyboard"),TEXT("Mods"),TEXT("Video"),TEXT("Credits")};
        for (int32 Index=0;Index<8;++Index)
        {
            for(int32 Step=0;Screen->ActiveTabIndex!=Index && Step<8;++Step) Screen->FocusTabByDelta(1);
            Widget->SlatePrepass();
            auto* Target=Renderer.DrawWidget(Widget,FVector2D(1920,1080));FlushRenderingCommands();
            TArray<FColor> Pixels;FReadSurfaceDataFlags ReadFlags(RCM_UNorm);ReadFlags.SetLinearToGamma(false);Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels,ReadFlags);
            TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(1920,1080,Pixels,PNG);
            FFileHelper::SaveArrayToFile(PNG,*FPaths::Combine(CaptureDir,FString(Names[Index])+TEXT(".png")));
        }
    }
    Screen->RemoveFromParent();
    return true;
}
#endif
