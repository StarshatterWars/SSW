#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "CommerceCharges.h"
#include "GameStructs_economy.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommerceChargesTest,"Starshatter.Economy.InclusiveGstAndServiceFees",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCommerceChargesTest::RunTest(const FString&)
{
    for(const int64 Total:{int64(0),int64(1),int64(1000),int64(57400),MAX_int64})
    {
        const auto Quote=CommerceCharges::Quote(Total,ECommercePurpose::Service);
        TestTrue(TEXT("Inclusive quote is finite and valid"),Quote.bValid);
        TestEqual(TEXT("Base, fees and taxes sum to advertised total"),Quote.Base+Quote.Tax+Quote.Fee,Total);
        TestTrue(TEXT("No negative charges"),Quote.Base>=0 && Quote.Tax>=0 && Quote.Fee>=0);
    }
    TestFalse(TEXT("Negative price fails closed"),CommerceCharges::Quote(-1,ECommercePurpose::Purchase).bValid);
    const auto Hull=CommerceCharges::Quote(57400,ECommercePurpose::Purchase);
    TestEqual(TEXT("Stormhawk final advertised price remains unchanged"),Hull.Total,int64(57400));
    TestTrue(TEXT("Hull price includes GST"),Hull.Tax>0 && Hull.Base<Hull.Total);
    FEconomyState State;State.Wallets.Add(TEXT("Buyer"),100000);State.Wallets.Add(TEXT("Vendor"),100);
    const auto Receipt=FGuid::NewGuid();
    TestTrue(TEXT("Inclusive payment commits atomically"),FEconomyLedger::Pay(State,TEXT("Buyer"),TEXT("Vendor"),Hull.Total,Receipt,TEXT("Hull purchase"),ECommercePurpose::Purchase));
    TestEqual(TEXT("Buyer pays exactly final total"),State.Wallets.FindRef(TEXT("Buyer")),int64(42600));
    TestEqual(TEXT("Vendor receives base value"),State.Wallets.FindRef(TEXT("Vendor")),100+Hull.Base);
    TestEqual(TEXT("GST reaches a real authority account"),State.Wallets.FindRef(CommerceCharges::TaxAccount()),Hull.Tax);
    TestEqual(TEXT("Wallet preserves included GST"),State.Journal.Last().TaxCredits,Hull.Tax);
    TestEqual(TEXT("Wallet delta reflects actual vendor proceeds"),State.Journal.Last().WalletDeltas.FindRef(TEXT("Vendor")),Hull.Base);
    int64 Money=0;for(const auto& Account:State.Wallets)Money+=Account.Value;
    TestEqual(TEXT("Fees conserve credits rather than minting or deleting them"),Money,int64(100100));
    TestFalse(TEXT("Receipt replay cannot charge GST twice"),FEconomyLedger::Pay(State,TEXT("Buyer"),TEXT("Vendor"),Hull.Total,Receipt,TEXT("Hull purchase"),ECommercePurpose::Purchase));
    const auto Before=State.Wallets.FindRef(TEXT("Buyer"));
    State.Wallets.Add(CommerceCharges::TaxAccount(),MAX_int64);
    TestFalse(TEXT("Full treasury fails without partial debit"),FEconomyLedger::Pay(State,TEXT("Buyer"),TEXT("Vendor"),1000,FGuid::NewGuid(),TEXT("Purchase"),ECommercePurpose::Purchase));
    TestEqual(TEXT("Failed tax collection preserves buyer wallet"),State.Wallets.FindRef(TEXT("Buyer")),Before);
    const auto Exempt=CommerceCharges::Quote(5000,ECommercePurpose::None);
    TestEqual(TEXT("Opening balances, rewards, escrow and refunds are not new sales"),Exempt.Base,int64(5000));
    TestEqual(TEXT("Broker fee is separate from refundable escrow"),CommerceCharges::BrokerFee(10000),int64(100));
    return true;
}
#endif
