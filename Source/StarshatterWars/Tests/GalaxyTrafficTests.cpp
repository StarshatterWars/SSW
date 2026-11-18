#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "TrafficAI.h"
#include "EconomyRuntime.h"
#include "JsonObjectConverter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGalaxyTrafficLedgerTest,"Starshatter.Traffic.ReservationsArrivalAndLoss",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FGalaxyTrafficLedgerTest::RunTest(const FString&)
{
    FEconomyState Economy;FGalaxyTrafficState Traffic;
    const FName Owner=FEconomyRuntime::IndustryOwner(),Source(TEXT("Janus/Mine/Ore")),Destination(TEXT("Solus/Refinery/Input"));
    FEconomyLedger::CreateInventory(Economy,Source,Owner,TEXT("Janus"),100);
    FEconomyLedger::CreateInventory(Economy,Destination,Owner,TEXT("Solus"),9);
    FEconomyLedger::Deposit(Economy,Source,TEXT("Magnetite"),10,1);
    FGalaxyTrafficEndpoint From;From.Id=TEXT("Janus/Janek/Mine");From.Sector=TEXT("Janus/Janek");From.System=TEXT("Janus");
    FGalaxyTrafficEndpoint To;To.Id=TEXT("Solus/Solus/Refinery");To.Sector=TEXT("Solus/Solus");To.System=TEXT("Solus");
    const TArray<FName> Route{From.Sector,To.Sector};
    const FGuid Id=TrafficAI::Schedule(Traffic,Economy,From,To,Route,Source,Destination,TEXT("Magnetite"),8,1);
    if(!TestTrue(TEXT("Real stock creates a reserved transport job"),Id.IsValid()))return false;
    TestEqual(TEXT("Reservation does not invent or remove pickup stock"),FEconomyLedger::Quantity(Economy,Source,TEXT("Magnetite")),10.0);
    TestEqual(TEXT("Reserved stock cannot be booked twice"),FEconomyLedger::Available(Economy,Source,TEXT("Magnetite")),2.0);
    TestFalse(TEXT("Second ship cannot oversubscribe pickup stock"),TrafficAI::Schedule(Traffic,Economy,From,To,Route,Source,Destination,TEXT("Magnetite"),8,1).IsValid());
    TestFalse(TEXT("Even available pickup stock cannot overbook destination capacity"),TrafficAI::Schedule(Traffic,Economy,From,To,Route,Source,Destination,TEXT("Magnetite"),2,1).IsValid());
    auto& Job=Traffic.Journeys[0];
    TrafficAI::Advance(Job,Economy,3,true,false,10,1);
    TrafficAI::Advance(Job,Economy,100,true,false,10,1);
    TestEqual(TEXT("Visible ship cannot load before pickup arrival"),Job.Phase,EGalaxyTrafficPhase::Loading);
    TestEqual(TEXT("Pickup stock retained while approaching"),FEconomyLedger::Quantity(Economy,Source,TEXT("Magnetite")),10.0);
    TrafficAI::Advance(Job,Economy,10,true,true,10,1);
    TestEqual(TEXT("Cargo debited only on completed loading"),FEconomyLedger::Quantity(Economy,Source,TEXT("Magnetite")),2.0);
    TestEqual(TEXT("Loaded stock is physically aboard its owned hold"),FEconomyLedger::Quantity(Economy,Job.Hold,TEXT("Magnetite")),8.0);
    TrafficAI::Advance(Job,Economy,100,true,false,10,1);
    TestEqual(TEXT("Time cannot finish a visible ship's route"),Job.Phase,EGalaxyTrafficPhase::Traveling);
    TestEqual(TEXT("No destination credit while traveling"),FEconomyLedger::Quantity(Economy,Destination,TEXT("Magnetite")),0.0);
    TrafficAI::Advance(Job,Economy,1,true,true,10,1);
    TestEqual(TEXT("Actual arrival starts unloading instead of crediting instantly"),Job.Phase,EGalaxyTrafficPhase::Unloading);
    TestTrue(TEXT("Unloading completes"),TrafficAI::Advance(Job,Economy,10,true,true,10,1));
    TestEqual(TEXT("Destination receives precisely delivered stock"),FEconomyLedger::Quantity(Economy,Destination,TEXT("Magnetite")),8.0);
    TestEqual(TEXT("Destination reservation released"),Economy.Inventories.FindChecked(Destination).ReservedOutputTonnes,0.0);
    TrafficAI::Advance(Job,Economy,100,true,true,10,1);
    TestEqual(TEXT("Repeated completion does not duplicate stock"),FEconomyLedger::Quantity(Economy,Destination,TEXT("Magnetite")),8.0);
    Economy.Inventories.FindChecked(Destination).CapacityTonnes=100;
    const FGuid LostId=TrafficAI::Schedule(Traffic,Economy,From,To,Route,Source,Destination,TEXT("Magnetite"),2,1);
    TestTrue(TEXT("Remaining stock can be scheduled"),LostId.IsValid());
    auto& Lost=Traffic.Journeys.Last();TrafficAI::Advance(Lost,Economy,3,false,false,10,1);TrafficAI::Advance(Lost,Economy,3,false,false,10,1);
    TrafficAI::Lose(Lost,Economy);
    TestEqual(TEXT("Destroyed ship cannot credit destination"),FEconomyLedger::Quantity(Economy,Destination,TEXT("Magnetite")),8.0);
    TestEqual(TEXT("Lost cargo is not refunded to pickup"),FEconomyLedger::Quantity(Economy,Source,TEXT("Magnetite")),0.0);
    TestFalse(TEXT("Destroyed hold retired"),Economy.Inventories.Contains(Lost.Hold));
    TestTrue(TEXT("Traffic does not create currency"),Economy.Wallets.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGalaxyTrafficCoverageTest,"Starshatter.Traffic.SectorCoverageGuardsAndPersistence",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FGalaxyTrafficCoverageTest::RunTest(const FString&)
{
    TMap<FName,FGalaxyTrafficSector> Graph;TMap<FName,FGalaxyTrafficEndpoint> Endpoints;
    for(int32 Index=0;Index<8;++Index)
    {
        FGalaxyTrafficSector Sector;Sector.Id=FName(*FString::Printf(TEXT("System%d/Sector%d"),Index/2,Index));Sector.System=FName(*FString::Printf(TEXT("System%d"),Index/2));Sector.Region=FName(*FString::Printf(TEXT("Sector%d"),Index));Sector.bSurface=(Index%2)!=0;
        Graph.Add(Sector.Id,Sector);
        for(const TCHAR* Side:{TEXT("TransitA"),TEXT("TransitB")})
        {FGalaxyTrafficEndpoint Point;Point.Id=FName(*(Sector.Id.ToString()+TEXT("/")+Side));Point.Sector=Sector.Id;Point.System=Sector.System;Point.bSurface=Sector.bSurface;Endpoints.Add(Point.Id,Point);}
    }
    FEconomyState Economy;FGalaxyTrafficState Traffic;
    TArray<FName> Ordered;Graph.GetKeys(Ordered);Ordered.Sort([](FName A,FName B){return A.LexicalLess(B);});
    for(int32 Index=0;Index+1<Ordered.Num();++Index)Graph.FindChecked(Ordered[Index]).Neighbors.Add(Ordered[Index+1]);
    TestEqual(TEXT("Routes traverse the complete authored directed graph"),TrafficAI::Route(Graph,Ordered[0],Ordered.Last()).Num(),8);
    TestTrue(TEXT("Missing reverse links cannot fabricate a gate route"),TrafficAI::Route(Graph,Ordered.Last(),Ordered[0]).IsEmpty());
    for(int32 Step=0;Step<8;++Step){Traffic.Clock=Step+1;TrafficAI::Dispatch(Traffic,Economy,Graph,Endpoints,NAME_None,8);}
    TestEqual(TEXT("Every supplied sector including empty/surface sectors receives a scheduled role"),Traffic.SectorLastServed.Num(),8);
    TestEqual(TEXT("Scheduler respects the global journey cap"),Traffic.Journeys.Num(),8);
    TrafficAI::Dispatch(Traffic,Economy,Graph,Endpoints,NAME_None,8);
    TestEqual(TEXT("Repeated dispatch cannot exceed cap"),Traffic.Journeys.Num(),8);
    TestTrue(TEXT("Standalone authority may run traffic"),TrafficAI::Eligible(true,true,false,false));
    TestFalse(TEXT("Clients never schedule or credit stock"),TrafficAI::Eligible(false,true,false,false));
    TestFalse(TEXT("Cinematics never spawn ambient traffic"),TrafficAI::Eligible(true,true,true,false));
    TestFalse(TEXT("Offline capture cannot be polluted"),TrafficAI::Eligible(true,true,false,true));
    TestFalse(TEXT("Menus do not advance traffic"),TrafficAI::Eligible(true,false,false,false));
    TestFalse(TEXT("Station and asteroid overlap anchor rejected"),TrafficAI::SafeAnchor(FVector::ZeroVector,100,{{FVector(100,0,0),200}}));
    TestTrue(TEXT("Clear orbital anchor accepted"),TrafficAI::SafeAnchor(FVector(10000,0,0),100,{{FVector::ZeroVector,200}}));
    FString Saved;FGalaxyTrafficState Restored;
    TestTrue(TEXT("Abstract traffic state serializes"),FJsonObjectConverter::UStructToJsonObjectString(Traffic,Saved));
    TestTrue(TEXT("Abstract traffic state restores"),FJsonObjectConverter::JsonObjectStringToUStruct(Saved,&Restored));
    TestEqual(TEXT("Journey IDs survive reload"),Restored.Journeys[0].Id,Traffic.Journeys[0].Id);
    TestEqual(TEXT("Sector coverage history survives reload"),Restored.SectorLastServed.Num(),8);
    return true;
}
#endif
