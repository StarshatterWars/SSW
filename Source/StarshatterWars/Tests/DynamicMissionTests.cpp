#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "DynamicMissionAI.h"
#include "ShipDesignRegistry.h"
#include "Kismet/GameplayStatics.h"
#include "LocalGameStateSave.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDynamicMissionOfferTest,"Starshatter.Agents.IndependentJobsHangarAndPersistence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDynamicMissionOfferTest::RunTest(const FString&)
{
 const auto Old=ShipDesignRegistry::GetAll();ON_SCOPE_EXIT{ShipDesignRegistry::Clear();for(const auto& Pair:Old)ShipDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);};
 FShipDesign Fighter;Fighter.ShipName=TEXT("Falcon");Fighter.ShipType=2;Fighter.Category=EShipCategory::Fighter;Fighter.ShipEmpire=EShipEmpire::All;Fighter.BaseCost=100;Fighter.CargoCapacity.Tonnes=1;ShipDesignRegistry::RegisterDesign(FName(TEXT("Falcon")),Fighter);
 FShipDesign Miner=Fighter;Miner.ShipName=TEXT("Prospector");Miner.ShipType=8;Miner.CargoCapacity.Tonnes=100;ShipDesignRegistry::RegisterDesign(FName(TEXT("Prospector")),Miner);
 auto* Save=NewObject<ULocalPilotState>();auto& P=Save->Pilot;P.Rank=1;P.Campaign=0;P.Empire=int32(EEMPIRE_NAME::Terellian);
 FDynamicAgentContext A;A.Id=TEXT("IndependentFixture");A.Name=TEXT("Agent");A.System=TEXT("Janus");A.Region=TEXT("Janek");A.Station=TEXT("Janus Transfer Station");A.Pools={TEXT("Security"),TEXT("Civil"),TEXT("Industry"),TEXT("Exploration")};A.Level=5;FText Reason;
 TestFalse(TEXT("No hangar craft means no independent job"),DynamicMissionAI::Generate(P,A,100000,Reason));TestTrue(TEXT("Missing craft is explained"),!Reason.IsEmpty());
 auto& H=P.HangarShips.AddDefaulted_GetRef();H.HullId=FGuid::NewGuid();H.Design=TEXT("Falcon");P.SelectedHangarHull=H.HullId;
 TestTrue(TEXT("Commissioned pilot gets work without a campaign"),DynamicMissionAI::Generate(P,A,100000,Reason));auto First=DynamicMissionAI::Offers(P,A.Id,100000);if(!TestEqual(TEXT("One stable offer per agent request"),First.Num(),1))return false;
 const int32 Id=First[0];TestTrue(TEXT("Reserved dynamic identity"),DynamicMissionAI::IsDynamic(Id));TestTrue(TEXT("Owned craft qualifies"),DynamicMissionAI::CanUse(P,Id,Reason));TestTrue(TEXT("Payout authored in JSON"),DynamicMissionAI::Reward(P,Id)>0);
 TestTrue(TEXT("Repeated request retains same outstanding job"),DynamicMissionAI::Generate(P,A,100001,Reason));TestEqual(TEXT("No duplicate generated job"),DynamicMissionAI::Offers(P,A.Id,100001)[0],Id);
 TArray<uint8> Bytes;TestTrue(TEXT("Offers serialize with player"),UGameplayStatics::SaveGameToMemory(Save,Bytes));auto* Reload=Cast<ULocalPilotState>(UGameplayStatics::LoadGameFromMemory(Bytes));if(!TestNotNull(TEXT("Offers reload"),Reload))return false;TestEqual(TEXT("Offer ID survives reload"),DynamicMissionAI::Offers(Reload->Pilot,A.Id,100001)[0],Id);
 H.bLost=true;TestFalse(TEXT("Destroyed hull invalidates acceptance"),DynamicMissionAI::CanUse(P,Id,Reason));H.bLost=false;
 P.Rank=0;TestFalse(TEXT("Saved job cannot bypass a reduced rank"),DynamicMissionAI::CanUse(P,Id,Reason));A.Id=TEXT("CadetFixture");TestFalse(TEXT("Independent work cannot bypass training rank gate"),DynamicMissionAI::Generate(P,A,100002,Reason));
 P.Rank=1;auto& M=P.HangarShips.AddDefaulted_GetRef();M.HullId=FGuid::NewGuid();M.Design=TEXT("Prospector");P.SelectedHangarHull=M.HullId;TSet<int32> Ids;
 for(int32 I=0;I<64;++I){A.Id=FName(*FString::Printf(TEXT("IndependentAgent%d"),I));if(DynamicMissionAI::Generate(P,A,200000+I,Reason)){const auto Offers=DynamicMissionAI::Offers(P,A.Id,200000+I);if(Offers.Num()){TestFalse(TEXT("Distinct jobs cannot reuse a payout identity"),Ids.Contains(Offers[0]));Ids.Add(Offers[0]);}}}
 TestTrue(TEXT("Jobs spread across distinct agents"),Ids.Num()>40);return true;
}

