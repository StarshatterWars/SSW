#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "FractalVersePilotMigration.h"
#include "FractalVerseAuthority.h"
#include "FractalVersePilotPromotion.h"
#include "ContractLedger.h"
#include "AcceptanceAI.h"
#include "SettlementAI.h"
#include "LocalGameStateSave.h"
#include "PlayerSaveGame.h"
#include "JsonObjectConverter.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Misc/SecureHash.h"

namespace
{
class FPilotMigrationMemoryIO final:public IPlayerProgressSlotIO
{
public:
    TMap<FString,TArray<uint8>> Files; TArray<FString> Reads;
    int32 Writes=0,Removes=0;
    TFunction<void(const FString&)> BeforeRead;
    bool Exists(const FString& Slot) override { return Files.Contains(Slot); }
    bool Read(const FString& Slot,TArray<uint8>& Out) override
    { Reads.Add(Slot); if (BeforeRead) BeforeRead(Slot); if (const auto* Bytes=Files.Find(Slot)) {Out=*Bytes;return true;} return false; }
    bool Write(const FString&,const TArray<uint8>&) override { ++Writes;return false; }
    bool Remove(const FString&) override { ++Removes;return false; }
    void Put(const FString& Slot,USaveGame* Object)
    { TArray<uint8> Bytes; UGameplayStatics::SaveGameToMemory(Object,Bytes); Files.Add(Slot,MoveTemp(Bytes)); }
};
FString Account(){return TEXT("PlayerSave_")+FString::ChrN(32,TEXT('a'));}
void SeedPilot(FPilotMigrationMemoryIO& IO,int64 Credits=100000)
{
    auto* Save=NewObject<UPlayerSaveGame>(); Save->SaveVersion=2;
    Save->PlayerInfo.Callsign=TEXT("Caloy"); Save->PlayerInfo.LocalStateRevision=1;
    Save->PlayerInfo.Economy.Wallets.Add(FEconomyLedger::Pilot(),Credits);
    Save->PlayerInfo.Economy.bLegacyWalletMigrated=true;
    IO.Put(Account(),Save);
}
FString SeedGeneration(FPilotMigrationMemoryIO& IO,bool Previous=false)
{
    auto* M=NewObject<ULocalStateManifest>(); M->ProfileSlot=Account(); M->Generation=FGuid::NewGuid();
    M->PlayerRevision=5; M->SavedUtc=FDateTime::UtcNow().ToUnixTimestamp();
    auto* Pilot=NewObject<ULocalPilotState>(); Pilot->Pilot.Callsign=TEXT("Caloy");
    Pilot->Pilot.LocalStateRevision=5; Pilot->Pilot.Rank=3; Pilot->Pilot.Campaign=1; Pilot->Pilot.Mission=4;
    Pilot->Pilot.CampaignCompleteMask=(int64(1)<<60)|1;
    Pilot->Pilot.MarkMissionCompleted(1,1); Pilot->Pilot.AgentJournalJson=TEXT("{\"chapter\":1}");
    FPlayerHangarShip Hull; Hull.HullId=FGuid::NewGuid(); Hull.Design=TEXT("Falcon"); Hull.Name=TEXT("Falcon One");
    Hull.LoadoutName=TEXT("Patrol"); Hull.LoadoutStations={1,2}; Hull.bCashPurchased=true;
    Pilot->Pilot.HangarShips.Add(Hull); Pilot->Pilot.SelectedHangarHull=Hull.HullId;
    auto* Economy=NewObject<ULocalEconomyState>(); Economy->Economy.Wallets.Add(FEconomyLedger::Pilot(),425000);
    Economy->Economy.Wallets.Add(TEXT("NPC business"),9000000); Economy->Economy.bLegacyWalletMigrated=true;
    auto* Skills=NewObject<ULocalSkillState>(); FSkillProgress Progress; Progress.TrainedLevel=3;
    Skills->Skills.Add(TEXT("Sensors"),Progress);
    auto* Universe=NewObject<ULocalUniverseState>(); Universe->MissionId=1;
    FLocalShipState Ship; Ship.Name=TEXT("Falcon One"); Ship.Design=TEXT("Falcon"); Universe->Ships.Add(Ship);
    TArray<ULocalStateArea*> Areas={Pilot,Economy,Skills,Universe};
    const TCHAR* Names[]={TEXT("pilot"),TEXT("economy"),TEXT("skills"),TEXT("universe")};
    for (int32 I=0;I<4;++I)
    {
        Areas[I]->Generation=M->Generation; Areas[I]->ProfileSlot=Account();
        const FString Slot=TEXT("State_")+FMD5::HashAnsiString(*Account())+TEXT("_")+M->Generation.ToString(EGuidFormats::Digits)+TEXT("_")+Names[I];
        M->Slots.Add(Slot); IO.Put(Slot,Areas[I]);
    }
    const FString Slot=TEXT("State_")+FMD5::HashAnsiString(*Account())+TEXT("_manifest")+(Previous?TEXT("_previous"):TEXT(""));
    IO.Put(Slot,M); return Slot;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPilotMigrationSnapshotTest,"Starshatter.FractalVerse.Migration.CompleteReadOnlySnapshot",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FPilotMigrationSnapshotTest::RunTest(const FString&)
{
    FPilotMigrationMemoryIO IO; SeedPilot(IO); SeedGeneration(IO);
    const auto Original=IO.Files; FFractalVersePilotCandidate Candidate; FString Reason;
    if (!TestTrue(TEXT("Complete checkpoint prepares"),FFractalVersePilotMigration::Read(IO,Account(),Candidate,Reason))) return false;
    TestEqual(TEXT("Latest checkpoint wallet overrides older base"),Candidate.Credits,int64(425000));
    TestEqual(TEXT("Rank retained"),Candidate.Pilot.Rank,3);
    TestTrue(TEXT("Mission completion retained"),Candidate.Pilot.HasCompletedMission(1,1));
    TestEqual(TEXT("Skill level retained"),Candidate.Pilot.Skills.FindRef(TEXT("Sensors")).TrainedLevel,3);
    TestTrue(TEXT("Saved mission and ship state retained"),Candidate.World.IsValid() && Candidate.World->MissionId==1 && Candidate.World->Ships.Num()==1);
    FS_PlayerGameInfo Career;
    if (!TestTrue(TEXT("Independent career payload decodes"),FJsonObjectConverter::JsonObjectStringToUStruct(Candidate.CareerJson,&Career))) return false;
    if (!TestEqual(TEXT("Hangar retained"),Career.HangarShips.Num(),1)) return false;
    TestEqual(TEXT("Selected hull retained"),Career.SelectedHangarHull,Candidate.Pilot.SelectedHangarHull);
    TestEqual(TEXT("Loadout retained"),Career.HangarShips[0].LoadoutName,FString(TEXT("Patrol")));
    TestTrue(TEXT("Lifetime purchase marker retained"),Career.HangarShips[0].bCashPurchased);
    TestEqual(TEXT("Agent journal retained"),Career.AgentJournalJson,Candidate.Pilot.AgentJournalJson);
    TestEqual(TEXT("64-bit progression flags are exact in career JSON"),Career.CampaignCompleteMask,Candidate.Pilot.CampaignCompleteMask);
    TestEqual(TEXT("Career excludes world economy"),Career.Economy.Wallets.Num(),0);
    TestEqual(TEXT("Career excludes traffic"),Career.Traffic.Journeys.Num(),0);
    TestEqual(TEXT("No source writes"),IO.Writes,0); TestEqual(TEXT("No source removals"),IO.Removes,0);
    for (const auto& File:Original) TestTrue(TEXT("Source bytes unchanged"),IO.Files.FindRef(File.Key)==File.Value);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPilotMigrationDamageTest,"Starshatter.FractalVerse.Migration.RejectIncompleteAndUnsafe",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FPilotMigrationDamageTest::RunTest(const FString&)
{
    FPilotMigrationMemoryIO IO; SeedPilot(IO); const FString Manifest=SeedGeneration(IO);
    FFractalVersePilotCandidate Candidate; FString Reason;
    IO.Files[Manifest]={1,2,3};
    TestFalse(TEXT("Damaged only checkpoint never silently uses stale base"),FFractalVersePilotMigration::Read(IO,Account(),Candidate,Reason));
    SeedGeneration(IO,true);
    TestTrue(TEXT("Complete previous generation can recover"),FFractalVersePilotMigration::Read(IO,Account(),Candidate,Reason));
    TestTrue(TEXT("Recovery is explicit in candidate"),Candidate.bRecoveredPrevious);
    TestEqual(TEXT("Recovered balance retained"),Candidate.Credits,int64(425000));
    IO.Files.Add(PlayerProgressSaveTransaction::JournalSlot(Account()),{1});
    TestFalse(TEXT("Pending transaction blocks migration"),FFractalVersePilotMigration::Read(IO,Account(),Candidate,Reason));
    TestTrue(TEXT("Rejected candidate is cleared"),Candidate.Sources.IsEmpty());
    TestFalse(TEXT("Path cannot choose an account"),FFractalVersePilotMigration::Read(IO,TEXT("../OtherAccount"),Candidate,Reason));
    TestEqual(TEXT("Never repairs source in place"),IO.Writes+IO.Removes,0);
    FPilotMigrationMemoryIO Changing; SeedPilot(Changing); bool First=true;
    Changing.BeforeRead=[&](const FString& Slot)
    {
        if (Slot!=Account()) return;
        if (First) First=false; else SeedPilot(Changing,200000);
    };
    TestFalse(TEXT("Changing source saves block staging"),FFractalVersePilotMigration::Read(Changing,Account(),Candidate,Reason));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPilotMigrationBackupTest,"Starshatter.FractalVerse.Migration.ImmutableVerifiedBackup",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FPilotMigrationBackupTest::RunTest(const FString&)
{
    FPilotMigrationMemoryIO IO; SeedPilot(IO,0); FFractalVersePilotCandidate Candidate; FString Reason,Bundle;
    if (!TestTrue(TEXT("Zero balance is valid"),FFractalVersePilotMigration::Read(IO,Account(),Candidate,Reason))) return false;
    TestEqual(TEXT("No starter-credit top-up"),Candidate.Credits,int64(0));
    const FString Directory=FPaths::ProjectSavedDir()/TEXT("Automation/PilotMigration-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    ON_SCOPE_EXIT { IFileManager::Get().DeleteDirectory(*Directory,false,true); };
    TestFalse(TEXT("Unverified identity cannot stage a bundle"),FFractalVersePilotMigration::Stage(Directory,NAME_None,Candidate,Bundle,Reason));
    IFileManager::Get().MakeDirectory(*(Directory/TEXT("legacy-source")),true);
    FFileHelper::SaveArrayToFile(IO.Files[Account()],*(Directory/TEXT("legacy-source")/(Account()+TEXT(".sav"))));
    FFractalVersePilotCandidate Physical;
    TestTrue(TEXT("Trusted physical source directory reads without using cooked sandbox paths"),
        FFractalVersePilotMigration::ReadDirectory(Directory/TEXT("legacy-source"),Account(),Physical,Reason));
    TestEqual(TEXT("Physical source preserves exact zero balance"),Physical.Credits,int64(0));
    const FName Principal(TEXT("EOS:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    TestFalse(TEXT("Empty backup root is refused"),FFractalVersePilotMigration::Stage(TEXT(""),Principal,Candidate,Bundle,Reason));
    const FString ValidDigest=Candidate.BundleDigest;
    Candidate.BundleDigest=TEXT("../")+FString::ChrN(37,TEXT('a'));
    TestFalse(TEXT("Digest cannot escape the backup root"),FFractalVersePilotMigration::Stage(Directory,Principal,Candidate,Bundle,Reason));
    Candidate.BundleDigest=ValidDigest;
    TestTrue(TEXT("Backup is verified before commit"),FFractalVersePilotMigration::Stage(Directory,Principal,Candidate,Bundle,Reason));
    TArray<uint8> Check; FFileHelper::LoadFileToArray(Check,*(Bundle/(Account()+TEXT(".sav"))));
    TestTrue(TEXT("Exact original save backed up"),Check==IO.Files[Account()]);
    FString Retry; TestTrue(TEXT("Same staged bundle is idempotent"),FFractalVersePilotMigration::Stage(Directory,Principal,Candidate,Retry,Reason));
    TestEqual(TEXT("Retry selects same bundle"),Retry,Bundle);
    FFileHelper::SaveStringToFile(TEXT("damaged"),*(Bundle/(Account()+TEXT(".sav"))));
    TestFalse(TEXT("Existing damaged backup cannot be overwritten"),FFractalVersePilotMigration::Stage(Directory,Principal,Candidate,Retry,Reason));
    const FString Epic=FString::ChrN(32,TEXT('b')), User=FString::ChrN(32,TEXT('c'));
    TestEqual(TEXT("Legacy Epic slot matches existing sign-in convention"),FFractalVersePilotMigration::EpicAccountSlot(Epic,User),
        FString(TEXT("PlayerSave_"))+FMD5::HashAnsiString(*(FString(TEXT("EOS:"))+Epic+TEXT("|")+User)));
    TestTrue(TEXT("Unverified display name is not an account key"),FFractalVersePilotMigration::EpicAccountSlot(TEXT("Caloy"),User).IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPilotPromotionTest,"Starshatter.FractalVerse.Migration.AtomicPromotionAndEconomyContinuation",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FPilotPromotionTest::RunTest(const FString&)
{
    const FName Old=FEconomyLedger::Pilot(), Principal(TEXT("EOS:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    auto* Save=NewObject<UPlayerSaveGame>(); Save->SaveVersion=2; Save->PlayerInfo.Callsign=TEXT("Caloy");
    auto& Source=Save->PlayerInfo.Economy; Source.bLegacyWalletMigrated=true; Source.Wallets.Add(Old,425000);
    FEconomyLedger::CreateInventory(Source,TEXT("Hold"),Old,TEXT("Janus"),100);
    FEconomyLedger::CreateInventory(Source,TEXT("Output"),Old,TEXT("Janus"),100);
    FEconomyLedger::CreateInventory(Source,TEXT("Market"),TEXT("Industry"),TEXT("Janus"),1000)->bMarket=true;
    FEconomyLedger::Deposit(Source,TEXT("Hold"),TEXT("Ore"),20,1);
    const FGuid Blueprint=FEconomyLedger::GrantBlueprint(Source,Old,TEXT("Concentrate"),false,3);
    const FGuid Purchase=FGuid::NewGuid(); Source.BlueprintReceipts.Add(Blueprint,Purchase); Source.CommittedTransactions.Add(Purchase);
    FCommodityAmount Material; Material.Item=TEXT("Ore"); Material.Quantity=2;
    FCommodityAmount Product; Product.Item=TEXT("Concentrate"); Product.Quantity=1;
    const FGuid Job=FEconomyLedger::QueueBlueprintJob(Source,Old,TEXT("Works"),TEXT("Hold"),TEXT("Output"),Blueprint,1,{Material},{Product},10);
    if (!TestTrue(TEXT("Fixture has an active licensed job"),Job.IsValid())) return false;
    FEconomyLedger::AdvanceJobs(Source,2,[](FName){return true;});
    FText MarketReason; const int64 Now=FDateTime::UtcNow().ToUnixTimestamp();
    const FGuid Buy=FEconomyLedger::PlaceMarketOrder(Source,Old,TEXT("Market"),TEXT("Hold"),TEXT("Concentrate"),EMarketOrderSide::Buy,3,1,100,Now,Now+3600,FGuid::NewGuid(),MarketReason);
    const FGuid Sell=FEconomyLedger::PlaceMarketOrder(Source,Old,TEXT("Market"),TEXT("Hold"),TEXT("Ore"),EMarketOrderSide::Sell,2,1,90,Now,Now+3600,FGuid::NewGuid(),MarketReason);
    if (!TestTrue(TEXT("Fixture has buy escrow and sell reservation"),Buy.IsValid() && Sell.IsValid())) return false;
    FContractAccounts Accounts; TMap<FName,FContractTransaction> Contracts;
    FContractLedger::Restore(Source,Contracts,Accounts);
    Accounts.FindOrAdd(TEXT("Issuer")).Credits=100000;
    Accounts.FindChecked(TEXT("Issuer")).StationStock.FindOrAdd(TEXT("A")).Add(TEXT("Supplies"),1);
    FContractTransaction Contract; Contract.Issuer=TEXT("Issuer"); Contract.Terms.Type=ESSWContractType::Courier;
    Contract.Terms.PickupStationId=TEXT("A"); Contract.Terms.DestinationStationId=TEXT("B");
    Contract.Terms.OfferExpiryMinutes=60; Contract.Terms.CompletionDays=1; Contract.Terms.RewardCredits=1000; Contract.Terms.CollateralCredits=2000;
    FSSWContractItem Supplies; Supplies.ItemId=TEXT("Supplies"); Supplies.Quantity=1; Contract.Terms.OfferedItems.Add(Supplies);
    if (!TestTrue(TEXT("Fixture courier is accepted with collateral held"),AcceptanceAI::Publish(Contract,Accounts,FDateTime::UtcNow()) &&
        AcceptanceAI::Accept(Contract,Accounts,Old,FDateTime::UtcNow()))) return false;
    Contracts.Add(TEXT("Delivery"),Contract); FContractLedger::Capture(Source,Contracts,Accounts);
    FEconomyLedger::Deposit(Source,TEXT("Hold"),TEXT("Courier/Delivery"),1,.1);
    Source.Inventories.FindChecked(TEXT("Hold")).ManifestItems.Add(TEXT("Courier/Delivery"));
    FMiningDroneFleetState Fleet; Fleet.ShipDesign=TEXT("Prospector"); Fleet.Region=TEXT("Janek");
    FMiningDroneState Drone; Drone.Id=FGuid::NewGuid(); Drone.State=EMiningDroneState::Returning; Drone.Ore=TEXT("Ore"); Drone.OreTonnes=1.25;
    Fleet.Drones.Add(Drone); Source.MiningDrones.Add(TEXT("Hold"),Fleet);
    FPilotMigrationMemoryIO IO; IO.Put(Account(),Save); FFractalVersePilotCandidate Prepared; FString Reason;
    if (!TestTrue(TEXT("Economic career prepares"),FFractalVersePilotMigration::Read(IO,Account(),Prepared,Reason))) return false;
    FFractalVerseWorld Seed; Seed.Universe=FGuid::NewGuid(); Seed.UniverseBaseUtc=8141644800; Seed.UniverseSeconds=1234; Seed.LastServerUtc=Now;
    Seed.Economy.Wallets.Add(TEXT("Issuer"),1234); Seed.Economy.EconomyClockSeconds=55;
    FEconomyLedger::CreateInventory(Seed.Economy,TEXT("Market"),TEXT("Industry"),TEXT("Janus"),1000)->bMarket=true;
    Seed.Traffic.Sequence=42;
    FFractalVerseAuthority Server; Server.Restore(Seed,Reason);
    if (!TestTrue(TEXT("Prepared career commits in one world candidate"),Server.ImportPilot(Principal,Prepared,Reason))) {AddError(Reason);return false;}
    const auto& Record=Server.Read().Pilots.FindChecked(Principal);
    const FName Hold=Record.InventoryAliases.FindRef(TEXT("Hold")), Output=Record.InventoryAliases.FindRef(TEXT("Output"));
    const FName Delivery=Record.ContractAliases.FindRef(TEXT("Delivery"));
    TestEqual(TEXT("Spendable balance excludes escrow and collateral"),Server.Read().Economy.Wallets.FindRef(Principal),int64(422700));
    TestEqual(TEXT("NPC wallet is not cloned from a private career"),Server.Read().Economy.Wallets.FindRef(TEXT("Issuer")),int64(1234));
    TestEqual(TEXT("Server clock is not replaced by source time"),Server.Read().UniverseSeconds,int64(1234));
    TestEqual(TEXT("Server economy time retained"),Server.Read().Economy.EconomyClockSeconds,55.0);
    TestEqual(TEXT("Global traffic retained"),Server.Read().Traffic.Sequence,42);
    TestEqual(TEXT("Owned inventory has verified owner"),Server.Read().Economy.Inventories.FindChecked(Hold).Owner,Principal);
    TestEqual(TEXT("Inputs already consumed are not restored"),FEconomyLedger::Quantity(Server.Read().Economy,Hold,TEXT("Ore")),18.0);
    TestEqual(TEXT("Sell reservation retained"),FEconomyLedger::Available(Server.Read().Economy,Hold,TEXT("Ore")),16.0);
    TestEqual(TEXT("Returning drone ore is not deposited prematurely"),Server.Read().Economy.MiningDrones.FindChecked(Hold).Drones[0].OreTonnes,1.25);
    TestEqual(TEXT("Courier seal points at imported contract"),FEconomyLedger::Quantity(Server.Read().Economy,Hold,FName(*(TEXT("Courier/")+Delivery.ToString()))),1.0);
    auto Continued=Server.Read();
    TestFalse(TEXT("Account cannot bypass blueprint policy using NPC queue"),FEconomyLedger::QueueJob(Continued.Economy,Principal,TEXT("Works"),Hold,Output,TEXT("Concentrate"),{Material},{Product},10).IsValid());
    FEconomyLedger::AdvanceJobs(Continued.Economy,8,[](FName){return true;});
    TestEqual(TEXT("Running job finishes without consuming twice"),FEconomyLedger::Quantity(Continued.Economy,Output,TEXT("Concentrate")),1.0);
    TestEqual(TEXT("Input remains consumed once"),FEconomyLedger::Quantity(Continued.Economy,Hold,TEXT("Ore")),18.0);
    const FGuid ImportedBuy=FFractalVersePilotPromotion::ScopedId(Principal,Buy);
    TestTrue(TEXT("Imported buy order can refund its own escrow"),FEconomyLedger::CancelMarketOrder(Continued.Economy,Principal,ImportedBuy,FGuid::NewGuid(),MarketReason));
    TestEqual(TEXT("Only reserved buy credits refunded"),Continued.Economy.Wallets.FindRef(Principal),int64(423000));
    FContractLedger::Restore(Continued.Economy,Contracts,Accounts);
    if (!TestTrue(TEXT("Imported courier remains deliverable"),SettlementAI::DeliverCourier(Contracts.FindChecked(Delivery),Accounts,Principal,TEXT("B")))) return false;
    FContractLedger::Capture(Continued.Economy,Contracts,Accounts);
    TestEqual(TEXT("Reward and collateral settle exactly once"),Continued.Economy.Wallets.FindRef(Principal),int64(426000));
    Server.Restore(Continued,Reason); const int64 Revision=Server.Read().Revision;
    TestTrue(TEXT("Retry after spending and settling returns original migration"),Server.ImportPilot(Principal,Prepared,Reason));
    TestEqual(TEXT("Migration retry never restores old balance"),Server.Read().Economy.Wallets.FindRef(Principal),int64(426000));
    TestEqual(TEXT("Migration receipt is idempotent"),Server.Read().Revision,Revision);
    TestFalse(TEXT("Same source cannot fund a second account"),Server.ImportPilot(TEXT("EOS:bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"),Prepared,Reason));
    const FString Directory=FPaths::ProjectSavedDir()/TEXT("Automation/PilotPromotion-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    ON_SCOPE_EXIT {IFileManager::Get().DeleteDirectory(*Directory,false,true);};
    FFractalVerseWorld Loaded;
    TestTrue(TEXT("Profile, receipt and assets checkpoint together"),FFractalVerseCheckpointStore::Save(Directory,Server.Read(),Reason));
    TestTrue(TEXT("Checkpoint reloads"),FFractalVerseCheckpointStore::Load(Directory,Loaded,Reason));
    FFractalVerseAuthority Restarted; Restarted.Restore(Loaded,Reason);
    TestTrue(TEXT("Migration retry after restart is idempotent"),Restarted.ImportPilot(Principal,Prepared,Reason));
    TestEqual(TEXT("Restart retains settled funds"),Restarted.Read().Economy.Wallets.FindRef(Principal),int64(426000));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPilotPromotionTamperTest,"Starshatter.FractalVerse.Migration.RejectTamperedCandidate",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FPilotPromotionTamperTest::RunTest(const FString&)
{
    FPilotMigrationMemoryIO IO; SeedPilot(IO); FFractalVersePilotCandidate Prepared; FString Reason;
    if (!FFractalVersePilotMigration::Read(IO,Account(),Prepared,Reason)) return false;
    FFractalVerseAuthority Server; const FGuid Universe=Server.Read().Universe;
    Prepared.Credits+=1000000;
    TestFalse(TEXT("Prepared balance cannot diverge from immutable source"),Server.ImportPilot(TEXT("EOS:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),Prepared,Reason));
    TestEqual(TEXT("Rejected promotion leaves revision unchanged"),Server.Read().Revision,int64(0));
    TestEqual(TEXT("Rejected promotion leaves universe intact"),Server.Read().Universe,Universe);
    TestEqual(TEXT("Rejected promotion creates no wallet"),Server.Read().Economy.Wallets.Num(),0);
    Prepared.Credits-=1000000;
    int32 Saves=0;
    const FName Principal(TEXT("EOS:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    TestFalse(TEXT("Failed durable save rejects the whole promotion"),FFractalVersePilotCommit::Commit(Server,Principal,Prepared,[&](){++Saves;return false;},Reason));
    TestEqual(TEXT("Save was attempted"),Saves,1);
    TestEqual(TEXT("Failed save rolls back wallets"),Server.Read().Economy.Wallets.Num(),0);
    TestEqual(TEXT("Failed save rolls back profile"),Server.Read().Pilots.Num(),0);
    TestEqual(TEXT("Failed save rolls back migration receipt"),Server.Read().MigrationSources.Num(),0);
    TestEqual(TEXT("Failed save rolls back revision"),Server.Read().Revision,int64(0));
    TestTrue(TEXT("Retry can commit after storage recovers"),FFractalVersePilotCommit::Commit(Server,Principal,Prepared,[&](){++Saves;return true;},Reason));
    TestTrue(TEXT("Acknowledged retry uses the durable receipt"),FFractalVersePilotCommit::Commit(Server,Principal,Prepared,[&](){++Saves;return false;},Reason));
    TestEqual(TEXT("Idempotent retry does not rewrite or grant"),Saves,2);
    return true;
}
#endif
