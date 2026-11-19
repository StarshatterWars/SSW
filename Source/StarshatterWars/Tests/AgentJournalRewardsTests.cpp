#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AgentAI.h"
#include "PlayerCharacter.h"
#include "MissionArcRules.h"
#include "MissionRewardRules.h"
#include "StationServicesAI.h"
#include "PlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "TransactionAI.h"
#include "DebriefProgressSnapshot.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentJournalDatesTest,"Starshatter.Agents.JournalDatesAndTrainingExpiry",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAgentJournalDatesTest::RunTest(const FString&)
{
    AgentAI A;const FName Pilot(TEXT("JournalTest")),Issuer(TEXT("Instructor"));const int32 Lessons[]={101,102};
    int32 Changes=0;auto Handle=AgentAI::OnJournalChanged().AddLambda([&](){++Changes;});
    const auto* Offer=A.Request(Pilot,Lessons,Issuer);if(!TestNotNull(TEXT("Training offer"),Offer)){AgentAI::OnJournalChanged().Remove(Handle);return false;}
    TestEqual(TEXT("Training never expires"),Offer->ExpiresUtc,0LL);
    FString Old=A.SaveJournal();TArray<TSharedPtr<FJsonValue>> LegacyEntries;
    TestTrue(TEXT("Read legacy expiry fixture"),FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Old),LegacyEntries));
    for(const auto& Entry:LegacyEntries)Entry->AsObject()->SetNumberField(TEXT("Expiry"),1);
    Old.Reset();FJsonSerializer::Serialize(LegacyEntries,TJsonWriterFactory<>::Create(&Old));
    AgentAI B;TestTrue(TEXT("Legacy journal loads"),B.RestoreJournal(Old));
    TestTrue(TEXT("Previously expired lesson accepts"),B.Accept(Pilot,101));
    TestTrue(TEXT("Accepted time captured"),B.GetQueue(Pilot)[0].AcceptedUtc>0);
    B.Start(Pilot,101);B.Resolve(Pilot,101,true);
    TestTrue(TEXT("Completion date retained"),B.GetHistory(Pilot)[0].CompletedUtc>=B.GetHistory(Pilot)[0].AcceptedUtc);
    B.Request(Pilot,Lessons,Issuer);B.Accept(Pilot,102);B.Decline(Pilot,102);
    const auto& Abandoned=B.GetHistory(Pilot).Last();
    TestEqual(TEXT("Abandon keeps issuing agent"),Abandoned.Issuer,Issuer);
    TestTrue(TEXT("Abandon date retained"),Abandoned.AbandonedUtc>=Abandoned.AcceptedUtc);
    AgentAI C;TestTrue(TEXT("History saved and restored"),C.RestoreJournal(B.SaveJournal()));
    TestEqual(TEXT("Both outcomes retained"),C.GetHistory(Pilot).Num(),2);
    TestEqual(TEXT("Original acceptance time survives"),C.GetHistory(Pilot)[0].AcceptedUtc,B.GetHistory(Pilot)[0].AcceptedUtc);
    TestTrue(TEXT("Abandoned branch choices survive reload"),C.HasChosen(Pilot,102));
    TestTrue(TEXT("Change event reached consumers"),Changes>=6);AgentAI::OnJournalChanged().Remove(Handle);
    const auto Before=C.SaveJournal();TestFalse(TEXT("Corrupt journal rejected"),C.RestoreJournal(TEXT("[]bad")));TestEqual(TEXT("Corruption leaves previous state intact"),C.SaveJournal(),Before);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBranchingAgentArcTest,"Starshatter.Agents.BranchingChapters",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBranchingAgentArcTest::RunTest(const FString&)
{
    const FString Json=TEXT(R"([{"Id":"Branch","Title":"Choice","Empire":"Terellian","Description":"Test","Steps":[
    {"MissionId":910900,"Title":"Start","Issuer":"One","System":"Janus","Region":"Janek","Brief":"","Chapter":"Opening","NextMissionIds":[910901,910902]},
    {"MissionId":910901,"Title":"Escort","Issuer":"Two","AgentId":"JanekTraffic","System":"Janus","Region":"Janek","Brief":"","ChoiceGroup":"Response","NextMissionIds":[910903]},
    {"MissionId":910902,"Title":"Intercept","Issuer":"Three","System":"Janus","Region":"Janek","Brief":"","ChoiceGroup":"Response","NextMissionIds":[910903]},
    {"MissionId":910903,"Title":"Final","Issuer":"Four","System":"Janus","Region":"Janek","Brief":"","Chapter":"Finale","NextMissionIds":[]}]}])");
    FMissionArcCatalog C;FText Error;if(!TestTrue(TEXT("Branch graph loads"),C.LoadJson(Json,Error)))return false;
    TSet<int32> Done={910900},Choices;auto Completed=[&](int32 Id){return Done.Contains(Id);};auto Chosen=[&](int32 Id){return Choices.Contains(Id);};
    TestEqual(TEXT("Two branch handoffs available"),C.Evaluate(910900,Completed,Chosen).Available.Num(),2);
    Choices.Add(910901);TestFalse(TEXT("Accepting branch locks sibling"),C.Evaluate(910902,Completed,Chosen).bEligible);
    TestFalse(TEXT("Failure cannot unlock join"),C.Evaluate(910903,Completed,Chosen).bEligible);
    Done.Add(910901);TestTrue(TEXT("Completed chosen branch unlocks join"),C.Evaluate(910903,Completed,Chosen).bEligible);
    Done.Add(910903);TestTrue(TEXT("Chosen terminal completes arc"),C.Evaluate(910903,Completed,Chosen).bComplete);
    FString Bad=Json;Bad.ReplaceInline(TEXT("\"NextMissionIds\":[]"),TEXT("\"NextMissionIds\":[910900]"));
    TestFalse(TEXT("Cycle rejected"),C.LoadJson(Bad,Error));TestEqual(TEXT("Bad import does not erase graph"),C.GetArcs().Num(),1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionCreditsTest,"Starshatter.Missions.RewardsAndPersistence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMissionCreditsTest::RunTest(const FString&)
{
    const auto& C=FMissionRewardCatalog::Get();FEconomyState State;State.Wallets.Add(FEconomyLedger::Pilot(),50000);
    auto Q=C.Quote(1,101,true,0,400);
    TestFalse(TEXT("Failed mission cannot pay"),C.Award(State,1,101,Q,false,true,false));
    TestFalse(TEXT("Test replay cannot pay"),C.Award(State,1,101,Q,true,true,true));
    TestTrue(TEXT("T1 credits base and recovery"),C.Award(State,1,101,Q,true,true,false));
    const int64 First=TransactionAI::Balance(State);TestFalse(TEXT("Repeated receipt cannot pay twice"),C.Award(State,1,101,Q,true,true,false));TestEqual(TEXT("Balance unchanged"),TransactionAI::Balance(State),First);
    TestTrue(TEXT("T2 reward settles"),C.Award(State,1,102,C.Quote(1,102,true,0,100),true,true,false));
    FString Ships;FFileHelper::LoadFileToString(Ships,*(FPaths::ProjectContentDir()/TEXT("GameData/Ships/ships.json")));
    TArray<TSharedPtr<FJsonValue>> Designs;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Ships),Designs);double Cost=0;
    for(const auto& D:Designs){auto O=D->AsObject();FString Name;if(O->TryGetStringField(TEXT("ShipName"),Name) && Name==TEXT("Falcon"))O->TryGetNumberField(TEXT("BaseCost"),Cost);}
    TestTrue(TEXT("First two lessons fund T3 Falcon at actual catalog cost"),Cost>0 && TransactionAI::Balance(State)>=Cost);
    const auto Fast=C.Quote(1,104,true,2,600),Slow=C.Quote(1,104,false,0,1800);
    TestTrue(TEXT("Selected strike rewards recovery secondary objectives and time"),Fast.Recovery>0 && Fast.Objectives>0 && Fast.Time>0 && Fast.Total()>Slow.Total());
    TestTrue(TEXT("Any authored or custom mission has base reward"),C.Quote(5,99999,false,0,0).Base>0);
    auto* Save=NewObject<UPlayerSaveGame>();Save->PlayerInfo.Economy=State;TArray<uint8> Bytes;
    TestTrue(TEXT("Account serializes"),UGameplayStatics::SaveGameToMemory(Save,Bytes));auto* Loaded=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if(TestNotNull(TEXT("Account reloads"),Loaded)){TestEqual(TEXT("Balance survives restart"),TransactionAI::Balance(Loaded->PlayerInfo.Economy),TransactionAI::Balance(State));TestEqual(TEXT("Reward receipts survive"),Loaded->PlayerInfo.Economy.Journal.Num(),2);}
    FS_PlayerGameInfo Player;Player.Economy=State;AgentAI Agent;FDebriefProgressSnapshot Before(Agent,Player,nullptr,nullptr);
    C.Award(Player.Economy,1,103,C.Quote(1,103,true,0,0),true,true,false);Before.Restore(Agent,Player,nullptr);
    TestEqual(TEXT("Save failure rolls wallet back with mission completion"),TransactionAI::Balance(Player.Economy),TransactionAI::Balance(State));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHostileStationServiceTest,"Starshatter.StationServices.FactionPolicy",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHostileStationServiceTest::RunTest(const FString&)
{
    FStationDockContext C;C.Design=TEXT("Orion");C.Facility=TEXT("Archon");C.ActualDock=C.Facility;C.bDocked=true;FText Why;
    TestTrue(TEXT("Friendly carrier repairs"),StationServicesAI::CanUse(C,ESSWStationService::Repair,Why));
    C.bFactionAllowed=false;for(const auto& S:StationServicesAI::Descriptors())TestFalse(TEXT("Hostile relation denies all service actions"),StationServicesAI::CanUse(C,S.Service,Why));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrainingHullGateTest,"Starshatter.Agents.TrainingHullRankGates",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTrainingHullGateTest::RunTest(const FString&)
{
    TestEqual(TEXT("Falcon allowed for cadets"),PlayerCharacter::HullRankRequired(2,TEXT("Falcon")),0);
    TestEqual(TEXT("Stormhawk allowed for cadets"),PlayerCharacter::HullRankRequired(2,TEXT("Stormhawk")),0);
    TestTrue(TEXT("Other fighters require Ensign"),PlayerCharacter::HullRankRequired(2,TEXT("Eagle"))>=1);
    TestTrue(TEXT("Courier cannot bypass Ensign via default class"),PlayerCharacter::HullRankRequired(0,TEXT("Courier"))>=1);
    TestTrue(TEXT("Mining shuttle requires Ensign"),PlayerCharacter::HullRankRequired(8,TEXT("Prospector"))>=1);return true;
}
#endif
