#pragma once
#include "CoreMinimal.h"

// Logical SaveGame slots only. Implementations must report failed reads separately from absence.
class IPlayerProgressSlotIO
{
public:
    virtual ~IPlayerProgressSlotIO() = default;
    virtual bool Exists(const FString& Slot) = 0;
    virtual bool Read(const FString& Slot, TArray<uint8>& Data) = 0;
    virtual bool Write(const FString& Slot, const TArray<uint8>& Data) = 0;
    virtual bool Remove(const FString& Slot) = 0;
};

namespace PlayerProgressSaveTransaction
{
    bool IsMutableProfile(const FString& Base, const FString& Profile);
    FString JournalSlot(const FString& Base);
    // A pending journal always means rollback. Removing it is the commit point.
    bool Recover(IPlayerProgressSlotIO& IO, const FString& Base);
    bool Save(IPlayerProgressSlotIO& IO, const FString& Base, const FString& Profile, const TArray<uint8>& Candidate);
}