#include "AgentAI.h"
#include "Sim.h"
#include "Ship.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "OrbitalRegion.h"
#include "Mission.h"
#include "DataLoader.h"
#include "Game.h"
#include "TransactionAI.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndependentSurveyFlowTest,"Starshatter.Agents.IndependentSurveyAcceptRestartComplete",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FIndependentSurveyFlowTest::RunTest(const FString&)
{
 const auto Old=ShipDesignRegistry::GetAll();ON_SCOPE_EXIT{ShipDesignRegistry::Clear();for(const auto& Pair:Old)ShipDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);};
 FShipDesign D;D.ShipName=TEXT("Falcon");D.ShipType=2;D.Category=EShipCategory::Fighter;D.ShipEmpire=EShipEmpire::All;D.BaseCost=100;D.CargoCapacity.Tonnes=1;D.Integrity=1000;D.Mass=20;D.Scale=1;ShipDesignRegistry::RegisterDesign(FName(TEXT("Falcon")),D);
 const bool OwnLoader=!DataLoader::GetLoader();if(OwnLoader)DataLoader::Initialize();ON_SCOPE_EXIT{if(OwnLoader)DataLoader::Close();};TUniquePtr<Sim> Owned;
 if(!Sim::GetSim()){AddExpectedError(TEXT("ERROR: could notxparse 'Explosions.def'"),EAutomationExpectedErrorFlags::Contains,1);Owned=MakeUnique<Sim>(nullptr);}
 auto& S=*Sim::GetSim();auto* Previous=S.mission;auto* OldRegion=S.active_region;auto* OldSystem=S.star_system;
 Mission Free(Mission::FreeFlightId);Free.SetName("Independent fixture");StarSystem System("Independent Janus",FVector::ZeroVector);OrbitalRegion Orbit(&System,"Independent Janek",0,1,1);auto* Region=new SimRegion(&S,&Orbit);S.regions.append(Region);S.mission=&Free;S.active_region=Region;S.star_system=&System;
 ON_SCOPE_EXIT{S.regions.remove(Region);S.mission=Previous;S.active_region=OldRegion;S.star_system=OldSystem;delete Region;};
 auto* Craft=S.CreateShip("Independent pilot","",ShipDesignRegistry::FindLegacy(FName(TEXT("Falcon"))),"Independent Janek",FVector::ZeroVector,1);if(!TestNotNull(TEXT("Owned live fighter"),Craft))return false;Region->SetPlayerShip(Craft);Craft->SetFlightPhase(EOPSMode::ACTIVE);
 auto* Save=NewObject<ULocalPilotState>();auto& P=Save->Pilot;P.Rank=1;P.Campaign=0;auto& H=P.HangarShips.AddDefaulted_GetRef();H.HullId=FGuid::NewGuid();H.Design=TEXT("Falcon");P.SelectedHangarHull=H.HullId;P.Economy.Wallets.Add(FEconomyLedger::Pilot(),50000);
 FDynamicAgentContext Agent;Agent.Id=TEXT("IndependentSurveyAgent");Agent.System=TEXT("Independent Janus");Agent.Region=TEXT("Independent Janek");Agent.Station=TEXT("Orion");Agent.Pools={TEXT("Exploration")};Agent.Level=1;FText Reason;
 if(!TestTrue(TEXT("Independent survey generated with no campaign"),DynamicMissionAI::Generate(P,Agent,FDateTime::UtcNow().ToUnixTimestamp(),Reason)))return false;const auto Offers=DynamicMissionAI::Offers(P,Agent.Id,FDateTime::UtcNow().ToUnixTimestamp());if(!TestEqual(TEXT("Survey offer"),Offers.Num(),1))return false;const int32 Id=Offers[0];const FName Pilot(TEXT("Independent pilot"));AgentAI Journal;
 TestNotNull(TEXT("Agent presents job"),Journal.Request(Pilot,Offers,Agent.Id));TestTrue(TEXT("Accept"),Journal.Accept(Pilot,Id));if(!TestTrue(TEXT("Start uses owned craft without loading a campaign"),DynamicMissionAI::Start(S,P,Journal,Pilot,Id,Reason)))return false;
 TestFalse(TEXT("Repeated Start cannot reset running objectives"),DynamicMissionAI::Start(S,P,Journal,Pilot,Id,Reason));
 TestTrue(TEXT("Active job remains visible after original offer expiry"),DynamicMissionAI::Offers(P,Agent.Id,FDateTime::UtcNow().ToUnixTimestamp()+86400).Contains(Id));
 const FString BeforeRefresh=P.DynamicAssignmentsJson;
 TestTrue(TEXT("Expired original offer cannot generate a duplicate while job runs"),DynamicMissionAI::Generate(P,Agent,FDateTime::UtcNow().ToUnixTimestamp()+86400,Reason));
 TestEqual(TEXT("Refresh preserves active objectives exactly"),P.DynamicAssignmentsJson,BeforeRefresh);
 const FVector Origin=Craft->GetLocation();DynamicMissionAI::Advance(S,P,Journal,Pilot,1);
 // Reload the job and journal after the first point is provisioned.
 P.AgentJournalJson=Journal.SaveJournal();TArray<uint8> Bytes;TestTrue(TEXT("Active independent job saves"),UGameplayStatics::SaveGameToMemory(Save,Bytes));auto* Reload=Cast<ULocalPilotState>(UGameplayStatics::LoadGameFromMemory(Bytes));if(!TestNotNull(TEXT("Job reload"),Reload))return false;P=Reload->Pilot;TestTrue(TEXT("Journal reload"),Journal.RestoreJournal(P.AgentJournalJson));
 FS_CampaignMission Preview;if(!TestTrue(TEXT("Generated objective type is available"),DynamicMissionAI::Preview(P,Id,Preview)))return false;const int32 Count=Preview.MissionType==EMissionType::RECON?4:3;AddInfo(Preview.MissionName);
 for(int32 I=0;I<Count;++I){const double Angle=I*2*PI/Count;Craft->MoveTo(Origin+FVector(FMath::Cos(Angle)*18000,2000+I*750,FMath::Sin(Angle)*18000));DynamicMissionAI::Advance(S,P,Journal,Pilot,1);}
 TestTrue(TEXT("Actual point visits complete job after restart"),Journal.IsCompleted(Pilot,Id));const int64 Paid=TransactionAI::Balance(P.Economy);TestTrue(TEXT("Base and time bonus paid"),Paid>50000);DynamicMissionAI::Advance(S,P,Journal,Pilot,100);TestEqual(TEXT("Repeated tick cannot pay twice"),TransactionAI::Balance(P.Economy),Paid);TestEqual(TEXT("Campaign remains unassigned"),P.Campaign,0);return true;
}
#endif
