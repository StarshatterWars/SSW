#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "FreeFlightHullRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreeFlightOwnedHullTest,"Starshatter.Player.OwnedFreeFlightHullSelection",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFreeFlightOwnedHullTest::RunTest(const FString&)
{
    const auto Previous=ShipDesignRegistry::GetAll();
    ON_SCOPE_EXIT {ShipDesignRegistry::Clear();for(const auto& Pair:Previous)ShipDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);};
    FShipDesign Fighter;Fighter.ShipName=TEXT("Falcon");
    FShipDesign Miner;Miner.ShipName=TEXT("Prospector");
    ShipDesignRegistry::RegisterDesign(FName(TEXT("Falcon")),Fighter);
    ShipDesignRegistry::RegisterDesign(FName(TEXT("Prospector")),Miner);
    // Supply every design referenced by the real entry definition; this test
    // exercises hull selection without depending on another test warming registries.
    for (const TCHAR* Name : {TEXT("Orion"), TEXT("Station1"), TEXT("Mining Station"), TEXT("Asteroid Base"), TEXT("Processing Facility")})
        if (!ShipDesignRegistry::Find(FName(Name)))
        {
            FShipDesign Support; Support.ShipName=Name;
            ShipDesignRegistry::RegisterDesign(FName(Name),Support);
        }
    FString Definition;
    if(!TestTrue(TEXT("Career entry definition readable"),FFileHelper::LoadFileToString(Definition,*(FPaths::ProjectContentDir()/TEXT("GameData/Campaigns/01/Scenes/FreeFlight.def")))))return false;
    Mission Free(Mission::FreeFlightId);if(!TestTrue(TEXT("Career entry parses"),Free.ParseMission(TCHAR_TO_UTF8(*Definition))))return false;
    FS_PlayerGameInfo Pilot;Pilot.Rank=PlayerCharacter::HullRankRequired(Miner.ShipType,Miner.ShipName);auto& Hull=Pilot.HangarShips.AddDefaulted_GetRef();Hull.HullId=FGuid::NewGuid();Hull.Design=TEXT("Prospector");Pilot.SelectedHangarHull=Hull.HullId;
    TestTrue(TEXT("Owned starter selection applies"),FreeFlightHullRules::Apply(Free,Pilot));
    TestTrue(TEXT("Career entry uses selected mining shuttle"),Free.GetPlayer()->GetShipDesign()==ShipDesignRegistry::Find(FName(TEXT("Prospector"))));
    Hull.bLost=true;FreeFlightHullRules::Apply(Free,Pilot);
    TestTrue(TEXT("Lost selected hull falls back to Falcon"),Free.GetPlayer()->GetShipDesign()==ShipDesignRegistry::Find(FName(TEXT("Falcon"))));
    Mission Assigned(101);
    TestFalse(TEXT("Training assignments are untouched"),FreeFlightHullRules::Apply(Assigned,Pilot));
    return true;
}
#endif
