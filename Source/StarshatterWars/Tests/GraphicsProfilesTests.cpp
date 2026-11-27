#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GraphicsProfiles.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGraphicsProfilesTest,"Starshatter.Video.GraphicsProfilePolicies",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FGraphicsProfilesTest::RunTest(const FString&)
{
    TestTrue(TEXT("Small output retains Full HD UI target"),SSWGraphicsProfiles::OutputSize({1280,720})==FIntPoint(1920,1080));
    TestTrue(TEXT("4K output is preserved"),SSWGraphicsProfiles::OutputSize({3840,2160})==FIntPoint(3840,2160));
    TestTrue(TEXT("Invalid output safely uses Full HD"),SSWGraphicsProfiles::OutputSize({-1,0})==FIntPoint(1920,1080));
    for(int32 I=0;I<SSWGraphicsProfiles::Count;++I)
    {
        const auto& P=SSWGraphicsProfiles::Get(I);
        TestTrue(TEXT("3D scale stays bounded independently of UI"),P.RenderPercent>=50 && P.RenderPercent<=100);
        for(int32 Quality:P.Quality)TestTrue(TEXT("Quality is a supported Unreal level"),Quality>=0 && Quality<=4);
    }
    const auto& Light=SSWGraphicsProfiles::Get(0);
    TestEqual(TEXT("Integrated GI is off"),Light.Quality[6],0);
    TestEqual(TEXT("Integrated reflections are off"),Light.Quality[7],0);
    TestTrue(TEXT("Hull texture quality remains readable"),Light.Quality[3]>=2);
    TestEqual(TEXT("Frame pacing target is 30 FPS"),Light.FrameLimit,30.f);
    return true;
}
#endif
