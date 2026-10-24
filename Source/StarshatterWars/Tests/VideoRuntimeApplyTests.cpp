#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameUserSettings.h"
#include "StarshatterVideoSubsystem.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVideoRuntimeApplyTest,"Starshatter.Audit.F24.VideoRuntimeApply",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVideoRuntimeApplyTest::RunTest(const FString& Parameters)
{
    auto* Settings=GEngine?GEngine->GetGameUserSettings():nullptr;if(!TestNotNull(TEXT("Unreal runtime settings available"),Settings))return false;
    const FIntPoint Resolution=Settings->GetScreenResolution();const auto Mode=Settings->GetFullscreenMode();const int Shadows=Settings->GetShadowQuality(),Textures=Settings->GetTextureQuality();const float Gamma=GEngine->DisplayGamma;
    const FString Path=FPaths::ProjectSavedDir()/TEXT("Starshatter/Config/video.cfg");TArray<uint8> OldFile;const bool Existed=FFileHelper::LoadFileToArray(OldFile,*Path);
    TMap<FString,FString> CVars;for(const TCHAR* Name:{TEXT("r.LensFlareQuality"),TEXT("r.DetailMode"),TEXT("r.MaterialQualityLevel")})if(auto* V=IConsoleManager::Get().FindConsoleVariable(Name))CVars.Add(Name,V->GetString());
    ON_SCOPE_EXIT {
        Settings->SetScreenResolution(Resolution);Settings->SetFullscreenMode(Mode);Settings->SetShadowQuality(Shadows);Settings->SetTextureQuality(Textures);GEngine->DisplayGamma=Gamma;
        for(const auto& Pair:CVars)IConsoleManager::Get().FindConsoleVariable(*Pair.Key)->Set(*Pair.Value,ECVF_SetByGameSetting);
        if(Existed)FFileHelper::SaveArrayToFile(OldFile,*Path);else IFileManager::Get().Delete(*Path);
    };
    auto* Subsystem=NewObject<UStarshatterVideoSubsystem>(NewObject<UGameInstance>());
    FStarshatterVideoConfig C;C.Width=1600;C.Height=900;C.bFullscreen=false;C.bShadows=false;C.GammaLevel=160;Subsystem->SetConfig(C);Subsystem->ApplySettingsToRuntime();
    TestTrue(TEXT("Applying config updates actual Unreal resolution"),Settings->GetScreenResolution()==FIntPoint(1600,900));
    TestTrue(TEXT("Applying config updates actual Unreal window mode"),Settings->GetFullscreenMode()==EWindowMode::Windowed);
    TestEqual(TEXT("Applying config updates actual shadow quality"),Settings->GetShadowQuality(),0);
    TestTrue(TEXT("Gamma reaches the engine"),FMath::IsNearlyEqual(GEngine->DisplayGamma,2.75f));
    FString Saved;FFileHelper::LoadFileToString(Saved,*Path);TestTrue(TEXT("Fullscreen survives config serialization"),Saved.Contains(TEXT("fullscreen: false")));
    return true;
}
#endif
