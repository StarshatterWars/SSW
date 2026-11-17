#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "DesignTableJson.h"
#include "Engine/DataTable.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "StarshatterShipDesignSubsystem.h"
#include "StarshatterWeaponDesignSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDesignJsonTablesTest, "Starshatter.Designs.JsonTableParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDesignJsonTablesTest::RunTest(const FString&)
{
    const auto* SavedShips = LoadObject<UDataTable>(nullptr, TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
    const auto* SavedWeapons = LoadObject<UDataTable>(nullptr, TEXT("/Game/Game/DT_WeaponDesign.DT_WeaponDesign"));
    if (!TestNotNull(TEXT("Saved ship table"), SavedShips) ||
        !TestNotNull(TEXT("Saved weapon table"), SavedWeapons)) return false;

    auto* Ships = NewObject<UDataTable>();
    auto* Weapons = NewObject<UDataTable>();
    const FString GameData = FPaths::ProjectContentDir() / TEXT("GameData");
    if (!TestTrue(TEXT("Import ship JSON"), UStarshatterShipDesignSubsystem::ImportJsonCatalog(*Ships, GameData / TEXT("Ships/ships.json"))) ||
        !TestTrue(TEXT("Import weapon JSON"), UStarshatterWeaponDesignSubsystem::ImportJsonCatalog(*Weapons, GameData / TEXT("Weapons/wep.json"))))
        return false;

    TestTrue(TEXT("Persisted ship table exactly matches JSON"), DesignTableJson::Compare(*SavedShips, *Ships));
    TestTrue(TEXT("Persisted weapon table exactly matches JSON"), DesignTableJson::Compare(*SavedWeapons, *Weapons));
    TestNotNull(TEXT("A key containing a space survives import"),
        Ships->FindRow<FShipDesign>(TEXT("Mining Station"), TEXT("JSON parity"), false));
    return true;
}

#endif
