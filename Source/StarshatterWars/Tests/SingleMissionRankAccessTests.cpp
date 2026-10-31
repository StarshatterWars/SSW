#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "MissionRankRules.h"
#include "Engine/DataTable.h"
#include "WeaponDesignRegistry.h"
#include "Misc/ScopeExit.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSingleMissionRankAccessTest,"Starshatter.SingleMissions.RankAccess",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSingleMissionRankAccessTest::RunTest(const FString& Parameters)
{
    auto* Missions=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_SingleMissions.DT_SingleMissions"));
    auto* Designs=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
    if(!TestNotNull(TEXT("Single missions"),Missions)||!TestNotNull(TEXT("Ship designs"),Designs))return false;
    const auto Original=ShipDesignRegistry::GetAll();
    const auto OriginalWeapons=WeaponDesignRegistry::GetAll();
    auto* Weapons=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_WeaponDesign.DT_WeaponDesign"));
    if(!TestNotNull(TEXT("Weapon designs"),Weapons))return false;
    for(const auto& Pair:Weapons->GetRowMap())WeaponDesignRegistry::RegisterDesign(Pair.Key,*reinterpret_cast<const FWeaponDesign*>(Pair.Value));
    auto* Previous=PlayerCharacter::GetCurrentPlayer();
    ON_SCOPE_EXIT {PlayerCharacter::SetCurrentPlayer(Previous);ShipDesignRegistry::Clear();WeaponDesignRegistry::Clear();for(const auto& Pair:OriginalWeapons)WeaponDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);for(const auto& Pair:Original)ShipDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);};
    for(const auto& Pair:Designs->GetRowMap())ShipDesignRegistry::RegisterDesign(Pair.Key,*reinterpret_cast<const FShipDesign*>(Pair.Value));
    PlayerCharacter Player;PlayerCharacter::SetCurrentPlayer(&Player);
    const int32 Expected[]={0,0,0,0,1,1,3,3,4,6};
    TSet<int32> Found;
    for(const auto& Pair:Missions->GetRowMap())
    {
        const auto& Mission=*reinterpret_cast<const FS_CampaignMission*>(Pair.Value);
        if(Mission.Id<1 || Mission.Id>10)continue;
        Found.Add(Mission.Id);
        const int32 Required=MissionRankRules::Required(Mission);
        TestEqual(FString::Printf(TEXT("Mission %d ship command rank"),Mission.Id),Required,Expected[Mission.Id-1]);
        for(int32 Rank=0;Rank<=10;++Rank)
        {
            Player.SetRank(Rank);
            TestEqual(FString::Printf(TEXT("Mission %d at player rank %d"),Mission.Id,Rank),MissionRankRules::Unlocked(Required),Rank>=Expected[Mission.Id-1]);
        }
    }
    TestEqual(TEXT("All ten standalone missions checked"),Found.Num(),10);
    return true;
}
#endif
