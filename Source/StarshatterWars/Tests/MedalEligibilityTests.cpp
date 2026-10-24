#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "MedalEligibility.h"
#include "AwardInfoRegistry.h"
#include "StarshatterGameDataSubsystem.h"
#include "PlayerCharacter.h"
#include "SimEvent.h"
#include "Engine/GameInstance.h"
#include "Game.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMedalEligibilityTest,"Starshatter.Audit.F06.MedalEligibility",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMedalEligibilityTest::RunTest(const FString& Parameters)
{
    FMedalInfo M;M.MedalId=0x20;M.bEligibilityConfigured=true;
    M.MinRank=2;M.MaxRank=6;M.RequiredAwards=0x3;M.MinShipClass=4;M.MaxShipClass=16;
    M.TotalPoints=1000;M.TotalMissions=15;M.MissionPoints=20;M.Kills=3;M.Lost=-1;
    FMedalEligibilityContext C;C.Rank=3;C.Medals=3;C.ShipClass=8;C.TotalPoints=1000;C.TotalMissions=15;C.MissionPoints=20;C.Kills=3;
    TestTrue(TEXT("All authored criteria satisfied"),MedalEligibility::MeetsRequirements(M,C));
    auto Reject=[&](const TCHAR* Label,FMedalEligibilityContext Changed){TestFalse(Label,MedalEligibility::MeetsRequirements(M,Changed));};
    auto X=C;X.Rank=1;Reject(TEXT("Minimum rank"),X);X=C;X.Rank=7;Reject(TEXT("Maximum rank"),X);
    X=C;X.Medals=1;Reject(TEXT("All prerequisite flags required"),X);X=C;X.Medals|=0x20;Reject(TEXT("No duplicate medal"),X);
    X=C;X.ShipClass=2;Reject(TEXT("Minimum ship class"),X);X=C;X.ShipClass=32;Reject(TEXT("Maximum ship class"),X);
    X=C;X.TotalPoints=999;Reject(TEXT("Career points"),X);X=C;X.TotalMissions=14;Reject(TEXT("Career missions"),X);
    X=C;X.MissionPoints=19;Reject(TEXT("Mission merit"),X);X=C;X.Kills=2;Reject(TEXT("Mission kills"),X);
    X=C;X.Deaths=1;Reject(TEXT("Survival excludes death"),X);X=C;X.Collisions=1;Reject(TEXT("Survival excludes collision"),X);
    M.bDynamicCampaignOnly=true;X=C;X.bHasCampaign=true;Reject(TEXT("Dynamic campaign restriction"),X);
    X.bDynamicCampaign=true;TestTrue(TEXT("Dynamic campaign accepted"),MedalEligibility::MeetsRequirements(M,X));
    M.CampaignId=2;M.bCampaignCompleteRequired=true;Reject(TEXT("Recorded campaign completion required"),X);
    X.bCompletedCampaign=true;X.bDynamicCampaign=false;TestTrue(TEXT("Completed campaign history takes legacy priority over current campaign"),MedalEligibility::MeetsRequirements(M,X));
    M.CampaignId=0;M.bCampaignCompleteRequired=false;M.bDynamicCampaignOnly=false;M.Lost=1;
    TestFalse(TEXT("Wound medal cannot be awarded without loss"),MedalEligibility::MeetsRequirements(M,C));
    X=C;X.Deaths=1;TestTrue(TEXT("Wound condition accepted"),MedalEligibility::MeetsRequirements(M,X));
    M.bEligibilityConfigured=false;TestFalse(TEXT("Unmigrated medal rows cannot auto-grant"),MedalEligibility::MeetsRequirements(M,X));

    const auto OldRanks=UAwardInfoRegistry::RankTable,OldMedals=UAwardInfoRegistry::MedalTable;
    UObject* OldContext=PlayerCharacter::GWorldContextForSave;
    ON_SCOPE_EXIT {UAwardInfoRegistry::Initialize(OldRanks.Get(),OldMedals.Get());PlayerCharacter::GWorldContextForSave=OldContext;};
    PlayerCharacter::GWorldContextForSave=nullptr;
    UGameInstance* GI=NewObject<UGameInstance>();
    auto* Data=NewObject<UStarshatterGameDataSubsystem>(GI);
    Data->RanksDataTable=NewObject<UDataTable>(Data);Data->RanksDataTable->RowStruct=FRankInfo::StaticStruct();
    Data->MedalsDataTable=NewObject<UDataTable>(Data);Data->MedalsDataTable->RowStruct=FMedalInfo::StaticStruct();
    UDataTable* Original=Data->MedalsDataTable;
    Data->LoadAwardTables();
    TestTrue(TEXT("Authored parser uses private transient tables"),Data->MedalsDataTable!=Original && Original->GetRowMap().IsEmpty());
    UAwardInfoRegistry::Initialize(Data->RanksDataTable,Data->MedalsDataTable);
    const FMedalInfo* Flight=UAwardInfoRegistry::FindMedal(0x40);
    if(TestNotNull(TEXT("Flight Medal parsed from Awards.def"),Flight))
    {
        TestEqual(TEXT("Authored kill requirement preserved"),Flight->Kills,3);
        TestEqual(TEXT("Authored career mission requirement preserved"),Flight->TotalMissions,15);
        TestEqual(TEXT("Authored survival requirement preserved"),Flight->Lost,-1);
        TestEqual(TEXT("Authored lottery preserved"),Flight->Lottery,10);
    }
    UDataTable* Medals=NewObject<UDataTable>();Medals->RowStruct=FMedalInfo::StaticStruct();
    FMedalInfo Silent;Silent.MedalId=1;Silent.bCeremony=false;Silent.bEligibilityConfigured=true;
    Medals->AddRow(TEXT("Silent"),Silent);FMedalInfo Ceremony=Silent;Ceremony.MedalId=2;Ceremony.bCeremony=true;Medals->AddRow(TEXT("Ceremony"),Ceremony);
    UAwardInfoRegistry::Initialize(Data->RanksDataTable,Medals);
    PlayerCharacter Player;Player.SetRank(0);ShipStats Stats("F06_Audit");
    Player.ProcessStats(&Stats,Game::GetGameTime());
    TestEqual(TEXT("Silent and first ceremonial medals receive exact flags"),Player.GetMedals(),3);
    TestTrue(TEXT("Only ceremonial medal queues a dialog"),Player.ShowAward()&&!Player.IsPendingAwardRank()&&Player.GetPendingAwardId()==2);
    TestFalse(TEXT("Already earned flag is not awarded twice"),Player.EarnedAward(1,false,&Stats));
    return true;
}
#endif
