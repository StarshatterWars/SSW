#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "TrafficAI.h"
#include "EconomyRuntime.h"
#include "PlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FScheduledFreightTest,"Starshatter.Economy.Events.FreightDeadlinesAndRestart",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FScheduledFreightTest::RunTest(const FString&)
{
    auto* Save=NewObject<UPlayerSaveGame>();auto& E=Save->PlayerInfo.Economy;auto& T=Save->PlayerInfo.Traffic;
    const FName Owner=FEconomyRuntime::IndustryOwner(),Source=TEXT("Janus/Mine/Ore"),Drop=TEXT("Janus/Refinery/Input"),Ore=TEXT("Magnetite");
    FEconomyLedger::CreateInventory(E,Source,Owner,TEXT("Janus"),100);FEconomyLedger::CreateInventory(E,Drop,Owner,TEXT("Janus"),100);FEconomyLedger::Deposit(E,Source,Ore,8,1);
    FGalaxyTrafficEndpoint A;A.Id=TEXT("A");A.System=TEXT("Janus");A.Sector=TEXT("Janus/A");
    FGalaxyTrafficEndpoint B;B.Id=TEXT("B");B.System=TEXT("Janus");B.Sector=TEXT("Janus/B");
    const auto Receipt=TrafficAI::Schedule(T,E,A,B,{A.Sector,B.Sector},Source,Drop,Ore,8,1);
    if(!TestTrue(TEXT("Freight reserved"),Receipt.IsValid()))return false;
    auto& J=T.Journeys[0];TrafficAI::AdvanceScheduled(J,E,1,false,false,true,10,2);
    const double Due=J.TransitionAt,Remaining=J.Remaining;
    TrafficAI::AdvanceScheduled(J,E,Due-.25,false,false,false,10,2);
    TestEqual(TEXT("No frame-driven freight countdown"),J.Remaining,Remaining);
    TrafficAI::AdvanceScheduled(J,E,Due,false,false,false,10,2);
    TestTrue(TEXT("Waiting deadline begins loading"),J.Phase==EGalaxyTrafficPhase::Loading);
    TestEqual(TEXT("No early cargo credit"),FEconomyLedger::Quantity(E,Drop,Ore),0.0);
    TrafficAI::AdvanceScheduled(J,E,J.TransitionAt,false,false,false,10,2);
    TestEqual(TEXT("Loading completion moves stock once"),FEconomyLedger::Quantity(E,J.Hold,Ore),8.0);
    const double RestartAt=J.TransitionAt;TArray<uint8> Bytes;UGameplayStatics::SaveGameToMemory(Save,Bytes);
    auto* Loaded=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));if(!TestNotNull(TEXT("Deadline save restores"),Loaded))return false;
    auto& Restored=Loaded->PlayerInfo.Traffic.Journeys[0];auto& Ledger=Loaded->PlayerInfo.Economy;
    TestEqual(TEXT("Phase deadline persists"),Restored.TransitionAt,RestartAt);
    TrafficAI::AdvanceScheduled(Restored,Ledger,RestartAt,false,false,false,10,2);
    TrafficAI::AdvanceScheduled(Restored,Ledger,Restored.TransitionAt,false,false,false,10,2);
    TestTrue(TEXT("Route completes before unloading"),Restored.Phase==EGalaxyTrafficPhase::Unloading);
    TestEqual(TEXT("Arrival does not grant stock"),FEconomyLedger::Quantity(Ledger,Drop,Ore),0.0);
    TrafficAI::AdvanceScheduled(Restored,Ledger,Restored.TransitionAt,false,false,false,10,2);
    TestEqual(TEXT("Unload deadline grants exact delivery"),FEconomyLedger::Quantity(Ledger,Drop,Ore),8.0);
    TrafficAI::AdvanceScheduled(Restored,Ledger,RestartAt+1000,false,false,true,10,2);
    TestEqual(TEXT("Duplicate wake cannot duplicate cargo"),FEconomyLedger::Quantity(Ledger,Drop,Ore),8.0);
    return true;
}
#endif
