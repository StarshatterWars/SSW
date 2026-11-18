#include "AgentAI.h"
#include "MissionArcRules.h"
#include "PlayerCharacter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

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

const AgentAI::FAssignment* AgentAI::Request(FName Pilot, TConstArrayView<int32> EligibleMissions, FName Issuer, bool bRetryDeclined, TFunction<bool(int32)> HistoricalCompletion)
{
    if (Pilot.IsNone()) return nullptr;
    auto& PilotOffers = Offers.FindOrAdd(Pilot);
    if (const auto* Existing = PilotOffers.Find(Issuer); Existing && !IsCompleted(Pilot,Existing->MissionId) && !(HistoricalCompletion && HistoricalCompletion(Existing->MissionId)) && ArcRequirement(Pilot,Existing->MissionId,HistoricalCompletion).IsEmpty() &&
        (Existing->ExpiresUtc == 0 || Existing->ExpiresUtc > FDateTime::UtcNow().ToUnixTimestamp()) && EligibleMissions.Contains(Existing->MissionId))
        return Existing;
    PilotOffers.Remove(Issuer);

    const auto* Outcome = LastOutcome.Find(Pilot);
    const auto Queue = GetQueue(Pilot);
    int32 Next = INDEX_NONE;
    for (const int32 Id : EligibleMissions)
    {
        if (Id <= 0 || Completed.FindRef(Pilot).Contains(Id) || (HistoricalCompletion && HistoricalCompletion(Id)) || !ArcRequirement(Pilot,Id,HistoricalCompletion).IsEmpty() || (Next != INDEX_NONE && Id >= Next)) continue;
        if (Outcome && Outcome->MissionId == Id &&
            ((!bRetryDeclined && Outcome->State == EState::Declined) || Outcome->State == EState::Completed)) continue;
        if (Queue.ContainsByPredicate([Id](const FAssignment& Item) { return Item.MissionId == Id; })) continue;
        bool bAlreadyOffered = false;
        for (const auto& Offer : PilotOffers)
            if (Offer.Value.MissionId == Id) { bAlreadyOffered = true; break; }
        if (bAlreadyOffered) continue;
        Next = Id;
    }
    if (Next == INDEX_NONE) return nullptr;
    return &PilotOffers.Add(Issuer, FAssignment{Next, EState::Offered, Issuer, FDateTime::UtcNow().ToUnixTimestamp() + 3600});
}

bool AgentAI::QueueTestReplay(FName Pilot,int32 MissionId,FName Issuer)
{
    if(MissionId<101 || MissionId>107 || Pilot.IsNone() || !IsCompleted(Pilot,MissionId) || !GetQueue(Pilot).IsEmpty())return false;
    FAssignment Replay{MissionId,EState::Accepted,Issuer};Replay.bTestReplay=true;
    Queues.FindOrAdd(Pilot).Add(Replay);return true;
}

bool AgentAI::Transition(FName Pilot, int32 MissionId, EState From, EState To)
{
    auto* Queue = Queues.Find(Pilot);
    if (!Queue || Queue->IsEmpty() || (*Queue)[0].MissionId != MissionId || (*Queue)[0].State != From) return false;
    (*Queue)[0].State = To;
    return true;
}

bool AgentAI::Accept(FName Pilot, int32 MissionId, TFunction<bool(int32)> HistoricalCompletion)
{
    if (IsCompleted(Pilot,MissionId) || (HistoricalCompletion && HistoricalCompletion(MissionId)) || !ArcRequirement(Pilot,MissionId,HistoricalCompletion).IsEmpty() ||
        GetQueue(Pilot).ContainsByPredicate([MissionId](const auto& Item){return Item.MissionId==MissionId;})) return false;
    auto* PilotOffers = Offers.Find(Pilot);
    if (!PilotOffers) return false;
    for (auto It = PilotOffers->CreateIterator(); It; ++It)
        if (It.Value().MissionId == MissionId && (It.Value().ExpiresUtc == 0 || It.Value().ExpiresUtc > FDateTime::UtcNow().ToUnixTimestamp()))
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
    const auto Queue=GetQueue(Pilot);
    if (Queue.IsEmpty() || (IsCompleted(Pilot,MissionId) && !Queue[0].bTestReplay)) return false;
    const auto Outcome = bSucceeded ? EState::Completed : EState::Failed;
    if (!Transition(Pilot, MissionId, EState::InProgress, Outcome)) return false;
    if (bSucceeded && !Queues.FindChecked(Pilot)[0].bTestReplay) Completed.FindOrAdd(Pilot).Add(MissionId);
    LastOutcome.Add(Pilot, Queues.FindChecked(Pilot)[0]);
    Queues.FindChecked(Pilot).RemoveAt(0);
    return true;
}

void AgentAI::Interrupt(FName Pilot, int32 MissionId)
{
    Transition(Pilot, MissionId, EState::InProgress, EState::Accepted);
}

