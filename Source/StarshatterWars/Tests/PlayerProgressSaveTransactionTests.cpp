#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "PlayerProgressSaveTransaction.h"

namespace
{
    const FString PrimarySaveSlotForTest = TEXT("PlayerSave");
    const FString Profile = TEXT("PlayerSave_Profile_0123456789abcdef0123456789abcdef");
    TArray<uint8> Bytes(uint8 Value) { return {Value, static_cast<uint8>(Value + 1)}; }
    class FMemorySlots final : public IPlayerProgressSlotIO
    {
    public:
        TMap<FString, TArray<uint8>> Slots;
        TFunction<bool(const FString&)> FailWrite;
        TFunction<bool(const FString&)> FailDelete;
        TFunction<void(const FString&)> AfterWrite;
        int32 Writes = 0;
        bool Exists(const FString& Slot) override { return Slots.Contains(Slot); }
        bool Read(const FString& Slot, TArray<uint8>& Data) override
        {
            if (auto* Found = Slots.Find(Slot)) { Data = *Found; return true; }
            return false;
        }
        bool Write(const FString& Slot, const TArray<uint8>& Data) override
        {
            ++Writes;
            if (FailWrite && FailWrite(Slot)) return false;
            Slots.Add(Slot, Data);
            if (AfterWrite) AfterWrite(Slot);
            return true;
        }
        bool Remove(const FString& Slot) override
        {
            if (FailDelete && FailDelete(Slot)) return false;
            Slots.Remove(Slot); return true;
        }
        void Seed() { Slots.Add(PrimarySaveSlotForTest, Bytes(10)); Slots.Add(Profile, Bytes(20)); }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProgressCommitTest, "Starshatter.PlayerSave.Transaction.CommitSuccess",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgressCommitTest::RunTest(const FString&)
{
    FMemorySlots IO; IO.Seed();
    IO.Slots.Add(TEXT("SV_20261118010203000"), Bytes(80));
    TestTrue(TEXT("Commit succeeds"), PlayerProgressSaveTransaction::Save(IO, PrimarySaveSlotForTest, Profile, Bytes(30)));
    TestTrue(TEXT("Both slots same candidate"), IO.Slots.FindRef(PrimarySaveSlotForTest) == Bytes(30) && IO.Slots.FindRef(Profile) == Bytes(30));
    TestEqual(TEXT("Only targets and immutable snapshot remain"), IO.Slots.Num(), 3);
    TestTrue(TEXT("Snapshot untouched"), IO.Slots.FindRef(TEXT("SV_20261118010203000")) == Bytes(80));
    TestFalse(TEXT("Snapshot cannot be transaction target"), PlayerProgressSaveTransaction::Save(IO, PrimarySaveSlotForTest, TEXT("SV_20261118010203000"), Bytes(30)));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProgressSecondaryFailureTest, "Starshatter.PlayerSave.Transaction.SecondaryWriteFailure",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgressSecondaryFailureTest::RunTest(const FString&)
{
    FMemorySlots IO; IO.Seed(); bool Failed = false;
    IO.FailWrite = [&](const FString& Slot) { if (Slot == Profile && !Failed) { Failed = true; return true; } return false; };
    TestFalse(TEXT("Secondary failure reported"), PlayerProgressSaveTransaction::Save(IO, PrimarySaveSlotForTest, Profile, Bytes(30)));
    TestTrue(TEXT("Both original states restored"), IO.Slots.FindRef(PrimarySaveSlotForTest) == Bytes(10) && IO.Slots.FindRef(Profile) == Bytes(20));
    TestEqual(TEXT("Journal/backups removed after rollback"), IO.Slots.Num(), 2);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProgressCrashRecoveryTest, "Starshatter.PlayerSave.Transaction.CrashRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgressCrashRecoveryTest::RunTest(const FString&)
{
    FMemorySlots IO; IO.Seed(); TMap<FString, TArray<uint8>> Crash;
    IO.AfterWrite = [&](const FString& Slot) { if (Slot == PrimarySaveSlotForTest) Crash = IO.Slots; };
    TestTrue(TEXT("Original operation completes"), PlayerProgressSaveTransaction::Save(IO, PrimarySaveSlotForTest, Profile, Bytes(30)));
    FMemorySlots Restart; Restart.Slots = Crash;
    TestTrue(TEXT("Crash image has pending marker"), Restart.Exists(PlayerProgressSaveTransaction::JournalSlot(PrimarySaveSlotForTest)));
    TestTrue(TEXT("Recovery succeeds"), PlayerProgressSaveTransaction::Recover(Restart, PrimarySaveSlotForTest));
    TestTrue(TEXT("Recovery restores both pretransaction bytes"), Restart.Slots.FindRef(PrimarySaveSlotForTest) == Bytes(10) && Restart.Slots.FindRef(Profile) == Bytes(20));
    TestTrue(TEXT("Recovery idempotent"), PlayerProgressSaveTransaction::Recover(Restart, PrimarySaveSlotForTest));
    // Repeat with a previously absent authoritative slot.
    IO = FMemorySlots(); IO.Slots.Add(Profile, Bytes(20));
    IO.AfterWrite = [&](const FString& Slot) { if (Slot == PrimarySaveSlotForTest) Crash = IO.Slots; };
    PlayerProgressSaveTransaction::Save(IO, PrimarySaveSlotForTest, Profile, Bytes(30));
    Restart.Slots = Crash;
    TestTrue(TEXT("Absent-slot rollback succeeds"), PlayerProgressSaveTransaction::Recover(Restart, PrimarySaveSlotForTest));
    TestFalse(TEXT("Rollback removes formerly absent slot"), Restart.Exists(PrimarySaveSlotForTest));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProgressMalformedTest, "Starshatter.PlayerSave.Transaction.MalformedJournal",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgressMalformedTest::RunTest(const FString&)
{
    FMemorySlots IO; IO.Seed(); IO.Slots.Add(PlayerProgressSaveTransaction::JournalSlot(PrimarySaveSlotForTest), Bytes(1));
    TestFalse(TEXT("Invalid journal blocks recovery"), PlayerProgressSaveTransaction::Recover(IO, PrimarySaveSlotForTest));
    TestFalse(TEXT("Invalid journal blocks new transaction"), PlayerProgressSaveTransaction::Save(IO, PrimarySaveSlotForTest, Profile, Bytes(30)));
    TestEqual(TEXT("No targets written"), IO.Writes, 0);
    TestTrue(TEXT("Original base untouched"), IO.Slots.FindRef(PrimarySaveSlotForTest) == Bytes(10));
    // A syntactically valid journal must still be bound to this base and a managed profile.
    FMemorySlots Source; Source.Seed(); TMap<FString, TArray<uint8>> Crash;
    Source.AfterWrite = [&](const FString& Slot) { if (Slot == PrimarySaveSlotForTest) Crash = Source.Slots; };
    PlayerProgressSaveTransaction::Save(Source, PrimarySaveSlotForTest, Profile, Bytes(30));
    auto Tamper = [&](const FString& From, const FString& To)
    {
        IO.Slots = Crash;
        auto& Marker = IO.Slots.FindChecked(PlayerProgressSaveTransaction::JournalSlot(PrimarySaveSlotForTest));
        FUTF8ToTCHAR Decode(reinterpret_cast<const ANSICHAR*>(Marker.GetData()), Marker.Num());
        FString Text(Decode.Length(), Decode.Get());
        Text.ReplaceInline(*From, *To, ESearchCase::CaseSensitive);
        FTCHARToUTF8 Encode(*Text); Marker.Reset();
        Marker.Append(reinterpret_cast<const uint8*>(Encode.Get()), Encode.Length());
        IO.Writes = 0;
        TestFalse(TEXT("Bound journal validation rejects tampering"), PlayerProgressSaveTransaction::Recover(IO, PrimarySaveSlotForTest));
        TestEqual(TEXT("Tampered journal never writes targets"), IO.Writes, 0);
    };
    Tamper(TEXT("\"PlayerSave\""), TEXT("\"AnotherPilot\""));
    Tamper(Profile, TEXT("SV_20261118010203000"));
    IO.Slots = Crash;
    IO.Slots.Add(PrimarySaveSlotForTest + TEXT("__ProgressTxn_Base"), Bytes(99));
    TestFalse(TEXT("Corrupt backup blocks rollback before targets change"), PlayerProgressSaveTransaction::Recover(IO, PrimarySaveSlotForTest));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProgressMarkerDeleteTest, "Starshatter.PlayerSave.Transaction.MarkerDeleteFailure",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgressMarkerDeleteTest::RunTest(const FString&)
{
    FMemorySlots IO; IO.Seed();
    IO.FailDelete = [](const FString& Slot) { return Slot == PlayerProgressSaveTransaction::JournalSlot(PrimarySaveSlotForTest); };
    TestFalse(TEXT("Commit deletion failure reported"), PlayerProgressSaveTransaction::Save(IO, PrimarySaveSlotForTest, Profile, Bytes(30)));
    TestTrue(TEXT("Original bytes restored despite pending marker"), IO.Slots.FindRef(PrimarySaveSlotForTest) == Bytes(10) && IO.Slots.FindRef(Profile) == Bytes(20));
    TestTrue(TEXT("Marker retained"), IO.Exists(PlayerProgressSaveTransaction::JournalSlot(PrimarySaveSlotForTest)));
    TestFalse(TEXT("Still blocked while marker deletion fails"), PlayerProgressSaveTransaction::Recover(IO, PrimarySaveSlotForTest));
    IO.FailDelete = nullptr;
    TestTrue(TEXT("Retry finishes recovery"), PlayerProgressSaveTransaction::Recover(IO, PrimarySaveSlotForTest));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProgressRestoreFailureTest, "Starshatter.PlayerSave.Transaction.RestoreFailureBlocks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProgressRestoreFailureTest::RunTest(const FString&)
{
    FMemorySlots IO; IO.Seed(); bool FailedSecondary = false;
    IO.FailWrite = [&](const FString& Slot)
    {
        if (Slot == Profile) { FailedSecondary = true; return true; }
        return FailedSecondary && Slot == PrimarySaveSlotForTest;
    };
    TestFalse(TEXT("Failure reported"), PlayerProgressSaveTransaction::Save(IO, PrimarySaveSlotForTest, Profile, Bytes(30)));
    TestTrue(TEXT("Journal retained for unfinished rollback"), IO.Exists(PlayerProgressSaveTransaction::JournalSlot(PrimarySaveSlotForTest)));
    TestFalse(TEXT("Failed restoration blocks reads via recovery gate"), PlayerProgressSaveTransaction::Recover(IO, PrimarySaveSlotForTest));
    IO.FailWrite = nullptr;
    TestTrue(TEXT("Later recovery succeeds"), PlayerProgressSaveTransaction::Recover(IO, PrimarySaveSlotForTest));
    TestTrue(TEXT("Original bytes restored"), IO.Slots.FindRef(PrimarySaveSlotForTest) == Bytes(10) && IO.Slots.FindRef(Profile) == Bytes(20));
    return true;
}
#endif
