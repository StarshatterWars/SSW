#include "AgentAI.h"
#include "PlayerCharacter.h"

FName AgentAI::PilotKey(const PlayerCharacter& Pilot)
{
    return FName(*FString::Printf(TEXT("%d:%s"), static_cast<int32>(Pilot.GetEmpire()), *Pilot.GetName()));
}

TConstArrayView<AgentAI::FAssignment> AgentAI::GetQueue(FName Pilot) const
{
    if (const auto* Queue = Queues.Find(Pilot)) return *Queue;
    return {};
}

const AgentAI::FAssignment* AgentAI::Find(FName Pilot) const
{
    const auto Queue = GetQueue(Pilot);
    if (!Queue.IsEmpty()) return &Queue[0];
    if (const auto* Offer = FindOffer(Pilot)) return Offer;
    return LastOutcome.Find(Pilot);
}

const AgentAI::FAssignment* AgentAI::FindOffer(FName Pilot, FName Issuer) const
{
    const auto* PilotOffers = Offers.Find(Pilot);
    return PilotOffers ? PilotOffers->Find(Issuer) : nullptr;
}

const AgentAI::FAssignment* AgentAI::Request(FName Pilot, TConstArrayView<int32> EligibleMissions, FName Issuer)
{
    if (Pilot.IsNone()) return nullptr;
    auto& PilotOffers = Offers.FindOrAdd(Pilot);
    if (const auto* Existing = PilotOffers.Find(Issuer); Existing && EligibleMissions.Contains(Existing->MissionId))
        return Existing;
    PilotOffers.Remove(Issuer);

    const auto* Outcome = LastOutcome.Find(Pilot);
    const auto Queue = GetQueue(Pilot);
    int32 Next = INDEX_NONE;
    for (const int32 Id : EligibleMissions)
    {
        if (Id <= 0 || (Next != INDEX_NONE && Id >= Next)) continue;
        if (Outcome && Outcome->MissionId == Id &&
            (Outcome->State == EState::Declined || Outcome->State == EState::Completed)) continue;
        if (Queue.ContainsByPredicate([Id](const FAssignment& Item) { return Item.MissionId == Id; })) continue;
        bool bAlreadyOffered = false;
        for (const auto& Offer : PilotOffers)
            if (Offer.Value.MissionId == Id) { bAlreadyOffered = true; break; }
        if (bAlreadyOffered) continue;
        Next = Id;
    }
    if (Next == INDEX_NONE) return nullptr;
    return &PilotOffers.Add(Issuer, FAssignment{Next, EState::Offered, Issuer});
}

bool AgentAI::Transition(FName Pilot, int32 MissionId, EState From, EState To)
{
    auto* Queue = Queues.Find(Pilot);
    if (!Queue || Queue->IsEmpty() || (*Queue)[0].MissionId != MissionId || (*Queue)[0].State != From) return false;
    (*Queue)[0].State = To;
    return true;
}

bool AgentAI::Accept(FName Pilot, int32 MissionId)
{
    auto* PilotOffers = Offers.Find(Pilot);
    if (!PilotOffers) return false;
    for (auto It = PilotOffers->CreateIterator(); It; ++It)
        if (It.Value().MissionId == MissionId)
        {
            Queues.FindOrAdd(Pilot).Add(FAssignment{MissionId, EState::Accepted, It.Key()});
            It.RemoveCurrent();
            return true;
        }
    return false;
}

bool AgentAI::Decline(FName Pilot, int32 MissionId)
{
    if (auto* PilotOffers = Offers.Find(Pilot))
        for (auto It = PilotOffers->CreateIterator(); It; ++It)
            if (It.Value().MissionId == MissionId)
            {
                LastOutcome.Add(Pilot, FAssignment{MissionId, EState::Declined, It.Key()});
                It.RemoveCurrent();
                return true;
            }
    auto* Queue = Queues.Find(Pilot);
    if (!Queue) return false;
    const int32 Removed = Queue->RemoveAll([MissionId](const FAssignment& Item)
    {
        return Item.MissionId == MissionId && Item.State == EState::Accepted;
    });
    if (Removed) LastOutcome.Add(Pilot, FAssignment{MissionId, EState::Declined});
    return Removed > 0;
}

bool AgentAI::Start(FName Pilot, int32 MissionId)
{
    return Transition(Pilot, MissionId, EState::Accepted, EState::InProgress);
}

bool AgentAI::Resolve(FName Pilot, int32 MissionId, bool bSucceeded)
{
    const auto Outcome = bSucceeded ? EState::Completed : EState::Failed;
    if (!Transition(Pilot, MissionId, EState::InProgress, Outcome)) return false;
    LastOutcome.Add(Pilot, Queues.FindChecked(Pilot)[0]);
    Queues.FindChecked(Pilot).RemoveAt(0);
    return true;
}

void AgentAI::Interrupt(FName Pilot, int32 MissionId)
{
    Transition(Pilot, MissionId, EState::InProgress, EState::Accepted);
}
