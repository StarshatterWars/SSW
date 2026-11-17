#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "ShipInsuranceSubsystem.h"
#include "PlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerCharacter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHangarInsuranceTest, "Starshatter.Hangar.InsuranceAndPersistence",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FHangarInsuranceTest::RunTest(const FString&)
{
    FPlayerHangarShip Hull;
    Hull.HullId = FGuid::NewGuid();
    Hull.Design = TEXT("Falcon");
    Hull.Name = TEXT("Test Falcon");
    Hull.HullValue = 100000;
    Hull.InstalledComponentValue = 20000;
    const int64 Value = 120000;
    for (int32 Index = 0; Index <= 6; ++Index)
    {
        const auto Quote = UShipInsuranceSubsystem::Quote(Hull, static_cast<EShipInsuranceTier>(Index));
        TestEqual(TEXT("EVE premium fraction"), Quote.Premium, Value * Index / 20);
        TestEqual(TEXT("EVE coverage fraction"), Quote.Payout, Value * (4 + Index) / 10);
    }
    Hull.Insurance.Tier = EShipInsuranceTier::Platinum;
    Hull.Insurance.StartsUtc = 100;
    Hull.Insurance.ExpiresUtc = 200;
    TestEqual(TEXT("Policy active before expiry"), UShipInsuranceSubsystem::EffectiveTier(Hull, 199), EShipInsuranceTier::Platinum);
    TestEqual(TEXT("Policy expires at boundary"), UShipInsuranceSubsystem::EffectiveTier(Hull, 200), EShipInsuranceTier::Uninsured);
    Hull.Insurance.bSettled = true;
    TestEqual(TEXT("Settled policy cannot pay again"), UShipInsuranceSubsystem::EffectiveTier(Hull, 150), EShipInsuranceTier::Uninsured);
    Hull.bCashPurchased = true;
    TestEqual(TEXT("Cash hull remains lifetime after expiry"), UShipInsuranceSubsystem::EffectiveTier(Hull, MAX_int64), EShipInsuranceTier::Lifetime);
    const auto Lifetime = UShipInsuranceSubsystem::Quote(Hull, EShipInsuranceTier::Basic);
    TestEqual(TEXT("Lifetime has no premium"), Lifetime.Premium, 0LL);
    TestEqual(TEXT("Lifetime covers hull and components"), Lifetime.Payout, Value);
    Hull.Components.AddDefaulted_GetRef().Level = 5;
    Hull.LoadoutStations = {1, 2, -1};
    Hull.LoadoutName = TEXT("Custom");
    auto* Save = NewObject<UPlayerSaveGame>();
    Save->PlayerInfo.HangarShips.Add(Hull);
    Save->PlayerInfo.SelectedHangarHull = Hull.HullId;
    Save->PlayerInfo.InsuranceCredits = 123456;
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("Save hangar in memory"), UGameplayStatics::SaveGameToMemory(Save, Bytes))) return false;
    const auto* Loaded = Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Read saved hangar"), Loaded)) return false;
    TestEqual(TEXT("One hull survives"), Loaded->PlayerInfo.HangarShips.Num(), 1);
    TestEqual(TEXT("Selected identity survives"), Loaded->PlayerInfo.SelectedHangarHull, Hull.HullId);
    TestTrue(TEXT("Cash provenance survives"), Loaded->PlayerInfo.HangarShips[0].bCashPurchased);
    TestEqual(TEXT("Fitted level survives"), Loaded->PlayerInfo.HangarShips[0].Components[0].Level, 5);
    TestEqual(TEXT("Stored loadout survives"), Loaded->PlayerInfo.HangarShips[0].LoadoutStations.Num(), 3);
    TestEqual(TEXT("Stored empty hardpoint survives"), Loaded->PlayerInfo.HangarShips[0].LoadoutStations[2], -1);
    TestEqual(TEXT("Balance survives"), Loaded->PlayerInfo.InsuranceCredits, 123456LL);
    TestTrue(TEXT("Frigates require a command rank"), PlayerCharacter::CommandRankRequired(static_cast<int32>(CLASSIFICATION::FRIGATE)) >= 3);
    return true;
}
#endif
