#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/DataTable.h"
#include "Misc/Paths.h"
#include "StarshatterShipDesignSubsystem.h"
#include "StarshatterSystemDesignSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShipSystemCalibrationTest, "Starshatter.Designs.SDERoleCalibration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FShipSystemCalibrationTest::RunTest(const FString&)
{
    auto* Ships = NewObject<UDataTable>(); auto* Systems = NewObject<UDataTable>();
    if (!TestTrue(TEXT("Native ship JSON ingestion"), UStarshatterShipDesignSubsystem::ImportJsonCatalog(*Ships,
            FPaths::ProjectContentDir() / TEXT("GameData/Ships/ships.json"))) ||
        !TestTrue(TEXT("Native component JSON ingestion"), UStarshatterSystemDesignSubsystem::ImportDefinitions(*Systems))) return false;
    const auto* Falcon = Ships->FindRow<FShipDesign>(TEXT("Falcon"), TEXT("Balance calibration"), false);
    const auto* Orion = Ships->FindRow<FShipDesign>(TEXT("Orion"), TEXT("Balance calibration"), false);
    const auto* Berents = Ships->FindRow<FShipDesign>(TEXT("Berents"), TEXT("Balance calibration"), false);
    const auto* ZolonDestroyer = Ships->FindRow<FShipDesign>(TEXT("ZZDD"), TEXT("Balance calibration"), false);
    if (!TestNotNull(TEXT("Falcon credit anchor exists"), Falcon) || !TestNotNull(TEXT("Carrier role exists"), Orion) ||
        !TestNotNull(TEXT("Unmodeled frigate remains in catalog"), Berents) || !TestNotNull(TEXT("Unmodeled destroyer remains in catalog"), ZolonDestroyer)) return false;
    TestEqual(TEXT("Falcon remains the SSW credit anchor, not an ISK price"), Falcon->BaseCost, int64(574000));
    TestEqual(TEXT("Carrier normalized role price is ingested"), Orion->BaseCost, int64(461926000));
    TestEqual(TEXT("Frigate price obeys bounded normalization"), Berents->BaseCost, int64(10190000));
    TestTrue(TEXT("Data-only Berents model stays absent"), Berents->Model.IsEmpty());
    TestTrue(TEXT("Data-only Zolon destroyer model stays absent"), ZolonDestroyer->Model.IsEmpty());
    if (!TestFalse(TEXT("Falcon physical reactor retained"), Falcon->Power.IsEmpty()) ||
        !TestFalse(TEXT("Carrier physical reactor retained"), Orion->Power.IsEmpty())) return false;
    TestEqual(TEXT("Falcon reactor uses SSW output"), double(Falcon->Power[0].Output), 65.0);
    TestEqual(TEXT("Carrier reactor uses SSW output"), double(Orion->Power[0].Output), 90000.0);
    int32 Components = 0;
    for (const auto& Pair : Systems->GetRowMap())
    {
        const auto& System = *reinterpret_cast<const FSystemDesign*>(Pair.Value);
        Components += System.Components.Num();
        TestEqual(TEXT("All five quality levels reach each system row"), System.ComponentGrades.Num(), 5);
        for (const auto& Grade : System.ComponentGrades)
        {
            const bool RailProfile = System.Name == TEXT("Bolt Weapon") || System.Name == TEXT("Plasma Weapon");
            const bool PulseProfile = System.Name == TEXT("Beam Weapon");
            const double PerformanceRatio = RailProfile ? 2.64 / 2.2 : PulseProfile ? 3.6 / 3.0 : 135.0 / 115.0;
            const double DemandRatio = RailProfile ? 8.0 / 7.0 : 11.0 / 10.0;
            // Pulse laser types lack basePrice in this SDE, so their price
            // curve retains the reviewed generic SSW fallback explicitly.
            const double PriceRatio = RailProfile ? 110528.0 / 9000.0 : 40566.0 / 6450.0;
            const double ExpectedPerformance = FMath::Pow(PerformanceRatio, (Grade.Level - 3) / 4.0);
            const double ExpectedDemand = FMath::Pow(DemandRatio, (Grade.Level - 3) / 4.0);
            const double ExpectedPrice = FMath::Pow(PriceRatio, (Grade.Level - 1) / 4.0);
            TestTrue(TEXT("Quality performance follows reviewed SSW curve"), FMath::IsNearlyEqual(Grade.PerformanceMultiplier, ExpectedPerformance, 1.e-8));
            TestTrue(TEXT("Better performance carries rated power demand"), FMath::IsNearlyEqual(Grade.PowerDemandMultiplier, ExpectedDemand, 1.e-8));
            TestTrue(TEXT("Credit tier follows normalized reference ratio"), FMath::IsNearlyEqual(Grade.PriceMultiplier, ExpectedPrice, 1.e-8));
            if (Grade.Level == 3)
            {
                TestEqual(TEXT("Grade III performance preserves current baseline"), Grade.PerformanceMultiplier, 1.0);
                TestEqual(TEXT("Grade III power preserves current baseline"), Grade.PowerDemandMultiplier, 1.0);
            }
        }
    }
    TestEqual(TEXT("Every authored component survives ingestion"), Components, 72);
    return true;
}
#endif
