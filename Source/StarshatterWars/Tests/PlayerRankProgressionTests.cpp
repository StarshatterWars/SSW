#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "AwardInfoRegistry.h"
#include "PlayerCharacter.h"
#include "SimEvent.h"
#include "Game.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerRankProgressionTest,"Starshatter.Audit.F05.RankProgression",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPlayerRankProgressionTest::RunTest(const FString& Parameters)
{
    const auto OldRanks=UAwardInfoRegistry::RankTable;const auto OldMedals=UAwardInfoRegistry::MedalTable;
    UObject* OldContext=PlayerCharacter::GWorldContextForSave;
    ON_SCOPE_EXIT { UAwardInfoRegistry::Initialize(OldRanks.Get(),OldMedals.Get());PlayerCharacter::GWorldContextForSave=OldContext; };
    PlayerCharacter::GWorldContextForSave=nullptr; // No profile writes from automation.
    UDataTable* Table=NewObject<UDataTable>();Table->RowStruct=FRankInfo::StaticStruct();
    FRankInfo Row;Row.RankId=7;Row.TotalPoints=1000;Table->AddRow(TEXT("Senior"),Row);
    Row.RankId=0;Row.TotalPoints=0;Table->AddRow(TEXT("Cadet"),Row);
    Row.RankId=2;Row.TotalPoints=100;Table->AddRow(TEXT("Junior"),Row);
    UAwardInfoRegistry::Initialize(Table,nullptr);
    PlayerCharacter Player;Player.SetPoints(99);
    TestEqual(TEXT("Below promotion threshold"),Player.GetRank(),0);
    Player.AddPoints(1);TestEqual(TEXT("Exact threshold promotes"),Player.GetRank(),2);
    Player.AddPoints(1000);TestEqual(TEXT("Can cross multiple ranks without row order dependence"),Player.GetRank(),7);
    Player.SetRank(2);TestEqual(TEXT("Explicit rank sets legacy threshold points"),Player.GetPoints(),100);
    FS_PlayerGameInfo Saved;Player.ToPlayerInfo(Saved);Saved.Rank=7;Saved.PlayerPoints=99;
    PlayerCharacter Reloaded;Reloaded.FromPlayerInfo(Saved);
    TestEqual(TEXT("Loaded stale cached rank is corrected from points"),Reloaded.GetRank(),0);
    Reloaded.ToPlayerInfo(Saved);TestEqual(TEXT("Save writes current derived rank"),Saved.Rank,0);
    Player.SetPoints(99);Player.ClearShowAward();
    ShipStats Stats("F05_Audit");Stats.AddPoints(1);
    Player.ProcessStats(&Stats,Game::GetGameTime());
    TestEqual(TEXT("Mission statistics promote player"),Player.GetRank(),2);
    TestTrue(TEXT("Promotion queues rank ceremony"),Player.ShowAward() && Player.IsPendingAwardRank() && Player.GetPendingAwardId()==2);
    Player.SetPoints(MAX_int32);Player.AddPoints(1);TestEqual(TEXT("Points cannot overflow into a demotion"),Player.GetPoints(),MAX_int32);
    UAwardInfoRegistry::Reset();Player.SetRank(4);TestEqual(TEXT("Unloaded table preserves profile fallback"),Player.GetRank(),4);
    UAwardInfoRegistry::Initialize(Table,nullptr);TestEqual(TEXT("Loading table resumes points authority"),Player.GetRank(),7);
    return true;
}
#endif
