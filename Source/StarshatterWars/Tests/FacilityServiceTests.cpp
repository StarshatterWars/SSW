#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "BlueprintCatalogSubsystem.h"
#include "EconomyRuntime.h"
#include <limits>
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFacilityServiceTermsTest,"Starshatter.Economy.FacilityServiceTerms",
 EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FFacilityServiceTermsTest::RunTest(const FString&)
{
 FEconomyState State;const auto Owner=FEconomyLedger::Pilot();State.Wallets.Add(Owner,1000);
 FSSWProductionFacility Site;Site.System=TEXT("Janus");Site.Facility=TEXT("Test Factory");Site.Recipe=TEXT("Parts");Site.CapacityTonnes=100;
 Site.ServiceProfile=TEXT("Factory");Site.PlayerMaterialMultiplier=.98;Site.PlayerTimeMultiplier=.95;Site.InstallationFeeCredits=100;Site.PerRunFeeCredits=5;Site.MaximumPlayerRuns=10;
 const auto Input=FEconomyRuntime::Warehouse(Site.System,Site.Facility,TEXT("Player Input"));
 const auto Output=FEconomyRuntime::Warehouse(Site.System,Site.Facility,TEXT("Player Output"));
 FEconomyLedger::CreateInventory(State,Input,Owner,TEXT("Janus"),100);FEconomyLedger::CreateInventory(State,Output,Owner,TEXT("Janus"),100);
 FSSWProductionRecipe Recipe;Recipe.InputTonnes=10;Recipe.OutputTonnes=2;Recipe.SecondsPerBatch=10;Recipe.OutputQuantity=2;Recipe.CargoItem=TEXT("Parts");
 FCommodityAmount Ore;Ore.Item=TEXT("Magnetite");Ore.Quantity=10;Recipe.Inputs.Add(Ore);
 FEconomyLedger::Deposit(State,Input,Ore.Item,50,1);
 const auto Blueprint=FEconomyLedger::GrantBlueprint(State,Owner,Site.Recipe,true,0,.1,.2);
 const auto Receipt=FGuid::NewGuid();State.BlueprintReceipts.Add(Blueprint,Receipt);State.CommittedTransactions.Add(Receipt);
 FPlayerIndustryQuote Quote;FText Reason;
 TestTrue(TEXT("Funded factory quote"),UBlueprintCatalogSubsystem::QuotePlayerJob(State,Site,Recipe,Blueprint,2,Quote,Reason));
 TestEqual(TEXT("Installation fee is explicit"),Quote.InstallationFee,int64(110));
 TestTrue(TEXT("Facility and blueprint time apply once"),FMath::IsNearlyEqual(Quote.Duration,15.2));
 if(!Quote.Materials.IsEmpty())TestTrue(TEXT("Material efficiencies combine once"),FMath::IsNearlyEqual(Quote.Materials[0].Quantity,17.64));
 TestEqual(TEXT("Quote never charges funds"),State.Wallets.FindRef(Owner),int64(1000));
 TestEqual(TEXT("Quote never installs a job"),State.Jobs.Num(),0);
 Site.bPlayerManufacturing=false;TestFalse(TEXT("Unavailable service denied"),UBlueprintCatalogSubsystem::QuotePlayerJob(State,Site,Recipe,Blueprint,2,Quote,Reason));Site.bPlayerManufacturing=true;
 Site.MaximumPlayerRuns=1;TestFalse(TEXT("Facility batch limit enforced"),UBlueprintCatalogSubsystem::QuotePlayerJob(State,Site,Recipe,Blueprint,2,Quote,Reason));Site.MaximumPlayerRuns=10;
 State.Wallets[Owner]=109;TestFalse(TEXT("Insufficient service payment denied"),UBlueprintCatalogSubsystem::QuotePlayerJob(State,Site,Recipe,Blueprint,2,Quote,Reason));State.Wallets[Owner]=1000;
 Site.PlayerMaterialMultiplier=std::numeric_limits<double>::quiet_NaN();TestFalse(TEXT("Malformed facility term denied"),UBlueprintCatalogSubsystem::QuotePlayerJob(State,Site,Recipe,Blueprint,2,Quote,Reason));
 return true;
}
#endif
