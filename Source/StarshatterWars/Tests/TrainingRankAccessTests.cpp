#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Campaign.h"
#include "GameStructs.h"
#include "Engine/DataTable.h"
#include "PlayerCharacter.h"
#include "Misc/ScopeExit.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrainingRankAccessTest,"Starshatter.Campaign1.TrainingRankAccess",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTrainingRankAccessTest::RunTest(const FString& Parameters)
{
    auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_Campaign.DT_Campaign"));
    if(!TestNotNull(TEXT("Campaign table"),Table))return false;
    const FS_Campaign* Row=nullptr;
    for(const auto& Pair:Table->GetRowMap()) {auto* C=reinterpret_cast<const FS_Campaign*>(Pair.Value);if(C->Name.Equals(TEXT("Operation Live Fire"),ESearchCase::IgnoreCase)){Row=C;break;}}
    if(!TestNotNull(TEXT("Training campaign"),Row))return false;
    Campaign Training(1,"Rank access verification",true);Training.LoadFromData(*Row);
    PlayerCharacter Player;
    auto* Previous=PlayerCharacter::GetCurrentPlayer();
    ON_SCOPE_EXIT {PlayerCharacter::SetCurrentPlayer(Previous);};
    PlayerCharacter::SetCurrentPlayer(&Player);
    for(int32 Rank=0;Rank<=10;++Rank)
    {
        Player.SetRank(Rank);
        for(int32 Id=101;Id<=107;++Id)
        {
            const int32 Required=Id==104?1:Id==105?3:Id==106?4:Id==107?6:0;
            TestEqual(FString::Printf(TEXT("Mission %d at rank %d"),Id,Rank),Training.IsMissionRankUnlocked(Id),Rank>=Required);
        }
    }
    PlayerCharacter::SetCurrentPlayer(nullptr);
    TestFalse(TEXT("Missing player cannot accept ship training"),Training.IsMissionRankUnlocked(105));
    TestTrue(TEXT("Missing player can view beginner training"),Training.IsMissionRankUnlocked(101));
    return true;
}
#endif
