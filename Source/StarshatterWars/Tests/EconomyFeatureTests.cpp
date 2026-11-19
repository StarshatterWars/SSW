#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Graphic.h"
#include "ForcesKnowledge.h"
#include "SkillsSubsystem.h"
#include "BlueprintCatalogSubsystem.h"
#include "ReprocessingAI.h"
#include "ManufacturingAI.h"
#include "MiningAI.h"
#include "AgentAI.h"
#include "JumpRouteReadout.h"
#include "CockpitMiniControls.h"
#include "FittingMetrics.h"
#include "WeaponDesign.h"
#include "SimShot.h"
#include "Camera.h"
#include "Asteroid.h"
#include "SimRegion.h"
#include "PlayerSaveGame.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "ContractLedger.h"
#include "AcceptanceAI.h"
#include "SettlementAI.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSkillBooksTest,"Starshatter.Economy.SkillBooksAndRankGates",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FSkillBooksTest::RunTest(const FString&)
{
    auto* Game=NewObject<UGameInstance>(); auto* Catalog=NewObject<USkillsSubsystem>(Game);
    if (!TestTrue(TEXT("Load JSON-only skill and book catalogs"),Catalog->LoadCatalog())) return false;
    TestTrue(TEXT("Substantial SSW skill tree"),Catalog->GetCatalog().Num()>=100);
    const auto* Frigate=Catalog->GetCatalog().Find(TEXT("frigate_operations"));
    if (!TestNotNull(TEXT("Frigate qualification exists"),Frigate)) return false;
    FEconomyState State; State.Wallets.Add(FEconomyLedger::Pilot(),1000000);
    TMap<FName,FSkillProgress> Learned; FText Reason;
    TestFalse(TEXT("Rank alone cannot buy advanced skill"),FSkillRules::Purchase(State,Learned,*Frigate,3,TEXT("Terellian"),100,Reason));
    for (FName Required:Frigate->Prerequisites) Learned.FindOrAdd(Required).bComplete=true;
    TestFalse(TEXT("Prerequisites cannot bypass rank"),FSkillRules::Purchase(State,Learned,*Frigate,2,TEXT("Terellian"),100,Reason));
    TestEqual(TEXT("Blocked purchase does not charge"),State.Wallets.FindRef(FEconomyLedger::Pilot()),1000000LL);
    TestTrue(TEXT("Qualified pilot buys complete skill"),FSkillRules::Purchase(State,Learned,*Frigate,3,TEXT("Terellian"),100,Reason));
    TestFalse(TEXT("Book ownership does not grant a trained level"),Learned.FindChecked(Frigate->Name).bComplete);
    TestEqual(TEXT("No level-training timer starts"),Learned.FindChecked(Frigate->Name).TrainingFinishesUtc,0LL);
    TestEqual(TEXT("Book costs SSW credits"),State.Wallets.FindRef(FEconomyLedger::Pilot()),1000000LL-Frigate->BookPrice);
    TestEqual(TEXT("Wallet identifies the purchased skill"),State.Journal.Last().Item,Frigate->Name);
    TestEqual(TEXT("Skill receipt links the learned entitlement"),State.Journal.Last().Reference,Learned.FindChecked(Frigate->Name).Id);
    TestFalse(TEXT("Already learned book cannot charge twice"),FSkillRules::Purchase(State,Learned,*Frigate,3,TEXT("Terellian"),101,Reason));
    TestEqual(TEXT("Hull requires its own skill"),USkillsSubsystem::RequiredHullSkill(static_cast<uint32>(CLASSIFICATION::FRIGATE)),FName(TEXT("frigate_operations")));
    auto* Save=NewObject<UPlayerSaveGame>(); Save->PlayerInfo.Skills=Learned; Save->PlayerInfo.Economy=State; TArray<uint8> Bytes;
    TestTrue(TEXT("Skills save with pilot"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
    const auto* Loaded=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Reload pilot skills"),Loaded)) return false;
    TestTrue(TEXT("Book ownership survives reload"),Loaded->PlayerInfo.Skills.FindChecked(Frigate->Name).Id.IsValid());
    TestEqual(TEXT("Skill purchase identity survives reload"),Loaded->PlayerInfo.Economy.Journal.Last().Item,Frigate->Name);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlueprintRightsTest,"Starshatter.Economy.PurchasedBlueprintRights",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FBlueprintRightsTest::RunTest(const FString&)
{
    FEconomyState State; const FName Pilot=FEconomyLedger::Pilot(); State.Wallets.Add(Pilot,10000);
    FEconomyLedger::CreateInventory(State,TEXT("Input"),Pilot,TEXT("Janus"),100);
    FEconomyLedger::CreateInventory(State,TEXT("Output"),Pilot,TEXT("Janus"),100);
    FCommodityAmount Ore; Ore.Item=TEXT("magnetite"); Ore.Quantity=4;
    FCommodityAmount Product; Product.Item=TEXT("industrial_concentrate"); Product.Quantity=2;
    FEconomyLedger::Deposit(State,TEXT("Input"),Ore.Item,20,1);
    TestFalse(TEXT("Raw player job cannot bypass blueprint"),FEconomyLedger::QueueJob(State,Pilot,TEXT("Refinery"),TEXT("Input"),TEXT("Output"),TEXT("Concentrate"),{Ore},{Product},10).IsValid());
    const auto Unpaid=FEconomyLedger::GrantBlueprint(State,Pilot,TEXT("Concentrate"),true,0);
    TestFalse(TEXT("Unpurchased player blueprint rejected"),FEconomyLedger::QueueBlueprintJob(State,Pilot,TEXT("Refinery"),TEXT("Input"),TEXT("Output"),Unpaid,1,{Ore},{Product},10).IsValid());
    FBlueprintOffer Offer; Offer.Recipe=TEXT("Concentrate"); Offer.OriginalPrice=1000; Offer.CopyPrice=100; Offer.CopyRuns=2;
    const auto Copy=UBlueprintCatalogSubsystem::PurchaseLicense(State,Offer,false);
    TestTrue(TEXT("NPC copy purchase creates entitlement"),Copy.IsValid());
    TestEqual(TEXT("NPC copy charged once"),State.Wallets.FindRef(Pilot),9900LL);
    TestEqual(TEXT("Blueprint wallet entry identifies its recipe"),State.Journal.Last().Item,Offer.Recipe);
    TestEqual(TEXT("Blueprint wallet entry links the purchased license"),State.Journal.Last().Reference,Copy);
    const auto Job=FEconomyLedger::QueueBlueprintJob(State,Pilot,TEXT("Refinery"),TEXT("Input"),TEXT("Output"),Copy,2,{Ore},{Product},10);
    TestTrue(TEXT("Purchased blueprint installs job"),Job.IsValid());
    TestFalse(TEXT("One blueprint cannot install simultaneous jobs"),FEconomyLedger::QueueBlueprintJob(State,Pilot,TEXT("Refinery"),TEXT("Input"),TEXT("Output"),Copy,1,{Ore},{Product},10).IsValid());
    FEconomyLedger::AdvanceJobs(State,1,[](FName){return true;});
    TestEqual(TEXT("Copy runs consumed when job starts"),State.Blueprints.FindChecked(Copy).RunsRemaining,0);
    auto* Save=NewObject<UPlayerSaveGame>(); Save->PlayerInfo.Economy=State; TArray<uint8> Bytes;
    TestTrue(TEXT("Save purchased blueprint and running job"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
    const auto* Loaded=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Reload blueprint entitlement"),Loaded)) return false;
    State=Loaded->PlayerInfo.Economy;
    TestTrue(TEXT("Purchase receipt survives"),State.BlueprintReceipts.Contains(Copy));
    TestEqual(TEXT("Blueprint wallet identity survives reload"),State.Journal.Last().Reference,Copy);
    TestEqual(TEXT("Resume completed batch"),FEconomyLedger::AdvanceJobs(State,19,[](FName){return true;}),1);
    TestEqual(TEXT("Two runs create exact output"),FEconomyLedger::Quantity(State,TEXT("Output"),Product.Item),4.0);
    TestFalse(TEXT("Exhausted copy cannot build"),FEconomyLedger::QueueBlueprintJob(State,Pilot,TEXT("Refinery"),TEXT("Input"),TEXT("Output"),Copy,1,{Ore},{Product},10).IsValid());
    const auto Original=UBlueprintCatalogSubsystem::PurchaseLicense(State,Offer,true);
    TestTrue(TEXT("Original has unlimited rights"),State.Blueprints.FindChecked(Original).bOriginal);
    FSSWProductionRecipe Recipe; Recipe.CargoItem=TEXT("concentrate"); Recipe.InputTonnes=4; Recipe.OutputTonnes=2; Recipe.OutputQuantity=2; Recipe.SecondsPerBatch=10;
    const auto Quote=ReprocessingAI::Calculate(Recipe,2,0.1,0.2);
    TestTrue(TEXT("Refining efficiency quote valid"),Quote.bValid);
    TestTrue(TEXT("Input/output/residue conserve mass"),FMath::IsNearlyEqual(Quote.InputTonnes,Quote.OutputTonnes+Quote.WasteTonnes));
    TestEqual(TEXT("Time modifier applied to full job"),Quote.Seconds,16.0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndustryPersistenceTest,"Starshatter.Economy.AgentAndContractPersistence",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FIndustryPersistenceTest::RunTest(const FString&)
{
    AgentAI Agent; const FName Pilot(TEXT("Pilot")); const int32 Eligible[]={101,102};
    Agent.Request(Pilot,Eligible,TEXT("Janek Instructor")); TestTrue(TEXT("Accept mission"),Agent.Accept(Pilot,101));
    TestTrue(TEXT("Start mission"),Agent.Start(Pilot,101));
    AgentAI Restored; TestTrue(TEXT("Restore mission journal"),Restored.RestoreJournal(Agent.SaveJournal()));
    TestEqual(TEXT("Interrupted assignment returns queued"),Restored.GetQueue(Pilot)[0].State,AgentAI::EState::Accepted);
    Restored.Start(Pilot,101); Restored.Resolve(Pilot,101,true);
    const auto* Next=Restored.Request(Pilot,Eligible); if (!TestNotNull(TEXT("Next offer"),Next)) return false;
    TestEqual(TEXT("Completed mission is not reoffered"),Next->MissionId,102);
    TestFalse(TEXT("Malformed journal rejected"),Restored.RestoreJournal(TEXT("[{}]")));
    TestEqual(TEXT("Rejected journal preserves queue state"),Restored.FindOffer(Pilot)->MissionId,102);
    FEconomyState Economy; FContractAccounts Accounts; Accounts.Add(TEXT("Issuer")).Credits=1000; Accounts.FindChecked(TEXT("Issuer")).StationStock.FindOrAdd(TEXT("Depot")).Add(TEXT("ore"),1);
    Accounts.Add(Pilot).Credits=1000;
    FContractTransaction Contract; Contract.Issuer=TEXT("Issuer"); Contract.Terms.PickupStationId=TEXT("Depot"); Contract.Terms.DestinationStationId=TEXT("Refinery");
    Contract.Terms.OfferExpiryMinutes=10; Contract.Terms.CompletionDays=1; Contract.Terms.RewardCredits=100; Contract.Terms.CollateralCredits=50;
    auto& Item=Contract.Terms.OfferedItems.AddDefaulted_GetRef(); Item.ItemId=TEXT("ore"); Item.Quantity=1;
    const FDateTime Now(2228,1,1); AcceptanceAI::Publish(Contract,Accounts,Now); AcceptanceAI::Accept(Contract,Accounts,Pilot,Now);
    TMap<FName,FContractTransaction> Contracts; Contracts.Add(TEXT("Delivery"),Contract); FContractLedger::Capture(Economy,Contracts,Accounts);
    FContractAccounts Reloaded; TMap<FName,FContractTransaction> ReloadedContracts; FContractLedger::Restore(Economy,ReloadedContracts,Reloaded);
    TestEqual(TEXT("Contract identity retained"),ReloadedContracts.FindChecked(TEXT("Delivery")).Id,Contract.Id);
    auto& Delivery=ReloadedContracts.FindChecked(TEXT("Delivery"));
    TestTrue(TEXT("Resumed contract settles"),SettlementAI::DeliverCourier(Delivery,Reloaded,Pilot,TEXT("Refinery")));
    FContractLedger::Capture(Economy,ReloadedContracts,Reloaded); FContractLedger::Restore(Economy,ReloadedContracts,Reloaded);
    TestFalse(TEXT("Reload cannot pay contract twice"),SettlementAI::DeliverCourier(ReloadedContracts.FindChecked(TEXT("Delivery")),Reloaded,Pilot,TEXT("Refinery")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMiningLeaseTest,"Starshatter.Economy.MiningKnowledgeAndClaims",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FMiningLeaseTest::RunTest(const FString&)
{
    SimRegion Region(nullptr,"Mining audit",SimRegion::REAL_SPACE);
    auto* A=new Asteroid(1,FVector(1000,0,0),5e7); auto* B=new Asteroid(2,FVector(1200,0,0),5e7);
    Region.InsertObject(A); Region.InsertObject(B);
    const TArray<ESSWMineral> Minerals={A->GetResource().Mineral,B->GetResource().Mineral};
    auto Survey=MiningAI::ScanAsteroids(Region,FVector::ZeroVector,5000,MiningAI::ScanTier::Survey);
    TestNull(TEXT("Unknown quantities do not authorize extraction"),MiningAI::SelectDeposit(Region,FVector::ZeroVector,Survey,Minerals,TEXT(""),{}));
    auto Assay=MiningAI::ScanAsteroids(Region,FVector::ZeroVector,5000,MiningAI::ScanTier::Assay);
    FEconomyState State;
    auto* First=MiningAI::SelectClaimedDeposit(State,TEXT("Miner A"),Region,FVector::ZeroVector,Assay,Minerals,TEXT(""),{});
    auto* Second=MiningAI::SelectClaimedDeposit(State,TEXT("Miner B"),Region,FVector::ZeroVector,Assay,Minerals,TEXT(""),{});
    TestTrue(TEXT("Miners take separate deposits"),First && Second && First!=Second);
    Assay.ScannedAt-=6;
    TestNull(TEXT("Stale scan requires rescan"),MiningAI::SelectDeposit(Region,FVector::ZeroVector,Assay,Minerals,TEXT(""),{}));
    A->ExtractOre(A->GetResource().AmountTonnes); B->ExtractOre(B->GetResource().AmountTonnes);
    TestEqual(TEXT("Assay identifies depleted belt"),MiningAI::ScanAsteroids(Region,FVector::ZeroVector,5000,MiningAI::ScanTier::Assay).Status,MiningAI::BeltStatus::Depleted);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFittingCalculationTest,"Starshatter.Economy.SSWDerivedFittingAndRoute",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FFittingCalculationTest::RunTest(const FString&)
{
    WeaponDesign Design; Design.graphic_type=Graphic::SOLID; Design.damage=10; Design.charge=2; Design.life=3; Design.refire_delay=0.5; Design.salvo_delay=0; Design.recharge_rate=2; Design.syncro=true; Design.primary=true; Design.beam=false;
    Camera Cam; SimShot Shot(FVector::ZeroVector,Cam,&Design); Shot.SetCharge(Design.charge);
    const auto Metrics=FFittingMetrics::Weapon(Design,1);
    TestTrue(TEXT("Volley uses actual SSW shot damage"),FMath::IsNearlyEqual(Metrics.VolleyDamage,Shot.Damage()));
    TestTrue(TEXT("Sustained output respects recharge"),Metrics.SustainedDPS<Metrics.BurstDPS);
    Design.name="Mining Laser"; TestEqual(TEXT("Mining laser has no combat DPS"),FFittingMetrics::Weapon(Design,1).BurstDPS,0.0);
    const TArray<FString> Route={TEXT("Janus"),TEXT("Jarnell"),TEXT("Borova")};
    TestEqual(TEXT("Route advances after actual system change"),FJumpRouteModel::RemainingRoute(Route,TEXT("jarnell")).Num(),2);
    TestEqual(TEXT("Arrival retains only destination"),FJumpRouteModel::RemainingRoute(Route,TEXT("Borova")).Num(),1);
    TestTrue(TEXT("Off-route location never advances silently"),FJumpRouteModel::RemainingRoute(Route,TEXT("Solus")).IsEmpty());
    TestEqual(TEXT("Cockpit exit shortcut is last"),SCockpitMiniControls::Shortcuts().Last().Action,FName(TEXT("Exit")));
    TestEqual(TEXT("Outside cockpit buttons cannot trigger actions"),SCockpitMiniControls::CellAt(FVector2D(800,10)),INDEX_NONE);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForcesScopeTest,"Starshatter.Economy.ForcesScopeAuthorization",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FForcesScopeTest::RunTest(const FString&)
{
    TArray<FForcesReport> Reports;
    const auto Add=[&](const TCHAR* System,const TCHAR* Sector,int32 IFF,bool Reported,bool CampaignAllowed)
    { FForcesReport R;R.System=System;R.Sector=Sector;R.IFF=IFF;R.bReported=Reported;R.bCampaignAllowed=CampaignAllowed;Reports.Add(R); };
    Add(TEXT("Janus"),TEXT("Janek"),1,true,true);
    Add(TEXT("Janus"),TEXT("Janek"),0,true,true);
    Add(TEXT("Janus"),TEXT("Janek"),2,true,true);
    Add(TEXT("Janus"),TEXT("Janek"),-1000,true,true);
    Add(TEXT("Janus"),TEXT("Janek"),2,true,false);
    Add(TEXT("Janus"),TEXT("Other sector"),1,true,true);
    Add(TEXT("Jarnell"),TEXT("Orbit"),1,true,true);
    Add(TEXT("Distant"),TEXT("Orbit"),1,true,true);
    const TSet<FName> Neighbors={TEXT("Jarnell")};
    const auto Filter=[&](int32 Level,EForcesScope Scope)
    {return FForcesKnowledge::Filter(Reports,1,TEXT("Janus"),TEXT("Janek"),NAME_None,Scope,Level,Neighbors);};
    TestEqual(TEXT("No skill reveals no forces"),Filter(0,EForcesScope::Sector).Num(),0);
    TestEqual(TEXT("Level I includes friendly neutral and known enemy only"),Filter(1,EForcesScope::Neighbors).Num(),3);
    TestEqual(TEXT("Level III extends to current system"),Filter(3,EForcesScope::Neighbors).Num(),4);
    TestEqual(TEXT("Level V adds only directly neighboring system"),Filter(5,EForcesScope::Neighbors).Num(),5);
    TestEqual(TEXT("Explicit sector selection retains scope"),Filter(5,EForcesScope::Sector).Num(),3);
    return true;
}
#endif
