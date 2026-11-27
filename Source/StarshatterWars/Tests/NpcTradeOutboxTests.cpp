#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "NpcTradeAI.h"
#include "PlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNpcTradeOutboxTest,"Starshatter.Economy.Trades.PlayerEscrowRestartAndRetries",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FNpcTradeOutboxTest::RunTest(const FString&)
{
    FEconomyState State;State.Wallets.Add(TEXT("Pilot"),5000);State.bLegacyWalletMigrated=true;
    FEconomyLedger::CreateInventory(State,TEXT("Pilot/Cargo"),TEXT("Pilot"),TEXT("Janus"),50);
    FEconomyLedger::Deposit(State,TEXT("Pilot/Cargo"),TEXT("Iron"),10,1);
    FNpcTradeReceipt Trade;Trade.Id=FGuid::NewGuid();Trade.Principal=TEXT("Pilot/Fixture");Trade.WorldId=TEXT("FixtureWorld");Trade.Inventory=TEXT("Pilot/Cargo");Trade.Market=TEXT("Janus/NPC Market/Stock");Trade.Item=TEXT("Iron");Trade.Quantity=2;Trade.UnitTonnes=1;Trade.TotalCredits=102;Trade.BaseCredits=100;Trade.TaxCredits=2;
    FText Reason;int32 Saves=0;const auto Persist=[&](){++Saves;return true;};const auto Original=State;
    TestFalse(TEXT("Failed escrow save rejects purchase"),NpcTradeAI::Stage(State,Trade,[]{return false;},Reason));
    TestTrue(TEXT("Failed stage restores wallet, holds, receipts and history"),FEconomyState::StaticStruct()->CompareScriptStruct(&State,&Original,0));
    if(!TestTrue(TEXT("Prepared purchase escrows and persists"),NpcTradeAI::Stage(State,Trade,Persist,Reason)))return false;
    TestEqual(TEXT("Pending funds are not spendable"),State.Wallets.FindRef(TEXT("Pilot")),4898LL);
    TestEqual(TEXT("Goods are not granted before server settlement"),FEconomyLedger::Quantity(State,TEXT("Pilot/Cargo"),TEXT("Iron")),10.0);
    TestEqual(TEXT("Receiving space remains reserved"),State.Inventories.FindChecked(TEXT("Pilot/Cargo")).ReservedOutputTonnes,2.0);
    auto OtherWorld=Trade;OtherWorld.WorldId=TEXT("AnotherWorld");TestFalse(TEXT("Receipt cannot replay against another economy world"),NpcTradeAI::Stage(State,OtherWorld,Persist,Reason));
    const int32 FirstSaves=Saves;TestTrue(TEXT("Stage retry is idempotent"),NpcTradeAI::Stage(State,Trade,Persist,Reason));TestEqual(TEXT("Retry does not perform another save/debit"),Saves,FirstSaves);
    auto* Save=NewObject<UPlayerSaveGame>();Save->PlayerInfo.Economy=State;TArray<uint8> Bytes;
    TestTrue(TEXT("Player outbox serializes with assets"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
    auto* Reloaded=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));if(!TestNotNull(TEXT("Player assets and outbox reload"),Reloaded))return false;State=Reloaded->PlayerInfo.Economy;
    const auto Escrowed=State;
    TestFalse(TEXT("Failed delivery save leaves escrow retryable"),NpcTradeAI::Resolve(State,Trade.Id,true,[]{return false;},Reason));
    TestTrue(TEXT("Failed delivery rolls back all candidate changes"),FEconomyState::StaticStruct()->CompareScriptStruct(&State,&Escrowed,0));
    if(!TestTrue(TEXT("Committed native receipt delivers once"),NpcTradeAI::Resolve(State,Trade.Id,true,Persist,Reason)))return false;
    TestEqual(TEXT("Purchase delivers exactly the quantity"),FEconomyLedger::Quantity(State,TEXT("Pilot/Cargo"),TEXT("Iron")),12.0);
    TestEqual(TEXT("Settlement cannot debit player twice"),State.Wallets.FindRef(TEXT("Pilot")),4898LL);
    const auto Applied=State;TestTrue(TEXT("Repeated completion succeeds without exchanging again"),NpcTradeAI::Resolve(State,Trade.Id,true,Persist,Reason));TestTrue(TEXT("Applied receipt prevents every duplicate side effect"),FEconomyState::StaticStruct()->CompareScriptStruct(&State,&Applied,0));
    TestFalse(TEXT("Committed purchase cannot become a refund"),NpcTradeAI::Resolve(State,Trade.Id,false,Persist,Reason));
    auto Sale=Trade;Sale.Id=FGuid::NewGuid();Sale.bBuying=false;
    if(!TestTrue(TEXT("Sale moves cargo into durable player escrow"),NpcTradeAI::Stage(State,Sale,Persist,Reason)))return false;
    TestEqual(TEXT("Escrowed cargo cannot be sold or used again"),FEconomyLedger::Available(State,TEXT("Pilot/Cargo"),TEXT("Iron")),10.0);
    TestEqual(TEXT("Sale pays nothing before acknowledgement"),State.Wallets.FindRef(TEXT("Pilot")),4898LL);
    TestTrue(TEXT("Cancelled native sale returns player cargo"),NpcTradeAI::Resolve(State,Sale.Id,false,Persist,Reason));TestEqual(TEXT("Refund restores exact cargo quantity"),FEconomyLedger::Quantity(State,TEXT("Pilot/Cargo"),TEXT("Iron")),12.0);
    Sale.Id=FGuid::NewGuid();TestTrue(TEXT("Another sale can be prepared"),NpcTradeAI::Stage(State,Sale,Persist,Reason));TestTrue(TEXT("Committed sale credits proceeds"),NpcTradeAI::Resolve(State,Sale.Id,true,Persist,Reason));TestEqual(TEXT("Included tax is not paid to seller"),State.Wallets.FindRef(TEXT("Pilot")),4998LL);
    const auto Sold=State;TestTrue(TEXT("Sale settlement retries remain safe"),NpcTradeAI::Resolve(State,Sale.Id,true,Persist,Reason));TestTrue(TEXT("Sale completion cannot repeat credit or cargo removal"),FEconomyState::StaticStruct()->CompareScriptStruct(&State,&Sold,0));
    auto CancelledBuy=Trade;CancelledBuy.Id=FGuid::NewGuid();TestTrue(TEXT("Purchase reservation can be staged"),NpcTradeAI::Stage(State,CancelledBuy,Persist,Reason));TestTrue(TEXT("Expired/cancelled purchase refunds escrow"),NpcTradeAI::Resolve(State,CancelledBuy.Id,false,Persist,Reason));TestEqual(TEXT("Cancelled purchase returns full payment"),State.Wallets.FindRef(TEXT("Pilot")),4998LL);
    TestFalse(TEXT("Terminal receipts leave no pending player exchange"),NpcTradeAI::HasPending(State));return true;
}
#endif
