#pragma once

#include "CoreMinimal.h"

class PlayerCharacter;

// Session-scoped offers. Mission content, rank and completion records remain
// owned by the campaign and player profile.
class AgentAI
{
public:
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
    };

    const FAssignment* Find(FName Pilot) const;
    const FAssignment* FindOffer(FName Pilot, FName Issuer = NAME_None) const;
    TConstArrayView<FAssignment> GetQueue(FName Pilot) const;
    const FAssignment* Request(FName Pilot, TConstArrayView<int32> EligibleMissions, FName Issuer = NAME_None);
    bool Accept(FName Pilot, int32 MissionId);
    bool Decline(FName Pilot, int32 MissionId);
    bool Start(FName Pilot, int32 MissionId);
    bool Resolve(FName Pilot, int32 MissionId, bool bSucceeded);
    void Interrupt(FName Pilot, int32 MissionId);

private:
    bool Transition(FName Pilot, int32 MissionId, EState From, EState To);
    TMap<FName, TMap<FName, FAssignment>> Offers;
    TMap<FName, TArray<FAssignment>> Queues;
    TMap<FName, FAssignment> LastOutcome;
    TMap<FName, TSet<int32>> Completed;
};
