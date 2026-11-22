#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include <limits>
#include "FractalVerseAuthority.h"
#include "FractalVerseSubsystem.h"
#include "FractalVerseIdentity.h"
#include "FractalVerseSession.h"
#include "FractalVerseFlightGate.h"
#include "SSWRuntimeSubsystem.h"
#include "FractalVerseGameMode.h"
#include "../StarshatterWarsGameMode.h"
#include "StarshatterInputBindings.h"
#include "InputMappingContext.h"
#include "GalaxyTrafficSubsystem.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFractalVerseFlightGateTest,"Starshatter.FractalVerse.Ships.ControlOwnershipAndTimeout",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FFractalVerseFlightGateTest::RunTest(const FString&)
{
    FFractalVerseFlightGate Gate; const FName A(TEXT("EOS:A")),B(TEXT("EOS:B"));
    FFractalVerseFlightInput Input; Input.Sequence=1; Input.Throttle=70;
    Input.Rotation=FVector(.2,-.4,1); Input.Translation=FVector(1,0,0); Input.bPrimary=true;
    TestFalse(TEXT("Unassigned account cannot steer"),Gate.Submit(A,Input,1));
    TestFalse(TEXT("Anonymous ship assignment rejected"),Gate.Bind(NAME_None,7));
    TestFalse(TEXT("Zero object ID rejected"),Gate.Bind(A,0));
    TestTrue(TEXT("Server assigns ship"),Gate.Bind(A,7));
    TestFalse(TEXT("Second pilot cannot acquire occupied ship"),Gate.Bind(B,7));
    TestTrue(TEXT("Second pilot has separate ship"),Gate.Bind(B,8));
    TestTrue(TEXT("Validated control reaches own lease"),Gate.Submit(A,Input,1));
    TestEqual(TEXT("Own throttle retained"),Gate.Read(A,1.1).Throttle,70.f);
    TestEqual(TEXT("Other ship remains idle"),Gate.Read(B,1.1).Throttle,0.f);
    TestFalse(TEXT("Replay rejected"),Gate.Submit(A,Input,1.1));
    Input.Sequence=2;
    TestFalse(TEXT("Input flood rejected"),Gate.Submit(A,Input,1.001));
    Input.Throttle=101; TestFalse(TEXT("Invalid throttle rejected"),Gate.Submit(A,Input,1.1));
    Input.Throttle=70; Input.Translation.X=2;
    TestFalse(TEXT("Oversized thruster input rejected"),Gate.Submit(A,Input,1.1));
    Input.Translation.X=1; Input.Rotation.X=std::numeric_limits<double>::quiet_NaN();
    TestFalse(TEXT("Nonfinite axis rejected"),Gate.Submit(A,Input,1.1));
    Input.Rotation.X=0;
    TestFalse(TEXT("Backwards server clock rejected"),Gate.Submit(A,Input,.9));
    TestTrue(TEXT("Next valid sequence accepted"),Gate.Submit(A,Input,1.1));
    const auto Stale=Gate.Read(A,1.351);
    TestEqual(TEXT("Lost input stops throttle"),Stale.Throttle,0.f);
    TestTrue(TEXT("Lost input stops maneuvering"),Stale.Translation.IsZero() && Stale.Rotation.IsZero());
    TestFalse(TEXT("Lost input stops firing"),Stale.bPrimary);
    Gate.Revoke(A);
    TestFalse(TEXT("Logout cannot keep controlling ship"),Gate.Submit(A,Input,2));
    TestTrue(TEXT("Released ship can be assigned to another pilot"),Gate.Bind(FName(TEXT("EOS:C")),7));
    Gate.Reset(); TestEqual(TEXT("World teardown removes bindings"),Gate.Object(B),uint32(0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFractalVerseSessionTest,"Starshatter.FractalVerse.Identity.LeaseAndLogout",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FFractalVerseSessionTest::RunTest(const FString&)
{
    FFractalVerseSession Session; const FName A(TEXT("EOS:A")),B(TEXT("EOS:B"));
    TestTrue(TEXT("Verified account opens a bounded session"),Session.Bind(A,100));
    TestEqual(TEXT("Session owns only its verified account"),Session.Principal(101),A);
    TestFalse(TEXT("No unnecessary refresh"),Session.NeedsRefresh(339));
    TestTrue(TEXT("Refresh begins before expiry"),Session.NeedsRefresh(340));
    TestTrue(TEXT("Expired session has no command authority"),Session.Principal(400).IsNone());
    TestTrue(TEXT("Same account can reauthenticate"),Session.Bind(A,400));
    TestFalse(TEXT("Different account cannot replace a bound connection"),Session.Bind(B,410));
    TestTrue(TEXT("Mismatch clears previous authority"),Session.Principal(411).IsNone());
    Session.Bind(A,500); Session.Clear();
    TestTrue(TEXT("Logout clears authority immediately"),Session.Principal(501).IsNone());
    TestFalse(TEXT("Invalid server clock cannot grant authority"),Session.Bind(A,-1));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFractalVersePilotTest, "Starshatter.FractalVerse.Identity.AccountIsolationAndGrant",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FFractalVersePilotTest::RunTest(const FString&)
{
    FFractalVerseAuthority Server; FString Reason;
    const FName A(TEXT("EOS:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")), B(TEXT("EOS:bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"));
    TestFalse(TEXT("Missing verified principal cannot create a wallet"),Server.EnsurePilot(NAME_None,Reason));
    TestTrue(TEXT("First verified pilot is created"),Server.EnsurePilot(A,Reason));
    TestTrue(TEXT("Second verified pilot has its own record"),Server.EnsurePilot(B,Reason));
    FFractalVersePilotView View;
    TestTrue(TEXT("Own record is available to trusted context"),Server.PilotView(A,View));
    TestEqual(TEXT("New pilot allowance"),View.Credits,int64(50000));
    TestEqual(TEXT("Home region comes from server defaults"),View.HomeRegion,FName(TEXT("Janek")));
    FFractalVerseWorld Modified=Server.Read(); Modified.Economy.Wallets[A]=12345;
    TestTrue(TEXT("Server-owned balance restores"),Server.Restore(Modified,Reason));
    const int64 Revision=Server.Read().Revision;
    TestTrue(TEXT("Reauthentication is idempotent"),Server.EnsurePilot(A,Reason));
    Server.PilotView(A,View); TestEqual(TEXT("Reconnect never tops up spent credits"),View.Credits,int64(12345));
    TestEqual(TEXT("Reconnect never advances revision"),Server.Read().Revision,Revision);
    Server.PilotView(B,View); TestEqual(TEXT("Other pilot balance is isolated"),View.Credits,int64(50000));
    TestEqual(TEXT("Only one opening grant per pilot"),Server.Read().Economy.Journal.Num(),2);
    TestFalse(TEXT("Unbound context cannot read another pilot"),Server.PilotView(NAME_None,View));
    TestFalse(TEXT("Rejected view is cleared"),View.bAuthenticated);
    Modified=Server.Read(); Modified.Economy.Wallets.Add(TEXT("Unmigrated"),900000);
    Server.Restore(Modified,Reason);
    TestFalse(TEXT("Existing wallet without a migrated profile is never overwritten"),Server.EnsurePilot(TEXT("Unmigrated"),Reason));
    const FString Directory=FPaths::ProjectSavedDir()/TEXT("Automation/FractalVersePilot-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    ON_SCOPE_EXIT { IFileManager::Get().DeleteDirectory(*Directory,false,true); };
    FFractalVerseWorld Saved;
    TestTrue(TEXT("Pilot and wallet checkpoint together"),FFractalVerseCheckpointStore::Save(Directory,Server.Read(),Reason));
    TestTrue(TEXT("Pilot checkpoint reloads"),FFractalVerseCheckpointStore::Load(Directory,Saved,Reason));
    FFractalVerseAuthority Restarted; Restarted.Restore(Saved,Reason); Restarted.EnsurePilot(A,Reason);
    Restarted.PilotView(A,View); TestEqual(TEXT("Restart retains credits without a second grant"),View.Credits,int64(12345));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFractalVerseIdentityProofTest, "Starshatter.FractalVerse.Identity.InvalidProofRejected",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FFractalVerseIdentityProofTest::RunTest(const FString&)
{
    const FString User=TEXT("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    const FString Shape=TEXT("aaaaaaaaaaaa.bbbbbbbbbbbb.cccccccccccc");
    TestTrue(TEXT("Token syntax is not confused with verification"),FFractalVerseIdentity::IsProofWellFormed(User,Shape));
    TestFalse(TEXT("Display name is not a product user ID"),FFractalVerseIdentity::IsProofWellFormed(TEXT("Pilot"),Shape));
    TestFalse(TEXT("Empty proof rejected"),FFractalVerseIdentity::IsProofWellFormed(User,TEXT("")));
    TestFalse(TEXT("Oversize proof rejected"),FFractalVerseIdentity::IsProofWellFormed(User,FString::ChrN(17000,TEXT('a'))));
    TestFalse(TEXT("Missing JWT signature rejected"),FFractalVerseIdentity::IsProofWellFormed(User,TEXT("aaaaaaaaaaaaaaaa.bbbbbbbbbbbbbbbb.")));
    TestFalse(TEXT("Header injection rejected"),FFractalVerseIdentity::IsProofWellFormed(User,Shape+TEXT("\r\n")));
    bool Called=false;
    FFractalVerseIdentity::Verify(User,TEXT("invalid"),[&](FName Principal){Called=true;TestTrue(TEXT("Invalid proof yields no principal"),Principal.IsNone());});
    TestTrue(TEXT("Malformed proof fails without contacting EOS"),Called);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFractalVerseCommandTest, "Starshatter.FractalVerse.Commands.OwnershipRevisionAndRetry",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FFractalVerseCommandTest::RunTest(const FString&)
{
    FFractalVerseAuthority Server;
    FFractalVerseWorld Seed;
    Seed.Universe=FGuid::NewGuid();
    const FName Pilot(TEXT("Pilot A")), Source(TEXT("Hold A")), Destination(TEXT("Locker A")), Ore(TEXT("Ore"));
    FEconomyLedger::CreateInventory(Seed.Economy, Source, Pilot, TEXT("Janus"), 100);
    FEconomyLedger::CreateInventory(Seed.Economy, Destination, Pilot, TEXT("Janus"), 100);
    FEconomyLedger::Deposit(Seed.Economy, Source, Ore, 20, 1);
    FString Reason;
    TestTrue(TEXT("Server restores its own state"), Server.Restore(Seed, Reason));
    FFractalVerseTransfer Command;
    Command.Request=FGuid::NewGuid(); Command.Source=Source; Command.Destination=Destination; Command.Item=Ore; Command.Quantity=5;
    FFractalVerseAccess Access;
    Access.Principal=Pilot; Access.AccessibleInventories={Source,Destination};
    TestFalse(TEXT("Undocked request rejected"), Server.Transfer(Access,Command,Reason));
    Access.bDocked=true;
    Access.Principal=TEXT("Another pilot");
    TestFalse(TEXT("Another account cannot use someone else's holds"), Server.Transfer(Access,Command,Reason));
    Access.Principal=Pilot; Command.ExpectedRevision=5;
    TestFalse(TEXT("Stale revision rejected"), Server.Transfer(Access,Command,Reason));
    Command.ExpectedRevision=0;
    TestTrue(TEXT("Validated docked transfer succeeds"), Server.Transfer(Access,Command,Reason));
    TestEqual(TEXT("Committed world advances once"), Server.Read().Revision, int64(1));
    TestTrue(TEXT("Identical retry returns original success"), Server.Transfer(Access,Command,Reason));
    TestEqual(TEXT("Retry never transfers twice"), FEconomyLedger::Quantity(Server.Read().Economy,Destination,Ore),5.0);
    Command.Quantity=7;
    TestFalse(TEXT("Changed payload cannot reuse a receipt"), Server.Transfer(Access,Command,Reason));
    TestEqual(TEXT("Source remains exact after rejected commands"),FEconomyLedger::Quantity(Server.Read().Economy,Source,Ore),15.0);
    FFractalVerseAuthority Restarted;
    TestTrue(TEXT("Server receipts restore with inventory"),Restarted.Restore(Server.Read(),Reason));
    Command.Quantity=5;
    TestTrue(TEXT("Retry after restart stays idempotent"),Restarted.Transfer(Access,Command,Reason));
    TestEqual(TEXT("Restart retry revision unchanged"),Restarted.Read().Revision,int64(1));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFractalVerseCheckpointTest, "Starshatter.FractalVerse.Persistence.InterruptionAndRecovery",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FFractalVerseCheckpointTest::RunTest(const FString&)
{
    const FString Directory=FPaths::ProjectSavedDir()/TEXT("Automation/FractalVerse-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    ON_SCOPE_EXIT { IFileManager::Get().DeleteDirectory(*Directory,false,true); };
    FFractalVerseWorld State, Loaded; FString Reason;
    State.Universe=FGuid::NewGuid();
    State.Revision=10; State.Economy.Wallets.Add(TEXT("Pilot"),12345);
    if(!TestTrue(TEXT("First server generation saved"),FFractalVerseCheckpointStore::Save(Directory,State,Reason)))return false;
    State.Revision=11; State.Economy.Wallets[TEXT("Pilot")]=12000;
    TestTrue(TEXT("Second generation committed"),FFractalVerseCheckpointStore::Save(Directory,State,Reason));
    FFileHelper::SaveStringToFile(TEXT("unfinished"),*(Directory/TEXT("head.pending")));
    TestTrue(TEXT("Interrupted pending index ignored"),FFractalVerseCheckpointStore::Load(Directory,Loaded,Reason));
    TestEqual(TEXT("Latest complete generation loaded"),Loaded.Revision,int64(11));
    FString Head;FFileHelper::LoadFileToString(Head,*(Directory/TEXT("head")));
    const TCHAR* Active=Head.StartsWith(TEXT("0"))?TEXT("world-0.sav"):TEXT("world-1.sav");
    FFileHelper::SaveStringToFile(TEXT("corrupt payload"),*(Directory/Active));
    TestTrue(TEXT("Damaged latest generation falls back"),FFractalVerseCheckpointStore::Load(Directory,Loaded,Reason));
    TestEqual(TEXT("Recovered previous coherent generation"),Loaded.Revision,int64(10));
    TestEqual(TEXT("Recovered wallet matches that generation"),Loaded.Economy.Wallets.FindRef(TEXT("Pilot")),int64(12345));
    TestTrue(TEXT("Recovered generation can checkpoint again"),FFractalVerseCheckpointStore::Save(Directory,Loaded,Reason));
    TestTrue(TEXT("New head survives recovery"),FFractalVerseCheckpointStore::Load(Directory,State,Reason));
    TestEqual(TEXT("Universe identity preserved"),State.Universe,Loaded.Universe);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFractalVerseUnloadedTest, "Starshatter.FractalVerse.Simulation.WithoutPlayerOrLevel",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FFractalVerseUnloadedTest::RunTest(const FString&)
{
    auto* Instance=NewObject<UGameInstance>();
    auto* Traffic=NewObject<UGalaxyTrafficSubsystem>(Instance);
    if(!TestTrue(TEXT("Server catalogs load without a mission"),Traffic->LoadEconomyCatalogs()))return false;
    FFractalVerseAuthority Server;
    TestTrue(TEXT("Dedicated native step"),Server.Advance(2,[Traffic](FFractalVerseWorld& State,double Seconds){Traffic->AdvanceServerWorld(State,Seconds);}));
    TestEqual(TEXT("World clock advances without any player"),Server.Read().Economy.EconomyClockSeconds,2.0);
    TestTrue(TEXT("Unloaded mines create real inventories"),Server.Read().Economy.Inventories.Num()>0);
    TestEqual(TEXT("Mining grants no ore before its scheduled cycle"),Server.Read().Economy.BackgroundOreRemaining.Num(),0);
    TestTrue(TEXT("Scheduled mining cycle advances without a level"),Server.Advance(60,[Traffic](FFractalVerseWorld& State,double Seconds){Traffic->AdvanceServerWorld(State,Seconds);}));
    TestTrue(TEXT("Finite deposit depletion is server-owned"),Server.Read().Economy.BackgroundOreRemaining.Num()>0);
    const auto ClientCopy=Server.Status(true);
    TestEqual(TEXT("Read model contains confirmed revision"),ClientCopy.Revision,int64(2));
    TestFalse(TEXT("Invalid time cannot advance authority"),Server.Advance(-1,[](auto&,double){}));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFractalVerseClockTest, "Starshatter.FractalVerse.Clock.ServerOnlyLeapDayAndOfflineContinuity",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FFractalVerseClockTest::RunTest(const FString&)
{
    FFractalVerseAuthority Server;
    const int64 Gmt=FDateTime(2028,2,28,23,59,59).ToUnixTimestamp();
    TestTrue(TEXT("Initial clock comes from server GMT"),Server.SynchronizeClock(Gmt));
    const auto Before=Server.Status(true);
    TestEqual(TEXT("Calendar keeps universe epoch"),FDateTime::FromUnixTimestamp(Before.UniverseBaseUtc+Before.UniverseSeconds).GetYear(),2228);
    Server.Advance(50,[](auto&,double){});
    TestEqual(TEXT("Simulation delta cannot accelerate universe clock"),Server.Read().UniverseSeconds,Before.UniverseSeconds);
    Server.SynchronizeClock(Gmt+2);
    const auto Leap=FDateTime::FromUnixTimestamp(Server.Read().UniverseBaseUtc+Server.Read().UniverseSeconds);
    TestEqual(TEXT("Server clock respects leap day"),Leap.GetDay(),29);
    TestEqual(TEXT("Leap day month"),Leap.GetMonth(),2);
    TestFalse(TEXT("Backward wall-clock adjustment cannot rewind universe"),Server.SynchronizeClock(Gmt));
    FFractalVerseAuthority Restarted; FString Reason;
    TestTrue(TEXT("Clock anchors persist with world"),Restarted.Restore(Server.Read(),Reason));
    Restarted.SynchronizeClock(Gmt+86402);
    const auto Next=FDateTime::FromUnixTimestamp(Restarted.Read().UniverseBaseUtc+Restarted.Read().UniverseSeconds);
    TestEqual(TEXT("Offline restart carries elapsed server GMT"),Next.GetMonth(),3);
    TestEqual(TEXT("Offline leap-day rollover"),Next.GetDay(),1);
    auto* Instance=NewObject<UGameInstance>();
    auto* Runtime=NewObject<USSWRuntimeSubsystem>(Instance);
    Runtime->SetTimeCompression(20);
    TestEqual(TEXT("Travel acceleration rejected"),Runtime->GetTimeCompression(),uint32(1));
    TestFalse(TEXT("No pilot acceleration permission"),Runtime->CanAccelerateTravel());
    TestFalse(TEXT("Main game rejects engine pause"),GetMutableDefault<AStarshatterWarsGameMode>()->SetPause(nullptr));
    TestFalse(TEXT("Universe server rejects engine pause"),GetMutableDefault<AFractalVerseGameMode>()->SetPause(nullptr));
    auto* Context=NewObject<UInputMappingContext>();
    StarshatterInputBindings::MapTravelSpeed(Context,false);
    StarshatterInputBindings::MapTravelSpeed(Context,true);
    TestEqual(TEXT("Travel-speed keys are unmapped"),Context->GetMappings().Num(),0);
    return true;
}
#endif
