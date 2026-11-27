#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "ManufacturingAI.h"
#include "TrafficAI.h"
#include "ModifierProvenance.h"
#include "BlueprintCatalogSubsystem.h"
#include "JsonObjectConverter.h"
#include "EconomyAI.h"
#include "EconomyRuntime.h"
#include "ProductionCatalog.h"
#include "NativeEconomyWorker.h"
#include "SimDataEvents.h"
namespace
{
struct FExpansionFixture
{
 FEconomyState State;FName Owner=TEXT("Civilian industry"),Facility=TEXT("Test works"),Input=TEXT("Input"),Output=TEXT("Output");
 FExpansionFixture(){FEconomyLedger::CreateInventory(State,Input,Owner,TEXT("Janus"),1000);FEconomyLedger::CreateInventory(State,Output,Owner,TEXT("Janus"),1000);FEconomyLedger::Deposit(State,Input,TEXT("ore"),100,1);}
 FSSWProductionRecipe Recipe()const
 {FSSWProductionRecipe R;R.InputTonnes=2;R.OutputTonnes=1;R.SecondsPerBatch=10;FCommodityAmount M;M.Item=TEXT("ore");M.Quantity=2;R.Inputs={M};M.Item=TEXT("parts");M.Quantity=1;M.UnitTonnes=.5;R.Outputs={M};M.Item=TEXT("residue");R.Outputs.Add(M);return R;}
 FGuid Add(double Seconds=10,FName RecipeId=TEXT("recipe"))
 {TArray<FCommodityAmount> In,Out;ManufacturingAI::BuildBatch(State,Input,Recipe(),In,Out);return FEconomyLedger::QueueJob(State,Owner,Facility,Input,Output,RecipeId,In,Out,Seconds);}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMultiProductTest,"Starshatter.IndustryExpansion.MultipleOutputsAndAtomicSettlement",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FMultiProductTest::RunTest(const FString&)
{
 FExpansionFixture F;auto Recipe=F.Recipe();TArray<FCommodityAmount> M,P;TestTrue(TEXT("Arrays build one complete bill"),ManufacturingAI::BuildBatch(F.State,F.Input,Recipe,M,P));TestEqual(TEXT("Both output products"),P.Num(),2);
 F.Add();FEconomyLedger::AdvanceJobs(F.State,5,[](FName){return true;});TestEqual(TEXT("All products reserve combined mass"),F.State.Inventories.FindChecked(F.Output).ReservedOutputTonnes,1.);
 TestEqual(TEXT("No partial early output"),FEconomyLedger::Quantity(F.State,F.Output,TEXT("parts")),0.);
 F.State.Inventories.FindChecked(F.Output).AcceptedItems={TEXT("parts")};FEconomyLedger::AdvanceJobs(F.State,5,[](FName){return true;});TestEqual(TEXT("Rejected second product leaves first uncredited"),FEconomyLedger::Quantity(F.State,F.Output,TEXT("parts")),0.);TestEqual(TEXT("Blocked output retains lane"),F.State.Jobs[0].State,EIndustryJobState::Running);
 F.State.Inventories.FindChecked(F.Output).AcceptedItems.Reset();FEconomyLedger::AdvanceJobs(F.State,.1,[](FName){return true;});TestEqual(TEXT("First coproduct settled"),FEconomyLedger::Quantity(F.State,F.Output,TEXT("parts")),1.);TestEqual(TEXT("Second coproduct settled"),FEconomyLedger::Quantity(F.State,F.Output,TEXT("residue")),1.);TestEqual(TEXT("Reservations released once"),F.State.Inventories.FindChecked(F.Output).ReservedOutputTonnes,0.);
 Recipe.Outputs[1].Item=Recipe.Outputs[0].Item;TestFalse(TEXT("Duplicate product rejected"),Recipe.ResolveOutputs(P));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLaneQueueTest,"Starshatter.IndustryExpansion.LanesQueueAndActivityEligibility",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FLaneQueueTest::RunTest(const FString&)
{
 FExpansionFixture F;FIndustryFacilityPolicy Policy;Policy.Lanes=2;F.State.IndustryPolicies.Add(F.Facility,Policy);F.Add(10);F.Add(20);F.Add(10);
 FEconomyLedger::AdvanceJobs(F.State,5,[](FName){return true;});TestEqual(TEXT("Two concurrent lanes consume only their inputs"),FEconomyLedger::Quantity(F.State,F.Input,TEXT("ore")),96.);TestEqual(TEXT("First lane remaining"),F.State.Jobs[0].Remaining,5.);TestEqual(TEXT("Second lane advances concurrently"),F.State.Jobs[1].Remaining,15.);TestEqual(TEXT("Queued job input untouched"),F.State.Jobs[2].Remaining,10.);
 FEconomyLedger::AdvanceJobs(F.State,5,[](FName){return true;});TestEqual(TEXT("Released lane starts queued work"),F.State.Jobs[2].State,EIndustryJobState::Running);TestEqual(TEXT("Other lane not accelerated by the first"),F.State.Jobs[1].Remaining,10.);TestEqual(TEXT("Queued work gets only unused lane time"),F.State.Jobs[2].Remaining,10.);
 F.State.IndustryPolicies[F.Facility].Activities={TEXT("Refining")};FText Why;TestFalse(TEXT("Wrong activity fails lane eligibility"),FEconomyLedger::CanStartJob(F.State,F.State.Jobs[2],true,Why));
 FExpansionFixture Default;Default.Add();Default.Add();FEconomyLedger::AdvanceJobs(Default.State,5,[](FName){return true;});TestEqual(TEXT("Old data retains one running job"),Default.State.Jobs[1].Remaining,10.);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FScheduledLaneTest,"Starshatter.IndustryExpansion.ScheduledIndependentLaneDeadlines",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FScheduledLaneTest::RunTest(const FString&)
{
 FExpansionFixture F;FSSWProductionFacility Site;Site.Facility=F.Facility.ToString();Site.System=TEXT("Janus");Site.Region=TEXT("Janek");Site.CapacityTonnes=1000;Site.Recipe=TEXT("recipe");Site.ProductionLanes=2;
 TMap<FName,FSSWProductionFacility> Sites;Sites.Add(TEXT("site"),Site);TMap<FName,FSSWProductionRecipe> Recipes;auto R=F.Recipe();Recipes.Add(Site.Recipe,R);
 const auto In=FEconomyRuntime::Warehouse(Site.System,Site.Facility,TEXT("Input")),Out=FEconomyRuntime::Warehouse(Site.System,Site.Facility,TEXT("Output"));
 FEconomyLedger::CreateInventory(F.State,In,F.Owner,TEXT("Janus"),1000);FEconomyLedger::CreateInventory(F.State,Out,F.Owner,TEXT("Janus"),1000);FEconomyLedger::Deposit(F.State,In,TEXT("ore"),4,1);
 TArray<FCommodityAmount> M,P;ManufacturingAI::BuildBatch(F.State,In,R,M,P);FEconomyLedger::QueueJob(F.State,F.Owner,F.Facility,In,Out,Site.Recipe,M,P,5);FEconomyLedger::QueueJob(F.State,F.Owner,F.Facility,In,Out,Site.Recipe,M,P,10);
 EconomyAI C;F.State.EconomyClockSeconds=100;C.AdvanceProduction(F.State,Sites,Recipes);TestTrue(TEXT("First deadline scheduled"),FMath::IsNearlyEqual(C.ProductionDeadline(),105.,.001));
 F.State.EconomyClockSeconds=105;C.AdvanceProduction(F.State,Sites,Recipes);TestEqual(TEXT("Only first lane completes"),FEconomyLedger::Quantity(F.State,Out,TEXT("parts")),1.);TestTrue(TEXT("Second deadline kept"),FMath::IsNearlyEqual(C.ProductionDeadline(),110.,.001));
 F.State.EconomyClockSeconds=110;C.AdvanceProduction(F.State,Sites,Recipes);TestEqual(TEXT("Second lane completes separately"),FEconomyLedger::Quantity(F.State,Out,TEXT("parts")),2.);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProductionCatalogExpansionTest,"Starshatter.IndustryExpansion.AuthoredCatalogAndLegacyProducts",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FProductionCatalogExpansionTest::RunTest(const FString&)
{
 FString Why;TestTrue(TEXT("Updated catalogs ingest directly"),SSWProductionCatalog::Reload(Why));if(!Why.IsEmpty())AddError(Why);
 for(const auto& Pair:SSWProductionCatalog::Recipes()){TArray<FCommodityAmount> Out;TestTrue(*Pair.Key.ToString(),Pair.Value.ResolveOutputs(Out));}
 FSSWProductionRecipe Legacy;Legacy.CargoItem=TEXT("parts");Legacy.OutputTonnes=2;Legacy.OutputQuantity=4;Legacy.OutputUnit=TEXT("units");TArray<FCommodityAmount> Out;TestTrue(TEXT("Old single output still resolves"),Legacy.ResolveOutputs(Out));TestEqual(TEXT("Legacy quantity preserved"),Out[0].Quantity,4.);TestEqual(TEXT("Legacy unit mass preserved"),Out[0].UnitTonnes,.5);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTargetedChangeProfileTest,"Starshatter.IndustryExpansion.TargetedRefreshProfileAndBatching",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FTargetedChangeProfileTest::RunTest(const FString&)
{
 FExpansionFixture F;FIndustryJob Owned;Owned.Id=FGuid::NewGuid();Owned.Owner=FEconomyLedger::Pilot();Owned.Input=TEXT("Player input");Owned.Output=TEXT("Player output");Owned.Facility=F.Facility;F.State.Jobs.Add(Owned);
 int32 Global=0,Industry=0,Cargo=0;const auto LegacyHandle=FSimDataEvents::Changed().AddLambda([&](const FEconomyState& S){if(&S==&F.State)++Global;});
 const auto DetailHandle=FSimDataEvents::DetailedChanged().AddLambda([&](const FEconomyState& S,const FSimDataChange& C){if(&S==&F.State){Industry+=C.AffectsIndustry(S,FEconomyLedger::Pilot());Cargo+=C.Affects(ESimDataDomain::Inventory,TEXT("Player hold"));}});
 const uint64 Revision=FSimDataEvents::CurrentThreadRevision();const double Start=FPlatformTime::Seconds();
 for(int32 I=0;I<1000;++I)FEconomyLedger::Deposit(F.State,F.Input,TEXT("ore"),.001,1);
 const double Elapsed=(FPlatformTime::Seconds()-Start)*1000.;TestEqual(TEXT("Global safety notification retained"),Global,1000);TestEqual(TEXT("Unrelated deposits cause no job refresh requests"),Industry,0);TestEqual(TEXT("Unrelated deposits cause no cargo refresh requests"),Cargo,0);TestTrue(TEXT("Global mutation revision advances"),FSimDataEvents::CurrentThreadRevision()>Revision);
 {FScopedSimDataChangeBatch Batch(F.State);FSimDataEvents::Inventory(F.State,TEXT("Player hold"),TEXT("parts"));FSimDataEvents::Job(F.State,Owned.Id,F.Facility);}
 TestEqual(TEXT("One coalesced global event"),Global,1001);TestEqual(TEXT("Relevant industry notification"),Industry,1);TestEqual(TEXT("Relevant cargo notification"),Cargo,1);
 FSimDataEvents::Publish(F.State);TestEqual(TEXT("Unknown mutation conservatively refreshes jobs"),Industry,2);TestEqual(TEXT("Unknown mutation conservatively refreshes cargo"),Cargo,2);
 FSimDataEvents::Changed().Remove(LegacyHandle);FSimDataEvents::DetailedChanged().Remove(DetailHandle);
 AddInfo(FString::Printf(TEXT("Refresh profile: 1000 unrelated inventory mutations in %.3f ms; global invalidations=1000, filtered job/cargo invalidations=0. This measures callback eligibility, not rendered frame time."),Elapsed));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMultiProductFreightTest,"Starshatter.IndustryExpansion.MinedOreProductionAndDeliveredCoproducts",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FMultiProductFreightTest::RunTest(const FString&)
{
 FExpansionFixture F;F.State.Inventories[F.Input].Items.Reset();const auto Mine=FEconomyRuntime::Warehouse(TEXT("Janus"),TEXT("Belt"),TEXT("Ore"));
 TestTrue(TEXT("Finite native mining extracts source material"),EconomyAI::AdvanceMine(F.State,TEXT("deposit"),Mine,TEXT("Janus"),TEXT("ore"),100,.1,100,20));TestEqual(TEXT("Deposit reserve debited"),F.State.BackgroundOreRemaining.FindRef(TEXT("deposit")),98.);
 FGalaxyTrafficState Traffic;FGalaxyTrafficEndpoint From;From.Id=TEXT("mine");From.Sector=TEXT("Janus/Belt");From.System=TEXT("Janus");FGalaxyTrafficEndpoint To;To.Id=F.Facility;To.Sector=TEXT("Janus/Works");To.System=TEXT("Janus");
 const auto Id=TrafficAI::Schedule(Traffic,F.State,From,To,{From.Sector,To.Sector},Mine,F.Input,TEXT("ore"),2,1);if(!TestTrue(TEXT("Ore route reserves real mined cargo"),Id.IsValid()))return false;
 for(int32 I=0;I<20 && !Traffic.Journeys[0].IsFinished();++I)TrafficAI::Advance(Traffic.Journeys[0],F.State,10,false,true,1,10);
 TestEqual(TEXT("Ore unloads into permitted facility input"),FEconomyLedger::Quantity(F.State,F.Input,TEXT("ore")),2.);F.Add();FEconomyLedger::AdvanceJobs(F.State,10,[](FName){return true;});
 const FName Buyer=TEXT("Solus/NPC Market/Stock");FEconomyLedger::CreateInventory(F.State,Buyer,F.Owner,TEXT("Solus"),100);From=To;To.Id=TEXT("buyer");To.System=TEXT("Solus");To.Sector=TEXT("Solus/Market");
 for(FName Item:{FName(TEXT("parts")),FName(TEXT("residue"))})
 {
  const FGuid Freight=TrafficAI::Schedule(Traffic,F.State,From,To,{From.Sector,To.Sector},F.Output,Buyer,Item,1,.5);if(!TestTrue(TEXT("Each co-product has a freight route"),Freight.IsValid()))return false;
  auto& Journey=Traffic.Journeys.Last();TestEqual(TEXT("Reserving freight does not credit buyer"),FEconomyLedger::Quantity(F.State,Buyer,Item),0.);
  for(int32 I=0;I<20 && !Journey.IsFinished();++I)TrafficAI::Advance(Journey,F.State,10,false,true,1,10);
  TestEqual(TEXT("Only delivered product credited"),FEconomyLedger::Quantity(F.State,Buyer,Item),1.);TrafficAI::Advance(Journey,F.State,100,false,true,1,10);TestEqual(TEXT("Arrival cannot duplicate output"),FEconomyLedger::Quantity(F.State,Buyer,Item),1.);
 }
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlueprintServicesTest,"Starshatter.IndustryExpansion.BlueprintResearchCopyAndCancel",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FBlueprintServicesTest::RunTest(const FString&)
{
 FExpansionFixture F;FIndustryFacilityPolicy Policy;Policy.Activities.Append({TEXT("ResearchMaterial"),TEXT("ResearchTime"),TEXT("Copying")});F.State.IndustryPolicies.Add(F.Facility,Policy);
 const FGuid Original=FEconomyLedger::GrantBlueprint(F.State,F.Owner,TEXT("recipe"),true,0);TestTrue(TEXT("Purchased original queues for research"),FEconomyLedger::QueueBlueprintService(F.State,F.Owner,F.Facility,Original,TEXT("ResearchMaterial"),1,10).IsValid());
 TestFalse(TEXT("Busy original cannot copy concurrently"),FEconomyLedger::QueueBlueprintService(F.State,F.Owner,F.Facility,Original,TEXT("Copying"),5,10).IsValid());
 FEconomyLedger::AdvanceJobs(F.State,9,[](FName){return true;});TestEqual(TEXT("No early research modifier"),F.State.Blueprints[Original].MaterialEfficiency,0.);FEconomyLedger::AdvanceJobs(F.State,1,[](FName){return true;});TestEqual(TEXT("Research applies one percentage point"),F.State.Blueprints[Original].MaterialEfficiency,.01);
 const FGuid CopyJob=FEconomyLedger::QueueBlueprintService(F.State,F.Owner,F.Facility,Original,TEXT("Copying"),5,10);TestTrue(TEXT("Original available for copy"),CopyJob.IsValid());FEconomyLedger::AdvanceJobs(F.State,10,[](FName){return true;});
 const auto* Job=F.State.Jobs.FindByPredicate([CopyJob](const auto& J){return J.Id==CopyJob;});if(!Job)return false;const FGuid Copy=Job->ProducedBlueprint;TestTrue(TEXT("Copy identity recorded on job"),Copy.IsValid());const auto* BP=F.State.Blueprints.Find(Copy);if(!BP)return false;
 TestFalse(TEXT("Produced blueprint is a limited copy"),BP->bOriginal);TestEqual(TEXT("Copy has requested licensed runs"),BP->RunsRemaining,5);TestEqual(TEXT("Copy inherits material research"),BP->MaterialEfficiency,.01);TestEqual(TEXT("Copy retains source original"),BP->SourceBlueprint,Original);TestTrue(TEXT("Copy has a committed entitlement receipt"),F.State.CommittedTransactions.Contains(F.State.BlueprintReceipts.FindRef(Copy)));
 const int32 Count=F.State.Blueprints.Num();FEconomyLedger::AdvanceJobs(F.State,100,[](FName){return true;});TestEqual(TEXT("Completion does not create duplicate copies"),F.State.Blueprints.Num(),Count);
 TestFalse(TEXT("Copies cannot be researched"),FEconomyLedger::QueueBlueprintService(F.State,F.Owner,F.Facility,Copy,TEXT("ResearchTime"),1,10).IsValid());
 const FGuid Cancel=FEconomyLedger::QueueBlueprintService(F.State,F.Owner,F.Facility,Original,TEXT("ResearchTime"),1,10);FEconomyLedger::AdvanceJobs(F.State,1,[](FName){return true;});TestTrue(TEXT("Running research can cancel without commodity holds"),FEconomyLedger::CancelJob(F.State,F.Owner,Cancel));FEconomyLedger::AdvanceJobs(F.State,100,[](FName){return true;});TestEqual(TEXT("Cancelled research grants no modifier"),F.State.Blueprints[Original].TimeEfficiency,0.);
 FString Json;TestTrue(TEXT("Research result serializes"),FJsonObjectConverter::UStructToJsonObjectString(*BP,Json));FBlueprintInstance Reloaded;TestTrue(TEXT("Research result reloads"),FJsonObjectConverter::JsonObjectStringToUStruct(Json,&Reloaded,0,0));TestEqual(TEXT("Copy provenance survives reload"),Reloaded.SourceJob,CopyJob);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlueprintServiceWorkerTest,"Starshatter.IndustryExpansion.BlueprintServiceFeeRollbackAndPermissions",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FBlueprintServiceWorkerTest::RunTest(const FString&)
{
 FExpansionFixture F;const auto Pilot=FEconomyLedger::Pilot();F.State.Wallets.Add(Pilot,5000);const FGuid Original=FEconomyLedger::GrantBlueprint(F.State,Pilot,TEXT("recipe"),true,0);const FGuid Purchase=FGuid::NewGuid();F.State.BlueprintReceipts.Add(Original,Purchase);F.State.CommittedTransactions.Add(Purchase);
 FNativeLedgerCommand C;C.Action=ELedgerCommand::IndustryService;C.Object=Original;C.Receipt=FGuid::NewGuid();C.Activity=TEXT("ResearchTime");C.Facility=F.Facility;C.Duration=10;C.Credits=1000;
 auto Run=[&](){FNativeEconomyInput In;In.State=F.State;In.Profile=TEXT("Pilot");In.Revision=2;In.Ledger=MakeUnique<FNativeLedgerCommand>(C);return FNativeEconomyWorker::Calculate(MoveTemp(In));};
 auto R=Run();TestFalse(TEXT("Facility without activity rejects job"),R.Ledger->bSucceeded);C.Policy.Activities.Add(C.Activity);R=Run();if(!TestTrue(TEXT("Native service command accepted"),R.Ledger->bSucceeded))return false;
 FText Why;TestFalse(TEXT("Failed durable save rejects service fee and job"),FNativeEconomyWorker::CommitLedger(MoveTemp(R),F.State,2,TEXT("Pilot"),[]{return false;},Why));TestEqual(TEXT("Balance restored"),F.State.Wallets.FindRef(Pilot),5000LL);TestTrue(TEXT("Queue restored"),F.State.Jobs.IsEmpty());
 R=Run();TestTrue(TEXT("Service installation saved"),FNativeEconomyWorker::CommitLedger(MoveTemp(R),F.State,2,TEXT("Pilot"),[]{return true;},Why));TestEqual(TEXT("Exactly quoted fee charged"),F.State.Wallets.FindRef(Pilot),4000LL);R=Run();TestFalse(TEXT("Repeated receipt cannot charge twice"),R.Ledger->bSucceeded);
 FEconomyLedger::AdvanceJobs(F.State,10,[](FName){return true;});TestEqual(TEXT("Time research only after completion"),F.State.Blueprints[Original].TimeEfficiency,.02);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModifierSourcesTest,"Starshatter.IndustryExpansion.ModifierProvenanceUsesSSWValues",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FModifierSourcesTest::RunTest(const FString&)
{
 FRecordDetails D;ModifierProvenance::Price(D,1000,ECommercePurpose::Purchase,900);TestTrue(TEXT("Source-aware price facts present"),D.Fields.Num()>=4);for(const auto& Field:D.Fields)TestFalse(TEXT("Each modifier fact explains its SSW source"),Field.Source.IsEmpty());
 FExpansionFixture F;FSSWProductionFacility Site;Site.PlayerTimeMultiplier=.95;FBlueprintInstance BP;BP.MaterialEfficiency=.05;BP.TimeEfficiency=.1;FPlayerIndustryQuote Q;Q.Duration=17.1;Q.InstallationFee=123;const int32 Before=D.Fields.Num();ModifierProvenance::Industry(D,F.Recipe(),Site,&BP,&Q);TestTrue(TEXT("Production modifiers use same detail view"),D.Fields.Num()>Before);TestEqual(TEXT("View never modifies quoted duration"),Q.Duration,17.1);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOwnerChangeTest,"Starshatter.IndustryExpansion.OwnerScopedNotificationsAndSafety",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FOwnerChangeTest::RunTest(const FString&)
{
 FExpansionFixture F;int32 Pilot=0,NPC=0;const auto Handle=FSimDataEvents::DetailedChanged().AddLambda([&](const FEconomyState& S,const FSimDataChange& C){if(&S==&F.State && C.Affects(ESimDataDomain::Wallet)){Pilot+=C.AffectsOwner(FEconomyLedger::Pilot());NPC+=C.AffectsOwner(F.Owner);}});
 const auto Revision=FSimDataEvents::CurrentThreadRevision();FSimDataEvents::Wallet(F.State,F.Owner);TestEqual(TEXT("NPC wallet does not invalidate pilot wallet"),Pilot,0);TestEqual(TEXT("NPC owner is notified"),NPC,1);TestTrue(TEXT("Global safety revision retained"),FSimDataEvents::CurrentThreadRevision()>Revision);
 FSimDataEvents::Wallet(F.State,FEconomyLedger::Pilot(),F.Owner);TestEqual(TEXT("Personal transaction reaches pilot"),Pilot,1);FSimDataEvents::Publish(F.State);TestEqual(TEXT("Unknown global mutation remains conservative"),Pilot,2);FSimDataEvents::DetailedChanged().Remove(Handle);return true;
}
#endif
