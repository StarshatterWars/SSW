#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "StarshatterPlayerSubsystem.h"
#include "TransactionAI.h"
#include "PlayerSaveGame.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNewCareerWalletTest,"Starshatter.Economy.Transactions.NewCareerStartingAllowance",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FNewCareerWalletTest::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> Owner(NewObject<UGameInstance>());
    TStrongObjectPtr<UStarshatterPlayerSubsystem> Players(NewObject<UStarshatterPlayerSubsystem>(Owner.Get()));
    Players->ResetToDefaults();
    auto& Economy=Players->GetEconomy();
    TestEqual(TEXT("New pilot starts with exactly 50000 credits"),TransactionAI::Balance(Economy),50000LL);
    TestEqual(TEXT("Starting allowance has one wallet receipt"),Economy.Journal.Num(),1);
    TestTrue(TEXT("New career is distinguished from legacy T1 demo grants"),Economy.InitializedMarkets.Contains(TEXT("Career starting grant")));
    Players->GetEconomy();Players->GetEconomy();
    TestEqual(TEXT("Opening wallet does not repeat the grant"),Economy.Journal.Num(),1);
    TestTrue(TEXT("Spending uses the ordinary ledger"),FEconomyLedger::Pay(Economy,FEconomyLedger::Pilot(),TEXT("Station"),500,FGuid::NewGuid(),TEXT("Test purchase")));
    auto* Save=NewObject<UPlayerSaveGame>();Save->PlayerInfo=Players->GetPlayerInfo();TArray<uint8> Bytes;
    if(!TestTrue(TEXT("Career wallet saves"),UGameplayStatics::SaveGameToMemory(Save,Bytes)))return false;
    const auto* Loaded=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if(!TestNotNull(TEXT("Career wallet reloads"),Loaded))return false;
    Players->GetMutablePlayerInfo()=Loaded->PlayerInfo;
    TestEqual(TEXT("Reload never tops a spent wallet back up"),TransactionAI::Balance(Players->GetEconomy()),49500LL);
    Players->GetMutablePlayerInfo()=FS_PlayerGameInfo();
    Players->GetMutablePlayerInfo().InsuranceCredits=1234;
    TestEqual(TEXT("Old career retains only its existing legacy balance"),TransactionAI::Balance(Players->GetEconomy()),1234LL);
    return true;
}
#endif
