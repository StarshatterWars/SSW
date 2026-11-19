#pragma once

#include "CoreMinimal.h"

class PlayerCharacter;

// Session-scoped offers. Mission content, rank and completion records remain
// owned by the campaign and player profile.
DECLARE_MULTICAST_DELEGATE(FOnAgentJournalChanged);

class AgentAI
{
public:
    static FOnAgentJournalChanged& OnJournalChanged();
    static bool IsTrainingMission(int32 Id) { return Id>=101 && Id<=107; }
    FString SaveJournal() const;
    bool RestoreJournal(const FString& Json);
    static FName PilotKey(const PlayerCharacter& Pilot);

    enum class EState : uint8 { None, Offered, Accepted, InProgress, Completed, Failed, Declined };

    struct FAssignment
    {
        int32 MissionId = INDEX_NONE;
        EState State = EState::None;
        FName Issuer;
        int64 ExpiresUtc = 0;
        bool bTestReplay = false;
        int64 AcceptedUtc = 0, CompletedUtc = 0, AbandonedUtc = 0;
    };

    TConstArrayView<FAssignment> GetHistory(FName Pilot) const;
    const FAssignment* LatestOutcome(FName Pilot) const { return LastOutcome.Find(Pilot); }
    bool HasChosen(FName Pilot,int32 MissionId) const;
    const FAssignment* Find(FName Pilot) const;
    const FAssignment* FindOffer(FName Pilot, FName Issuer = NAME_None) const;
    TConstArrayView<FAssignment> GetQueue(FName Pilot) const;
    const FAssignment* Request(FName Pilot, TConstArrayView<int32> EligibleMissions, FName Issuer = NAME_None, bool bRetryDeclined = false, TFunction<bool(int32)> HistoricalCompletion = {});
    bool IsCompleted(FName Pilot, int32 MissionId) const { const auto* History = Completed.Find(Pilot); return History && History->Contains(MissionId); }
    FText ArcRequirement(FName Pilot, int32 MissionId, TFunction<bool(int32)> HistoricalCompletion = {}) const;
    FText ArcStatus(FName Pilot, int32 MissionId, TFunction<bool(int32)> HistoricalCompletion = {}) const;
    bool Accept(FName Pilot, int32 MissionId, TFunction<bool(int32)> HistoricalCompletion = {});
    bool QueueTestReplay(FName Pilot,int32 MissionId,FName Issuer);
    bool Decline(FName Pilot, int32 MissionId);
    bool Start(FName Pilot, int32 MissionId);
    bool Resolve(FName Pilot, int32 MissionId, bool bSucceeded);
    void Interrupt(FName Pilot, int32 MissionId);

private:
    bool Transition(FName Pilot, int32 MissionId, EState From, EState To);
    void RecordOutcome(FName Pilot,const FAssignment& Item);
    TMap<FName,TArray<FAssignment>> JournalHistory;
    TMap<FName,TSet<int32>> Choices;
    TMap<FName, TMap<FName, FAssignment>> Offers;
    TMap<FName, TArray<FAssignment>> Queues;
    TMap<FName, FAssignment> LastOutcome;
    TMap<FName, TSet<int32>> Completed;
};
