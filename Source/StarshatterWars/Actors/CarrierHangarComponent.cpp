#include "CarrierHangarComponent.h"
#include "Hangar.h"

UCarrierHangarComponent::UCarrierHangarComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCarrierHangarComponent::BindHangar(Hangar* InHangar, int32 InDeckIndex)
{
    RuntimeHangar = InHangar;
    AssociatedDeckIndex = InDeckIndex;
}

TArray<FCarrierHangarSlotStatus> UCarrierHangarComponent::GetSlotStatuses() const
{
    TArray<FCarrierHangarSlotStatus> Result;
    if (!RuntimeHangar) return Result;
    for (int32 Q=0; Q<RuntimeHangar->NumSquadrons(); ++Q)
        for (int32 I=0; I<RuntimeHangar->SquadronSize(Q); ++I)
        {
            const HangarSlot* Entry = RuntimeHangar->GetSlot(Q,I);
            if (!Entry) continue;
            FCarrierHangarSlotStatus State;
            State.Squadron = Q; State.Slot = I;
            State.SquadronName = UTF8_TO_TCHAR(RuntimeHangar->SquadronName(Q).data());
            State.LegacyState = RuntimeHangar->GetState(Entry);
            State.bInTurnaround = State.LegacyState == Hangar::MAINT;
            State.bCanPrepare = State.LegacyState == Hangar::STORAGE;
            State.SecondsRemaining = FMath::Max(0.f, float(RuntimeHangar->TimeRemaining(Entry)));
            State.Status = State.bInTurnaround ? TEXT("Turnaround") : UTF8_TO_TCHAR(RuntimeHangar->StatusName(Entry).data());
            Result.Add(State);
        }
    return Result;
}
