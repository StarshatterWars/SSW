#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "TrainingCompletion.h"
#include "PlayerCharacter.h"
#include "AwardInfoRegistry.h"
#include "StarshatterGameDataSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "SimEvent.h"
#include "Misc/ScopeExit.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrainingCompletionAwardTest,"Starshatter.Training.CompletionAward",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTrainingCompletionAwardTest::RunTest(const FString&)
{
    const auto OldRanks=UAwardInfoRegistry::RankTable, OldMedals=UAwardInfoRegistry::MedalTable;
    UObject* OldContext=PlayerCharacter::GWorldContextForSave;
    ON_SCOPE_EXIT { UAwardInfoRegistry::Initialize(OldRanks.Get(),OldMedals.Get());PlayerCharacter::GWorldContextForSave=OldContext; };
    PlayerCharacter::GWorldContextForSave=nullptr;
    auto* Data=NewObject<UStarshatterGameDataSubsystem>(NewObject<UGameInstance>());Data->LoadAwardsJson();
    FS_PlayerGameInfo State;State.Campaign=1;State.Rank=1;
    for(int32 Lesson=101;Lesson<=103;++Lesson)State.MarkMissionCompleted(1,Lesson);
    TestFalse(TEXT("Promotion alone does not complete training"),TrainingCompletion::Record(State));
    TestFalse(TEXT("No completion flag before T4"),State.IsCampaignComplete(1));
    State.MarkMissionCompleted(1,104);
    TestTrue(TEXT("All four lessons complete the campaign"),TrainingCompletion::Record(State));
    PlayerCharacter Pilot;Pilot.FromPlayerInfo(State);ShipStats Stats("Training award fixture");
    TestTrue(TEXT("Campaign medal evaluates successfully"),Pilot.EarnedAward(8,false,&Stats));
    TestFalse(TEXT("Repeated evaluation does not duplicate medal or credit"),Pilot.EarnedAward(8,false,&Stats));
    FS_PlayerGameInfo Saved;Pilot.ToPlayerInfo(Saved);PlayerCharacter Reloaded;Reloaded.FromPlayerInfo(Saved);
    TestTrue(TEXT("Completion survives profile round trip"),Reloaded.HasCompletedCampaign(1));
    TestEqual(TEXT("Medal remains enumerated for logbook display"),Reloaded.Medal(0),8);
    if(const auto* Medal=UAwardInfoRegistry::FindMedal(8))
        TestEqual(TEXT("Correct training award display name"),Medal->Name,FString(TEXT("Basic Flight Training")));
    else AddError(TEXT("Training medal missing from JSON"));
    return true;
}
#endif
