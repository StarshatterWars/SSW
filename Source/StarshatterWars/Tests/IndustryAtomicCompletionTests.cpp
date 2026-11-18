#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GameStructs_economy.h"

namespace
{
FCommodityAmount Amount(FName Item,double Quantity,double Mass)
{
    FCommodityAmount Result;Result.Item=Item;Result.Quantity=Quantity;Result.UnitTonnes=Mass;return Result;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndustryAtomicCompletionTest,"Starshatter.Economy.Industry.AtomicMultiProductCompletionAndRetry",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FIndustryAtomicCompletionTest::RunTest(const FString&)
{
    FEconomyState State;
    const FName Owner(TEXT("Industry")),Input(TEXT("Input")),Output(TEXT("Output")),Recipe(TEXT("CompletionFixture"));
    FEconomyLedger::CreateInventory(State,Input,Owner,TEXT("Janus"),100);
    FEconomyLedger::CreateInventory(State,Output,Owner,TEXT("Janus"),100);
    const auto Ore=Amount(TEXT("Ore"),4,1),First=Amount(TEXT("FirstProduct"),2,1),Last=Amount(TEXT("LastProduct"),1,1.e-15);
    TestTrue(TEXT("Input stock supplied"),FEconomyLedger::Deposit(State,Input,Ore.Item,10,1));
    TestTrue(TEXT("First existing product supplied"),FEconomyLedger::Deposit(State,Output,First.Item,10,1));
    TestTrue(TEXT("Second stack is valid at numeric quantity limit"),FEconomyLedger::Deposit(State,Output,Last.Item,1.e15,Last.UnitTonnes));
    // Another job's capacity reservation must survive both failure and success.
    State.Inventories.FindChecked(Output).ReservedOutputTonnes=3;
    const FGuid License=FEconomyLedger::GrantBlueprint(State,Owner,Recipe,false,1);
    const FGuid Id=FEconomyLedger::QueueBlueprintJob(State,Owner,TEXT("Works"),Input,Output,License,1,{Ore},{First,Last},1);
    if(!TestTrue(TEXT("Job accepted by normal blueprint queue"),Id.IsValid()))return false;
    TestEqual(TEXT("Job starts without finishing"),FEconomyLedger::AdvanceJobs(State,.25,[](FName){return true;}),0);
    const double Reserved=State.Inventories.FindChecked(Output).ReservedOutputTonnes;
    const FGuid StackId=State.Inventories.FindChecked(Output).Items[0].Id;
    TestEqual(TEXT("Input consumed once at start"),FEconomyLedger::Quantity(State,Input,Ore.Item),6.0);
    TestEqual(TEXT("Copy run consumed once at start"),State.Blueprints.FindChecked(License).RunsRemaining,0);
    TestEqual(TEXT("Quantity overflow does not report completion"),FEconomyLedger::AdvanceJobs(State,1,[](FName){return true;}),0);
    TestEqual(TEXT("No partial first product survives failed second deposit"),FEconomyLedger::Quantity(State,Output,First.Item),10.0);
    TestEqual(TEXT("Failed second product remains unchanged"),FEconomyLedger::Quantity(State,Output,Last.Item),1.e15);
    TestEqual(TEXT("All reservations retained after failure"),State.Inventories.FindChecked(Output).ReservedOutputTonnes,Reserved);
    TestEqual(TEXT("Original stack identity retained after failure"),State.Inventories.FindChecked(Output).Items[0].Id,StackId);
    TestEqual(TEXT("Pending completion stays Running, never re-enters input consumption"),State.Jobs[0].State,EIndustryJobState::Running);
    TestEqual(TEXT("Completed processing time is retained"),State.Jobs[0].Remaining,0.0);
    TestEqual(TEXT("Unchanged failure retries safely"),FEconomyLedger::AdvanceJobs(State,1,[](FName){return true;}),0);
    TestEqual(TEXT("Retry did not consume extra ore"),FEconomyLedger::Quantity(State,Input,Ore.Item),6.0);
    TestEqual(TEXT("Retry did not consume extra licensed runs"),State.Blueprints.FindChecked(License).RunsRemaining,0);
    TestTrue(TEXT("Real stock withdrawal resolves blocked addition"),FEconomyLedger::Withdraw(State,Owner,Output,Last.Item,1));
    TestEqual(TEXT("All products commit together after correction"),FEconomyLedger::AdvanceJobs(State,1,[](FName){return true;}),1);
    TestEqual(TEXT("First product delivered exactly once"),FEconomyLedger::Quantity(State,Output,First.Item),12.0);
    TestEqual(TEXT("Second product delivered exactly once"),FEconomyLedger::Quantity(State,Output,Last.Item),1.e15);
    TestTrue(TEXT("Only this job's reservation released"),FMath::IsNearlyEqual(State.Inventories.FindChecked(Output).ReservedOutputTonnes,3.0,1.e-12));
    TestEqual(TEXT("Completed marker commits with products"),State.Jobs[0].State,EIndustryJobState::Completed);
    TestEqual(TEXT("Completed job cannot deliver twice"),FEconomyLedger::AdvanceJobs(State,100,[](FName){return true;}),0);
    TestEqual(TEXT("Repeated completion does not duplicate first product"),FEconomyLedger::Quantity(State,Output,First.Item),12.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndustryDestinationCompletionTest,"Starshatter.Economy.Industry.CompletionRevalidatesDestination",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FIndustryDestinationCompletionTest::RunTest(const FString&)
{
    FEconomyState State;const FName Owner(TEXT("Industry")),Input(TEXT("Input")),Output(TEXT("Output"));
    FEconomyLedger::CreateInventory(State,Input,Owner,TEXT("Janus"),10);
    FEconomyLedger::CreateInventory(State,Output,Owner,TEXT("Janus"),10);
    const auto Ore=Amount(TEXT("Ore"),1,1),Product=Amount(TEXT("Product"),1,1);
    FEconomyLedger::Deposit(State,Input,Ore.Item,1,1);
    FEconomyLedger::QueueJob(State,Owner,TEXT("Works"),Input,Output,TEXT("Recipe"),{Ore},{Product},1);
    FEconomyLedger::AdvanceJobs(State,.25,[](FName){return true;});
    const auto ReservedOutput=State.Inventories.FindChecked(Output);
    State.Inventories.Remove(Output);
    TestEqual(TEXT("Missing destination does not crash or mark completed"),FEconomyLedger::AdvanceJobs(State,1,[](FName){return true;}),0);
    State.Inventories.Add(Output,ReservedOutput);
    State.Inventories.FindChecked(Output).Owner=TEXT("Other owner");
    TestEqual(TEXT("Changed ownership cannot receive the completed job"),FEconomyLedger::AdvanceJobs(State,1,[](FName){return true;}),0);
    TestEqual(TEXT("Foreign destination retains its reservation"),State.Inventories.FindChecked(Output).ReservedOutputTonnes,1.0);
    State.Inventories.FindChecked(Output).Owner=Owner;
    State.Inventories.FindChecked(Output).CapacityTonnes=0;
    TestEqual(TEXT("Reduced storage cannot silently discard output"),FEconomyLedger::AdvanceJobs(State,1,[](FName){return true;}),0);
    TestEqual(TEXT("Storage rejection retains reservation"),State.Inventories.FindChecked(Output).ReservedOutputTonnes,1.0);
    TestEqual(TEXT("Storage rejection deposits nothing"),FEconomyLedger::Quantity(State,Output,Product.Item),0.0);
    State.Inventories.FindChecked(Output).CapacityTonnes=10;
    TestEqual(TEXT("Restored destination finishes pending job"),FEconomyLedger::AdvanceJobs(State,1,[](FName){return true;}),1);
    TestEqual(TEXT("Restored destination receives real product"),FEconomyLedger::Quantity(State,Output,Product.Item),1.0);
    TestEqual(TEXT("Already consumed input is not required again"),FEconomyLedger::Quantity(State,Input,Ore.Item),0.0);
    return true;
}
#endif
