#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EconomyOwnership.h"
#include "GameStructs_economy.h"
#include "EconomyRuntime.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyOwnershipBoundaryTest,"Starshatter.Economy.Ownership.PlayerAssetsStayWithPlayer",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FEconomyOwnershipBoundaryTest::RunTest(const FString&)
{
    FEconomyState Local,Remote;Local.PlayerAccounts.Add(TEXT("Pilot"));Local.Wallets.Add(TEXT("Pilot"),425000);Local.bLegacyWalletMigrated=true;
    auto* Hold=FEconomyLedger::CreateInventory(Local,TEXT("Pilot/Cargo"),TEXT("Pilot"),TEXT("Janus"),50);
    FEconomyItem Item;Item.Id=FGuid(1,2,3,4);Item.Type=TEXT("Iron");Item.Quantity=7.5;Item.UnitTonnes=1;Hold->Items.Add(Item);const auto Original=*Hold;
    FEconomyLedger::CreateInventory(Local,TEXT("Carrier/UnknownDomain"),TEXT("CV6-Orion"),TEXT("Janus"),100);
    FEconomyLedger::CreateInventory(Local,TEXT("Old/NPC/Stock"),FEconomyRuntime::IndustryOwner(),TEXT("Janus"),100);
    FIndustryJob Job;Job.Id=FGuid(5,6,7,8);Job.Owner=TEXT("Pilot");Job.Remaining=123;Local.Jobs.Add(Job);
    FBlueprintInstance BP;BP.Id=FGuid(6,7,8,9);BP.Owner=TEXT("Pilot");Local.Blueprints.Add(BP.Id,BP);
    const FGuid Receipt(7,8,9,10);Local.CommittedTransactions.Add(Receipt);
    FEconomyLedger::CreateInventory(Remote,TEXT("Janus/Works/Output"),FEconomyRuntime::IndustryOwner(),TEXT("Janus"),100);
    Remote.Inventories.FindChecked(TEXT("Janus/Works/Output")).TradingOwner=TEXT("NPC/Works");Remote.Wallets.Add(TEXT("NPC/Works"),20000);
    Remote.Wallets.Add(TEXT("Pilot"),5000);Remote.BackgroundOreRemaining.Add(TEXT("Deposit"),77);
    auto PlayerMutation=Local;PlayerMutation.Wallets[TEXT("Pilot")]-=100;
    TestTrue(TEXT("Player-only mutation does not write the NPC projection"),SSWEconomyOwnership::MatchesNpc(Local,PlayerMutation));
    auto NpcMutation=Local;NpcMutation.Inventories.FindChecked(TEXT("Old/NPC/Stock")).ReservedOutputTonnes=1;
    TestFalse(TEXT("NPC stock reservation changes need the remote adapter"),SSWEconomyOwnership::MatchesNpc(Local,NpcMutation));
    const auto OnlyNpc=SSWEconomyOwnership::ExtractNpc(Local);
    TestFalse(TEXT("NPC export contains no pilot hold"),OnlyNpc.Inventories.Contains(TEXT("Pilot/Cargo")));
    TestFalse(TEXT("NPC export contains no pilot wallet"),OnlyNpc.Wallets.Contains(TEXT("Pilot")));
    TestTrue(TEXT("NPC export contains no pilot jobs or blueprints"),OnlyNpc.Jobs.IsEmpty()&&OnlyNpc.Blueprints.IsEmpty());
    SSWEconomyOwnership::ApplyNpcProjection(Local,Remote);
    TestEqual(TEXT("Remote view cannot reset player balance"),Local.Wallets.FindRef(TEXT("Pilot")),425000LL);
    TestTrue(TEXT("Item IDs, quantities and ownership remain exact"),FEconomyInventory::StaticStruct()->CompareScriptStruct(&Original,&Local.Inventories.FindChecked(TEXT("Pilot/Cargo")),0));
    TestTrue(TEXT("Unclassified carrier assets stay in their existing domain"),Local.Inventories.Contains(TEXT("Carrier/UnknownDomain")));
    TestTrue(TEXT("Pilot job and license survive"),Local.Jobs.ContainsByPredicate([&](const auto& J){return J.Id==Job.Id&&J.Remaining==123;})&&Local.Blueprints.Contains(BP.Id));
    TestTrue(TEXT("Applied receipts remain with pilot"),Local.CommittedTransactions.Contains(Receipt));
    TestFalse(TEXT("Old NPC projection is replaced"),Local.Inventories.Contains(TEXT("Old/NPC/Stock")));
    TestTrue(TEXT("NPC stock and budgets become visible"),Local.Inventories.Contains(TEXT("Janus/Works/Output"))&&Local.Wallets.FindRef(TEXT("NPC/Works"))==20000);
    TestEqual(TEXT("Shared depletion projects without replenishing"),Local.BackgroundOreRemaining.FindRef(TEXT("Deposit")),77.0);
    SSWEconomyOwnership::ApplyNpcProjection(Local,Remote);TestEqual(TEXT("Projection retry never changes player credits"),Local.Wallets.FindRef(TEXT("Pilot")),425000LL);
    return true;
}
#endif
