#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Kismet/GameplayStatics.h"
#include "LocalGameStateSave.h"
#include "JsonObjectConverter.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldCheckpointInspectionTest,"Starshatter.Persistence.InspectWorldCheckpoint",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWorldCheckpointInspectionTest::RunTest(const FString&)
{
    FString File;
    if(!FParse::Value(FCommandLine::Get(),TEXT("SSWInspectWorld="),File))return true;
    TArray<uint8> Bytes;
    if(!TestTrue(TEXT("Read checkpoint without modifying it"),FFileHelper::LoadFileToArray(Bytes,*File)))return false;
    TStrongObjectPtr<ULocalUniverseState> State(Cast<ULocalUniverseState>(UGameplayStatics::LoadGameFromMemory(Bytes)));
    if(!TestNotNull(TEXT("Checkpoint is a native universe save"),State.Get()))return false;
    FString Json;
    if(!TestTrue(TEXT("Serialize diagnostic copy"),FJsonObjectConverter::UStructToJsonObjectString(State->GetClass(),State.Get(),Json)))return false;
    const FString Output=FPaths::ProjectSavedDir()/TEXT("Diagnostics/WorldCheckpoint.json");
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Output),true);
    TestTrue(TEXT("Write read-only inspection report"),FFileHelper::SaveStringToFile(Json,*Output));
    AddInfo(FString::Printf(TEXT("Checkpoint inspection: %s"),*Output));
    return true;
}
#endif
