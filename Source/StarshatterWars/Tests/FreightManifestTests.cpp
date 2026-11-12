#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/DataTable.h"
#include "FreightManifest.h"
#include "GameStructs.h"
#include "GameStructs_System.h"
#include "Game.h"
#include "Instruction.h"
#include "SimRegion.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreightManifestTest, "Starshatter.Freight.ManifestAndSensorRange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreightManifestTest::RunTest(const FString&)
{
    const UDataTable* Table = LoadObject<UDataTable>(nullptr, TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
    if (!TestNotNull(TEXT("Ship data table"), Table)) return false;

    for (const TCHAR* Design : {TEXT("IM3500"), TEXT("IM4300"), TEXT("IM2800"), TEXT("IM1906"),
        TEXT("IM1912"), TEXT("Courier"), TEXT("Transport"), TEXT("QShip")})
    {
        const FShipDesign* Row = Table->FindRow<FShipDesign>(FName(Design), TEXT("Cargo test"));
        if (!TestNotNull(Design, Row)) continue;
        TestTrue(TEXT("Cargo capacity is positive"), Row->CargoCapacity.Tonnes > 0);
        const auto Load = SSWFreightManifest::Assign(Row->CargoManifest, Row->CargoCapacity, "Golden Hart", "Reg-1");
        if (!TestTrue(*FString::Printf(TEXT("%s carries cargo"), Design), Load.IsSet())) continue;
        const auto Repeat = SSWFreightManifest::Assign(Row->CargoManifest, Row->CargoCapacity, "Golden Hart", "Reg-1");
        TestEqual(TEXT("Same ship yields the same cargo item"), Repeat->Item, Load->Item);
        TestEqual(TEXT("Same ship yields the same quantity"), Repeat->Quantity, Load->Quantity);
        TestTrue(TEXT("Quantity is positive"), Load->Quantity > 0);
        if (Load->Unit == TEXT("t"))
            TestTrue(TEXT("Load does not exceed hold capacity"), Load->Quantity <= Row->CargoCapacity.Tonnes);
    }
    const FShipDesign* Transport = Table->FindRow<FShipDesign>(FName(TEXT("Transport")), TEXT("Cargo test"));
    if (TestNotNull(TEXT("Transport"), Transport))
    {
        TestTrue(TEXT("Transport has passenger berths"), Transport->CargoCapacity.PassengerBerths > 0);
        TestTrue(TEXT("Transport can carry passengers"), Transport->CargoManifest.ContainsByPredicate(
            [](const FSSWCargoOption& Option) { return Option.bPassengers && Option.Item == TEXT("passengers"); }));
        TestTrue(TEXT("Finished goods retain their enum value"), Transport->CargoManifest.ContainsByPredicate(
            [](const FSSWCargoOption& Option) { return Option.FinishedItem == ESSWFinishedItem::MedicalSupplies; }));
    }
    const FShipDesign* Falcon = Table->FindRow<FShipDesign>(FName(TEXT("Falcon")), TEXT("Cargo test"));
    if (TestNotNull(TEXT("Falcon"), Falcon))
    {
        TestTrue(TEXT("Falcon capacity survives import as a fractional tonne"),
            Falcon->CargoCapacity.Tonnes > 0.0 && Falcon->CargoCapacity.Tonnes < 1.0);
        TestTrue(TEXT("Fighters have no cargo options"), Falcon->CargoManifest.IsEmpty());
    }

    const uint32 ShipClasses = static_cast<uint32>(CLASSIFICATION::DROPSHIPS) |
        (static_cast<uint32>(CLASSIFICATION::STARSHIPS) & ~static_cast<uint32>(CLASSIFICATION::FARCASTER));
    for (const FName Name : Table->GetRowNames())
    {
        const FShipDesign* Row = Table->FindRow<FShipDesign>(Name, TEXT("Cargo coverage"));
        if (Row && (static_cast<uint32>(Row->ShipType) & ShipClasses))
            TestTrue(*FString::Printf(TEXT("%s has cargo capacity"), *Name.ToString()), Row->CargoCapacity.Tonnes > 0.0);
    }

    SimRegion Departure(nullptr, "Nero", SimRegion::REAL_SPACE);
    SimRegion Arrival(nullptr, "Vesta", SimRegion::REAL_SPACE);
    const FVector AuthoredPosition(90000, -60000, 0);
    Instruction LastWaypoint(&Arrival, AuthoredPosition, EInstruction::Dock);
    LastWaypoint.SetTarget("Vesta Transfer Station");

    SSWFreightManifest::FLoad RouteLoad;
    RouteLoad.CaptureOrigin(Departure, FVector(100, 200, 300));
    RouteLoad.CaptureOrigin(Arrival, FVector::ZeroVector);
    RouteLoad.CaptureDestination(LastWaypoint);
    Instruction ReplacementWaypoint(&Departure, FVector::ZeroVector);
    RouteLoad.CaptureDestination(ReplacementWaypoint);
    if (TestTrue(TEXT("Freight route has both endpoints"), RouteLoad.Origin.IsSet() && RouteLoad.Destination.IsSet()))
    {
        TestEqual(TEXT("Origin stays at the spawn sector after travel"), RouteLoad.Origin->Sector, FString(TEXT("Nero")));
        TestEqual(TEXT("Destination uses final waypoint sector"), RouteLoad.Destination->Sector, FString(TEXT("Vesta")));
        TestEqual(TEXT("Destination preserves named berth"), RouteLoad.Destination->Site, FString(TEXT("Vesta Transfer Station")));
        TestEqual(TEXT("Destination preserves region coordinates"), RouteLoad.Destination->RegionPosition, OtherHand(AuthoredPosition));
    }

    TestTrue(TEXT("Manifest resolves at the scan limit"), SSWFreightManifest::CanResolve(5000, 20000));
    TestFalse(TEXT("Manifest is unresolved beyond the scan limit"), SSWFreightManifest::CanResolve(5001, 20000));
    TestFalse(TEXT("Long-range sensors still require proximity"), SSWFreightManifest::CanResolve(25001, 500000));
    TestFalse(TEXT("No available sensor range"), SSWFreightManifest::CanResolve(100, 0));
    TestFalse(TEXT("Invalid contact distance"), SSWFreightManifest::CanResolve(-1, 20000));
    return true;
}

#endif
