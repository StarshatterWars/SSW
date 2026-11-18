#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AgentJournalTransaction.h"

namespace
{
    const FName Pilot(TEXT("TransactionPilot")), Issuer(TEXT("Instructor"));
    bool OfferLesson(AgentAI& Agent,int32 Id=101)
    { TArray<int32> Eligible={Id}; return Agent.Request(Pilot,Eligible,Issuer)!=nullptr; }
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentRequestSaveFailureTest,"Starshatter.Agents.Transaction.RequestFailure",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAgentRequestSaveFailureTest::RunTest(const FString&)
{
    AgentAI Agent; FS_PlayerGameInfo Player; Player.AgentJournalJson=TEXT("previous");
    const auto Result=AgentJournalTransaction::Commit(Agent,Player,[&](){return OfferLesson(Agent);},[&]()
    { Player.AgentJournalJson=Agent.SaveJournal(); return false; });
    TestTrue(TEXT("Persistence failure distinguished"),Result==AgentJournalTransaction::EResult::SaveFailed);
    TestNull(TEXT("Unsaved offer removed"),Agent.FindOffer(Pilot,Issuer));
    TestEqual(TEXT("Player serialization side effect restored"),Player.AgentJournalJson,FString(TEXT("previous")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentAcceptSaveFailureTest,"Starshatter.Agents.Transaction.AcceptFailure",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAgentAcceptSaveFailureTest::RunTest(const FString&)
{
    AgentAI Agent; FS_PlayerGameInfo Player; Player.InsuranceCredits=500;
    OfferLesson(Agent);
    const auto Result=AgentJournalTransaction::Commit(Agent,Player,[&](){return Agent.Accept(Pilot,101);},[&]()
    { Player.InsuranceCredits=0; Player.AgentJournalJson=Agent.SaveJournal(); return false; });
    TestTrue(TEXT("Failed accept is not acknowledged"),Result==AgentJournalTransaction::EResult::SaveFailed);
    TestTrue(TEXT("Original offer remains"),Agent.FindOffer(Pilot,Issuer)!=nullptr);
    TestTrue(TEXT("Queue unchanged"),Agent.GetQueue(Pilot).IsEmpty());
    TestEqual(TEXT("Full relevant player state rolled back"),Player.InsuranceCredits,int64(500));
    TestTrue(TEXT("Retry can commit once"),AgentJournalTransaction::Commit(Agent,Player,[&](){return Agent.Accept(Pilot,101);},[](){return true;})==AgentJournalTransaction::EResult::Committed);
    TestEqual(TEXT("Exactly one accepted queue entry"),Agent.GetQueue(Pilot).Num(),1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentDeclineSaveFailureTest,"Starshatter.Agents.Transaction.DeclineFailure",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAgentDeclineSaveFailureTest::RunTest(const FString&)
{
    AgentAI Agent; FS_PlayerGameInfo Player; OfferLesson(Agent); Agent.Accept(Pilot,101);
    TestTrue(TEXT("Decline failure reported"),AgentJournalTransaction::Commit(Agent,Player,[&](){return Agent.Decline(Pilot,101);},[](){return false;})==AgentJournalTransaction::EResult::SaveFailed);
    TestEqual(TEXT("Accepted assignment restored"),Agent.GetQueue(Pilot).Num(),1);
    TestTrue(TEXT("Decline retry succeeds"),AgentJournalTransaction::Commit(Agent,Player,[&](){return Agent.Decline(Pilot,101);},[](){return true;})==AgentJournalTransaction::EResult::Committed);
    TestTrue(TEXT("Committed decline removes queue entry"),Agent.GetQueue(Pilot).IsEmpty());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentExactStateCommitTest,"Starshatter.Agents.Transaction.ExactStateAndCommit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAgentExactStateCommitTest::RunTest(const FString&)
{
    AgentAI Agent; FS_PlayerGameInfo Player; OfferLesson(Agent); Agent.Accept(Pilot,101); Agent.Start(Pilot,101);
    AgentJournalTransaction::Commit(Agent,Player,[&](){Agent.Interrupt(Pilot,101);return true;},[](){return false;});
    TestTrue(TEXT("Rollback keeps exact InProgress state instead of load-time Accepted migration"),Agent.GetQueue(Pilot)[0].State==AgentAI::EState::InProgress);
    AgentJournalTransaction::Commit(Agent,Player,[&](){return Agent.Resolve(Pilot,101,true);},[](){return false;});
    TestFalse(TEXT("Failed save cannot retain completion/reward eligibility"),Agent.IsCompleted(Pilot,101));
    TestTrue(TEXT("Running assignment restored"),Agent.GetQueue(Pilot)[0].State==AgentAI::EState::InProgress);
    int32 Saves=0;
    TestTrue(TEXT("Completion commits"),AgentJournalTransaction::Commit(Agent,Player,[&](){return Agent.Resolve(Pilot,101,true);},[&](){++Saves;return true;})==AgentJournalTransaction::EResult::Committed);
    TestTrue(TEXT("Completion retained"),Agent.IsCompleted(Pilot,101));
    TestEqual(TEXT("Exactly one persistence boundary"),Saves,1);
    TestTrue(TEXT("Duplicate rejected"),AgentJournalTransaction::Commit(Agent,Player,[&](){return Agent.Resolve(Pilot,101,true);},[&](){++Saves;return true;})==AgentJournalTransaction::EResult::Rejected);
    TestEqual(TEXT("Rejected command does not save"),Saves,1);
    return true;
}
#endif
