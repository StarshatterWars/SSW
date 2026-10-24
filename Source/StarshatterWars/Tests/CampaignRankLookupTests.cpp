#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Misc/ScopeExit.h"
#include "StarshatterGameDataSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "AwardInfoRegistry.h"
#include "PlayerData.h"
#include "PlayerCharacter.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignRankLookupTest,"Starshatter.Audit.F14.CampaignRankLookup",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampaignRankLookupTest::RunTest(const FString& Parameters)
{
    const auto OldRanks=UAwardInfoRegistry::RankTable; const auto OldMedals=UAwardInfoRegistry::MedalTable;
    ON_SCOPE_EXIT { UAwardInfoRegistry::Initialize(OldRanks.Get(),OldMedals.Get()); };
    auto* Data=NewObject<UStarshatterGameDataSubsystem>(NewObject<UGameInstance>());
    Data->RanksDataTable=NewObject<UDataTable>(Data);Data->RanksDataTable->RowStruct=FRankInfo::StaticStruct();
    Data->MedalsDataTable=NewObject<UDataTable>(Data);Data->MedalsDataTable->RowStruct=FMedalInfo::StaticStruct();
    Data->LoadAwardTables();UAwardInfoRegistry::Initialize(Data->RanksDataTable,Data->MedalsDataTable);
    FString Definition;
    TestTrue(TEXT("Authored rank definitions exist"),FFileHelper::LoadFileToString(Definition,*(FPaths::ProjectContentDir()/TEXT("GameData/Awards/Awards.def"))));
    // Verify every loaded authored rank, including admiral grades omitted by the old implementation.
    int32 Found=0;
    for(int32 I=0;I<128;++I) if(const FRankInfo* Rank=UAwardInfoRegistry::FindRank(I))
    {
        ++Found;
        TestEqual(TEXT("Legacy helper full rank name"),FString(UTF8_TO_TCHAR(PlayerCharacter::RankName(I))),Rank->Name);
        TestEqual(TEXT("Legacy helper rank description"),FString(UTF8_TO_TCHAR(PlayerCharacter::RankDescription(I))),FString(UAwardInfoRegistry::RankDescription(I)));
        TestEqual(TEXT("Legacy helper abbreviation"),FString(UTF8_TO_TCHAR(PlayerCharacter::RankAbrv(I))),FString(UAwardInfoRegistry::RankAbrv(I)));
        TestEqual(*FString::Printf(TEXT("Campaign full name %s"),*Rank->Name),PlayerData::RankFromName(TCHAR_TO_ANSI(*Rank->Name)),I);
        TestEqual(TEXT("Case and surrounding whitespace"),PlayerCharacter::RankFromName(TEXT(" ")+Rank->Name.ToLower()+TEXT(" ")),I);
        if(!Rank->Abrv.IsEmpty()) TestEqual(TEXT("Rank abbreviation"),UAwardInfoRegistry::RankFromName(Rank->Abrv),I);
    }
    TestTrue(TEXT("All eleven legacy rank grades loaded"),Found>=11);
    UAwardInfoRegistry::ForEachMedal([&](const FMedalInfo& Medal){
        TestEqual(TEXT("Legacy medal name"),FString(UTF8_TO_TCHAR(PlayerCharacter::MedalName(Medal.MedalId))),FString(UAwardInfoRegistry::MedalName(Medal.MedalId)));
        TestEqual(TEXT("Legacy medal description"),FString(UTF8_TO_TCHAR(PlayerCharacter::MedalDescription(Medal.MedalId))),FString(UAwardInfoRegistry::MedalDescription(Medal.MedalId)));
    });
    TestEqual(TEXT("Unknown names preserve legacy fallback"),PlayerData::RankFromName("not-a-rank"),0);
    TestEqual(TEXT("Null name is safe"),PlayerData::RankFromName(nullptr),0);
    return true;
}
#endif
