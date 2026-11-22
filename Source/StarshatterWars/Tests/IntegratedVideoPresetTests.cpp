#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "VideoDlg.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntegratedVideoPresetTest,"Starshatter.Video.IntegratedPreset",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FIntegratedVideoPresetTest::RunTest(const FString&)
{
    auto* Dialog=NewObject<UVideoDlg>();
    for(int32 Index=0;Index<9;++Index)
    {
        auto* Combo=NewObject<UComboBoxString>(Dialog);
        for(int32 Quality=0;Quality<5;++Quality)Combo->AddOption(FString::FromInt(Quality));
        Dialog->QualityCombos.Add(Combo);
    }
    Dialog->AntiAliasingMethodCombo=NewObject<UComboBoxString>(Dialog);
    for(const TCHAR* Method:{TEXT("OFF"),TEXT("FXAA"),TEXT("TAA"),TEXT("TSR")})Dialog->AntiAliasingMethodCombo->AddOption(Method);
    Dialog->FrameLimitCombo=NewObject<UComboBoxString>(Dialog);
    Dialog->FrameLimitCombo->AddOption(TEXT("30 FPS"));
    Dialog->RenderScaleSlider=NewObject<USlider>(Dialog);
    Dialog->OnLightPresetClicked();
    TestEqual(TEXT("Light mode uses less expensive temporal AA"),Dialog->AntiAliasingMethodCombo->GetSelectedIndex(),2);
    TestEqual(TEXT("Hull texture quality is retained"),Dialog->QualityCombos[3]->GetSelectedIndex(),2);
    TestEqual(TEXT("Expensive GI is disabled"),Dialog->QualityCombos[6]->GetSelectedIndex(),0);
    TestEqual(TEXT("Expensive reflections are disabled"),Dialog->QualityCombos[7]->GetSelectedIndex(),0);
    TestEqual(TEXT("Frame pacing target is 30 FPS"),Dialog->FrameLimitCombo->GetSelectedOption(),FString(TEXT("30 FPS")));
    TestTrue(TEXT("Preset waits for Apply"),Dialog->bDirty);
    return true;
}
#endif