FString AgentAI::SaveJournal() const
{
    TArray<TSharedPtr<FJsonValue>> Entries;
    const auto Add = [&Entries](FName Pilot, const FAssignment& Item, const TCHAR* Kind)
    {
        auto Object = MakeShared<FJsonObject>();
        Object->SetStringField(TEXT("Pilot"), Pilot.ToString()); Object->SetStringField(TEXT("Kind"), Kind);
        Object->SetBoolField(TEXT("TestReplay"),Item.bTestReplay);
        Object->SetStringField(TEXT("Issuer"), Item.Issuer.ToString()); Object->SetNumberField(TEXT("Mission"), Item.MissionId);
        Object->SetNumberField(TEXT("State"), static_cast<int32>(Item.State)); Object->SetNumberField(TEXT("Expiry"), Item.ExpiresUtc);
        Entries.Add(MakeShared<FJsonValueObject>(Object));
    };
    for (const auto& Pilot : Offers) for (const auto& Offer : Pilot.Value) Add(Pilot.Key, Offer.Value, TEXT("Offer"));
    for (const auto& Pilot : Queues) for (const auto& Item : Pilot.Value) Add(Pilot.Key, Item, TEXT("Queue"));
    for (const auto& Pilot : LastOutcome) Add(Pilot.Key, Pilot.Value, TEXT("Outcome"));
    for (const auto& Pilot : Completed) for (int32 Id : Pilot.Value) Add(Pilot.Key, FAssignment{Id, EState::Completed}, TEXT("Completed"));
    FString Json; FJsonSerializer::Serialize(Entries, TJsonWriterFactory<>::Create(&Json)); return Json;
}
bool AgentAI::RestoreJournal(const FString& Json)
{
    if (Json.IsEmpty()) { *this = AgentAI(); return true; }
    TArray<TSharedPtr<FJsonValue>> Entries;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Entries) || Entries.Num() > 4096) return false;
    AgentAI Restored;
    for (const auto& Value : Entries)
    {
        const TSharedPtr<FJsonObject>* Object = nullptr; if (!Value->TryGetObject(Object) || !Object || !Object->IsValid()) return false;
        FString Pilot, Kind, Issuer; int32 Mission = 0, State = 0; double Expiry = 0;
        if (!(*Object)->TryGetStringField(TEXT("Pilot"), Pilot) || Pilot.IsEmpty() ||
            !(*Object)->TryGetStringField(TEXT("Kind"), Kind) || !(*Object)->TryGetStringField(TEXT("Issuer"), Issuer) ||
            !(*Object)->TryGetNumberField(TEXT("Mission"), Mission) || Mission <= 0 ||
            !(*Object)->TryGetNumberField(TEXT("State"), State) || State < 0 || State > static_cast<int32>(EState::Declined) ||
            !(*Object)->TryGetNumberField(TEXT("Expiry"), Expiry) || !FMath::IsFinite(Expiry) || Expiry < 0 || Expiry > 1.e12) return false;
        FAssignment Item{Mission, static_cast<EState>(State), FName(*Issuer), static_cast<int64>(Expiry)};
        (*Object)->TryGetBoolField(TEXT("TestReplay"),Item.bTestReplay);
        const FName Id(*Pilot);
        if (Kind == TEXT("Offer") && Item.State == EState::Offered) Restored.Offers.FindOrAdd(Id).Add(Item.Issuer, Item);
        else if (Kind == TEXT("Queue") && (Item.State == EState::Accepted || Item.State == EState::InProgress))
        { Item.State = EState::Accepted; Restored.Queues.FindOrAdd(Id).Add(Item); }
        else if (Kind == TEXT("Outcome")) Restored.LastOutcome.Add(Id, Item);
        else if (Kind == TEXT("Completed")) Restored.Completed.FindOrAdd(Id).Add(Mission);
        else return false;
    }
    *this = MoveTemp(Restored); return true;
}

FText AgentAI::ArcRequirement(FName Pilot,int32 MissionId,TFunction<bool(int32)> HistoricalCompletion) const
{
    // Training and authored campaigns retain their own existing progression.
    if(MissionId<=910000 || MissionId>=1000000) return FText::GetEmpty();
    return FMissionArcCatalog::Get().Requirement(MissionId,[&](int32 Id)
        {return IsCompleted(Pilot,Id) || (HistoricalCompletion && HistoricalCompletion(Id));});
}
FText AgentAI::ArcStatus(FName Pilot,int32 MissionId,TFunction<bool(int32)> HistoricalCompletion) const
{
    if(MissionId<=910000 || MissionId>=1000000) return FText::GetEmpty();
    return FMissionArcCatalog::Get().Status(MissionId,[&](int32 Id)
        {return IsCompleted(Pilot,Id) || (HistoricalCompletion && HistoricalCompletion(Id));});
}
