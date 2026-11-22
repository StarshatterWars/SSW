#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AwardCreditAI.h"
#include "TransactionAI.h"
#include "Kismet/GameplayStatics.h"
#include "LocalGameStateSave.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAwardCreditTest,"Starshatter.Economy.AwardAndPromotionCredits",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FAwardCreditTest::RunTest(const FString&)
{
 auto* Save=NewObject<ULocalEconomyState>();auto& E=Save->Economy;E.Wallets.Add(FEconomyLedger::Pilot(),50000);
 const auto& P=AwardCreditAI::Get();TestEqual(TEXT("Cadet creation gives no promotion windfall"),P.Credits(true,0),0LL);
 TestTrue(TEXT("Earned Ensign promotion pays configured bonus"),P.Grant(E,true,1));TestEqual(TEXT("Promotion balance"),TransactionAI::Balance(E),55000LL);
 TestFalse(TEXT("Promotion cannot pay twice"),P.Grant(E,true,1));TestTrue(TEXT("New medal pays configured award"),P.Grant(E,false,64));TestEqual(TEXT("Medal balance"),TransactionAI::Balance(E),56000LL);
 TArray<uint8> Bytes;TestTrue(TEXT("Wallet and receipts save together"),UGameplayStatics::SaveGameToMemory(Save,Bytes));auto* Reload=Cast<ULocalEconomyState>(UGameplayStatics::LoadGameFromMemory(Bytes));if(!TestNotNull(TEXT("Wallet restored"),Reload))return false;
 TestFalse(TEXT("Reload cannot repeat promotion"),P.Grant(Reload->Economy,true,1));TestFalse(TEXT("Mission replay cannot repeat medal"),P.Grant(Reload->Economy,false,64));TestEqual(TEXT("No duplicated credits after restart"),TransactionAI::Balance(Reload->Economy),56000LL);
 return true;
}
#endif
