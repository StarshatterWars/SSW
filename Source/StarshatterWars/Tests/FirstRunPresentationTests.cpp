#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "FirstTimeDlg.h"
#include "MenuDlg.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Engine/Texture2D.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "ImageUtils.h"
#include "RenderingThread.h"
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#include "ContentStreaming.h"
#include "FormattingUtils.h"
#include "Internationalization/Regex.h"
#include "HAL/FileManager.h"
#include "RHI.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstRunPresentationTest,"Starshatter.Menu.FirstRunAndCircularLogo",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFirstRunPresentationTest::RunTest(const FString& Parameters)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);World->SetGameInstance(NewObject<UGameInstance>());
    ON_SCOPE_EXIT {World->DestroyWorld(false);};
    auto* Setup=NewObject<UFirstTimeDlg>(World);Setup->Initialize();auto SetupSlate=Setup->TakeWidget();
    auto* Frame=Cast<USizeBox>(Setup->WidgetTree->FindWidget(TEXT("FirstRunFrame")));
    if(!TestNotNull(TEXT("First-run modal exists"),Frame))return false;
    TestEqual(TEXT("First-run width is 512"),Frame->GetWidthOverride(),512.f);
    TestEqual(TEXT("First-run height is 512"),Frame->GetHeightOverride(),512.f);
    TestNotNull(TEXT("Pilot name input"),Setup->NameEdit.Get());TestNotNull(TEXT("Callsign input"),Setup->CallsignEdit.Get());
    TestEqual(TEXT("Only Alliance and Marakan are first-run choices"),Setup->EmpireCombo->GetOptionCount(),2);
    TestEqual(TEXT("Dantari remains campaign-only"),Setup->EmpireCombo->FindOptionIndex(TEXT("Dantari Separatists")),INDEX_NONE);
    TestEqual(TEXT("Both flight models preserved"),Setup->PlayStyleCombo->GetOptionCount(),2);
    TestEqual(TEXT("Both experience choices preserved"),Setup->ExperienceCombo->GetOptionCount(),2);
    Setup->NameEdit->SetText(FText::FromString(TEXT("Caloy")));Setup->CallsignEdit->SetText(FText::FromString(TEXT("Caloy")));
    auto* Menu=NewObject<UMenuDlg>(World);Menu->Initialize();auto MenuSlate=Menu->TakeWidget();
    auto* Logo=Cast<UImage>(Menu->WidgetTree->FindWidget(TEXT("SSWMenu_CircularLogo")));
    if(!TestNotNull(TEXT("Main menu circular logo widget exists"),Logo))return false;
    TestTrue(TEXT("Circular asset is actually assigned"),Logo->GetBrush().GetResourceObject() && Logo->GetBrush().GetResourceObject()->GetPathName()==TEXT("/Game/UI/Materials/M_UISSWCircularLogo.M_UISSWCircularLogo"));
    if (GUsingNullRHI) {
        auto VerifyAsset=[&](UButton* Button,const TCHAR* Path){
            auto* Size=Button?Cast<USizeBox>(Button->GetContent()):nullptr;
            auto* Icon=Size?Cast<UImage>(Size->GetContent()):nullptr;
            TestNotNull(TEXT("Standard icon widget exists"),Icon);
            if(Icon)TestTrue(TEXT("Standard arrow asset assigned"),Icon->GetBrush().GetResourceObject() && Icon->GetBrush().GetResourceObject()->GetPathName()==Path);
        };
        VerifyAsset(Setup->AcceptBtn,TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_Arrow_R_256x256.T_Arrow_R_256x256"));
        VerifyAsset(Setup->CancelBtn,TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_OnOff_256x256.T_OnOff_256x256"));
        AddWarning(TEXT("NullRHI: structural and asset assertions passed; pixel captures require an RHI-enabled run."));
        return true;
    }
    FlushAsyncLoading();FAssetCompilingManager::Get().FinishAllCompilation();
    if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();IStreamingManager::Get().StreamAllResources(0);FlushRenderingCommands();
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("Automation/FirstRunAndLogo");IFileManager::Get().MakeDirectory(*Dir,true);
    FWidgetRenderer Renderer(true,false);
    auto Capture=[&](TSharedRef<SWidget> Widget,const TCHAR* Name){Widget->SlatePrepass();Renderer.DrawWidget(Widget,FVector2D(1920,1080));FlushRenderingCommands();
        auto* Target=Renderer.DrawWidget(Widget,FVector2D(1920,1080));FlushRenderingCommands();TArray<FColor> Pixels;FReadSurfaceDataFlags Flags(RCM_UNorm);Flags.SetLinearToGamma(false);
        if(!Target || !Target->GameThread_GetRenderTargetResource()) {AddError(TEXT("UI capture target unavailable"));return Pixels;}
        if(!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels,Flags)){AddError(TEXT("UI capture readback failed"));return Pixels;}
        TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(1920,1080,Pixels,PNG);
        TestTrue(TEXT("UI verification capture saved"),FFileHelper::SaveArrayToFile(PNG,*FPaths::Combine(Dir,FString(Name)+TEXT(".png")))); return Pixels;};
    Capture(SetupSlate,TEXT("FirstRun"));Capture(MenuSlate,TEXT("MainMenu"));
    AddInfo(FString::Printf(TEXT("Circular logo geometry %s at %s, opacity %.3f brush tint %s"),*Logo->GetCachedGeometry().GetLocalSize().ToString(),*Logo->GetCachedGeometry().GetAbsolutePosition().ToString(),Logo->GetRenderOpacity(),*Logo->GetBrush().TintColor.GetSpecifiedColor().ToString()));
    TestTrue(TEXT("Circular logo has visible layout area"),Logo->GetCachedGeometry().GetLocalSize().X>100 && Logo->GetCachedGeometry().GetLocalSize().Y>100);
    Capture(Logo->TakeWidget(),TEXT("LogoOnly"));
    auto* AcceptIcon=Cast<UImage>(CastChecked<USizeBox>(Setup->AcceptBtn->GetContent())->GetContent());
    AddInfo(FString::Printf(TEXT("First-run accept icon geometry %s, opacity %.3f"),*AcceptIcon->GetCachedGeometry().GetLocalSize().ToString(),AcceptIcon->GetRenderOpacity()));
    TestTrue(TEXT("Accept icon is laid out visibly"),AcceptIcon->GetCachedGeometry().GetLocalSize().X>=20);
    auto VerifyIcon=[&](UButton* Button,const TCHAR* Name,const TCHAR* Path){
        auto* Icon=CastChecked<UImage>(CastChecked<USizeBox>(Button->GetContent())->GetContent());
        TestEqual(TEXT("Standard arrow texture assigned"),Icon->GetBrush().GetResourceObject()->GetPathName(),FString(Path));
        const auto Pixels=Capture(Icon->TakeWidget(),Name);
        int32 WhitePixels=0,TransparentPixels=0;
        for(const FColor& Pixel:Pixels){if(Pixel.A>128 && Pixel.R>200 && Pixel.G>200 && Pixel.B>200)++WhitePixels;if(Pixel.A==0)++TransparentPixels;}
        TestTrue(TEXT("White arrow renders actual visible pixels"),WhitePixels>100);
        TestTrue(TEXT("Arrow background remains transparent"),TransparentPixels>100);
    };
    VerifyIcon(Setup->AcceptBtn,TEXT("AcceptIconOnly"),TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_Arrow_R_256x256.T_Arrow_R_256x256"));
    VerifyIcon(Setup->CancelBtn,TEXT("CancelIconOnly"),TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_OnOff_256x256.T_OnOff_256x256"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignTimeFormatTest,"Starshatter.CampaignUI.StandardMissionTimes",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampaignTimeFormatTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("33 legacy elapsed hours retain the same mission time"),UFormattingUtils::ParseStarshatterTime(TEXT("33:00:00")),UFormattingUtils::ParseStarshatterTime(TEXT("02/09:00:00")));
    TArray<FString> Files;IFileManager::Get().FindFilesRecursive(Files,*(FPaths::ProjectContentDir()/TEXT("GameData/Campaigns")),TEXT("*.def"),true,false);
    const FRegexPattern TimeField(TEXT("\\b(?:start|after|start_before|start_after)\\s*:\\s*\"([^\"]+)\""));
    const FRegexPattern Standard(TEXT("^[0-9]{2}/(?:[01][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9]$"));int32 Count=0;
    for(const FString& File:Files){FString Source;FFileHelper::LoadFileToString(Source,*File);FRegexMatcher M(TimeField,Source);
        while(M.FindNext()){const FString Value=M.GetCaptureGroup(1);FRegexMatcher Check(Standard,Value);
            TestTrue(*FString::Printf(TEXT("Standard time in %s: %s"),*File,*Value),Check.FindNext());++Count;}}
    TestTrue(TEXT("Campaign mission timestamp inventory is nonempty"),Count>100);
    AddInfo(FString::Printf(TEXT("Validated %d campaign timestamps in %d definition files"),Count,Files.Num()));return true;
}
#endif
