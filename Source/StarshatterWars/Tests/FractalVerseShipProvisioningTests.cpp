#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "FractalVerseShipProvisioning.h"
#include "FractalVerseAuthority.h"
#include "GameStructs.h"
#include "ShipDesignRegistry.h"
#include "WeaponDesignRegistry.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "StarshatterShipDesignSubsystem.h"
#include "Engine/DataTable.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "JsonObjectConverter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFractalVerseShipPlanTest,"Starshatter.FractalVerse.Ships.AssignedHullAndRestartState",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FFractalVerseShipPlanTest::RunTest(const FString&)
{
    FS_PlayerGameInfo Career; FPlayerHangarShip Hull; Hull.HullId=FGuid::NewGuid(); Hull.Design=TEXT("Falcon"); Hull.Name=TEXT("Delta 1");
    Hull.bAssigned=true; Career.AssignedHangarHull=Hull.HullId; Career.HangarShips.Add(Hull);
    FFractalVersePilot Pilot; FString Reason; FFractalVersePlayerShip Plan;
    TestTrue(TEXT("Server career serialized"),FJsonObjectConverter::UStructToJsonObjectString(Career,Pilot.CareerJson));
    TestTrue(TEXT("Assigned mission hull provisioned"),FFractalVerseShipProvisioning::Plan(Pilot,0,Plan,Reason));
    TestEqual(TEXT("Exact hull identity retained"),Plan.Hull.HullId,Hull.HullId);
    FFractalVersePlayerShip Other; TestTrue(TEXT("Second spawn is valid"),FFractalVerseShipProvisioning::Plan(Pilot,1,Other,Reason));
    TestTrue(TEXT("Independent spawn positions"),!Other.Position.Equals(Plan.Position));
    FFractalVerseAuthority Server; const FName Account(TEXT("EOS:provision-test")); Server.EnsurePilot(Account,Reason);
    auto World=Server.Read(); Plan.Position=FVector(12,34,56); Plan.Velocity=FVector(10,20,30); Plan.Hull.bHasCondition=true; Plan.Hull.HullCondition=.6f;
    World.PlayerShips.Add(Account,Plan); TestTrue(TEXT("Native physical state restores"),Server.Restore(World,Reason));
    TestEqual(TEXT("Restart retains position"),Server.Read().PlayerShips[Account].Position,Plan.Position);
    TestEqual(TEXT("Restart retains velocity"),Server.Read().PlayerShips[Account].Velocity,Plan.Velocity);
    TestEqual(TEXT("Restart does not heal damage"),Server.Read().PlayerShips[Account].Hull.HullCondition,.6f);
    auto Invalid=World; Invalid.PlayerShips[Account].Heading=FVector::ZeroVector;
    TestFalse(TEXT("Malformed physics checkpoint rejected"),Server.Restore(Invalid,Reason));
    TestEqual(TEXT("Failed restore leaves live state intact"),Server.Read().PlayerShips[Account].Position,Plan.Position);
    Career.HangarShips[0].bLost=true; FJsonObjectConverter::UStructToJsonObjectString(Career,Pilot.CareerJson);
    TestFalse(TEXT("Lost hull never silently respawns"),FFractalVerseShipProvisioning::Plan(Pilot,0,Other,Reason));
    Career.HangarShips[0].bLost=false; Career.AssignedHangarHull=FGuid(); Career.SelectedHangarHull=FGuid();
    FJsonObjectConverter::UStructToJsonObjectString(Career,Pilot.CareerJson);
    TestFalse(TEXT("No unselected replacement hull is granted"),FFractalVerseShipProvisioning::Plan(Pilot,0,Other,Reason));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFractalVerseHullBoundsTest,"Starshatter.FractalVerse.Ships.HeadlessHullBounds",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FFractalVerseHullBoundsTest::RunTest(const FString&)
{
    const auto Before=ShipDesignRegistry::GetAll();
    const auto WeaponsBefore=WeaponDesignRegistry::GetAll();
    ON_SCOPE_EXIT {
        ShipDesignRegistry::Clear(); WeaponDesignRegistry::Clear();
        for(const auto& Pair:WeaponsBefore) WeaponDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);
        for(const auto& Pair:Before) ShipDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);
    };
    auto* Weapons=NewObject<UDataTable>();
    if(!UStarshatterWeaponDesignSubsystem::ImportJsonCatalog(*Weapons,FPaths::ProjectContentDir()/TEXT("GameData/Weapons/wep.json"))) return false;
    for(const auto& Pair:Weapons->GetRowMap()) WeaponDesignRegistry::RegisterDesign(Pair.Key,*reinterpret_cast<const FWeaponDesign*>(Pair.Value));
    auto* Catalog=NewObject<UDataTable>();
    if(!UStarshatterShipDesignSubsystem::ImportJsonCatalog(*Catalog,FPaths::ProjectContentDir()/TEXT("GameData/Ships/ships.json"))) return false;
    for(const auto& Pair:Catalog->GetRowMap()) ShipDesignRegistry::RegisterDesign(Pair.Key,*reinterpret_cast<const FShipDesign*>(Pair.Value));

    for(const auto& Pair:Catalog->GetRowMap())
    {
        const FName Name=Pair.Key;
        const auto* Row=ShipDesignRegistry::Find(Name);
        const auto* Design=ShipDesignRegistry::FindLegacy(Name);
        if(!TestNotNull(*Name.ToString(),Row) || !TestNotNull(TEXT("Native design exists"),Design)) return false;
        TestTrue(*FString::Printf(TEXT("%s has finite measured hull bounds"),*Name.ToString()),FMath::IsFinite(Row->CollisionRadius) && Row->CollisionRadius>0);
        TestEqual(TEXT("Headless collision uses authored physical radius"),Design->radius,Row->CollisionRadius);
    }
    return true;
}
#endif
