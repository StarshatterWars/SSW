#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "ReleasePresentation.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReleasePresentationTest,"Starshatter.Menu.AlphaReleasePresentation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FReleasePresentationTest::RunTest(const FString&)
{
 FString Json;TestTrue(TEXT("Canonical release metadata exists for cooking and launcher publishing"),FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("Build/release-info.json"))));
 const auto Info=SSWReleasePresentation::Read();
 TestEqual(TEXT("Current milestone"),Info.Stage,FString(TEXT("Alpha 5")));
 TestTrue(TEXT("Warning names crashes and save incompatibilities"),Info.Warning.Contains(TEXT("crashes")) && Info.Warning.Contains(TEXT("save incompatibilities")));
 TestEqual(TEXT("Game version format matches launcher game build"),Info.Version(TEXT("20261120-testZ")),FString(TEXT("Alpha 5 | Build 20261120-testZ")));
 return true;
}
#endif
