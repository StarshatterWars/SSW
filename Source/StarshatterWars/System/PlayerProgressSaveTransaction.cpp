#include "PlayerProgressSaveTransaction.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/Crc.h"

namespace
{
    constexpr int32 JournalVersion = 1;
    FString BaseBackup(const FString& Base) { return Base + TEXT("__ProgressTxn_Base"); }
    FString ProfileBackup(const FString& Base) { return Base + TEXT("__ProgressTxn_Profile"); }
    struct FBefore
    {
        bool Exists = false;
        TArray<uint8> Bytes;
    };
    bool ValidBase(const FString& Base)
    {
        return !Base.IsEmpty() && Base.Len() <= 200 && !Base.StartsWith(TEXT("SV_")) &&
            !Base.Contains(TEXT("/")) && !Base.Contains(TEXT("\\")) && !Base.Contains(TEXT("..")) &&
            !Base.Contains(TEXT("__ProgressTxn_"));
    }
    bool Snapshot(IPlayerProgressSlotIO& IO, const FString& Slot, FBefore& Before)
    {
        Before.Exists = IO.Exists(Slot);
        return !Before.Exists || (IO.Read(Slot, Before.Bytes) && !Before.Bytes.IsEmpty());
    }
    bool WriteVerified(IPlayerProgressSlotIO& IO, const FString& Slot, const TArray<uint8>& Bytes)
    {
        if (!IO.Write(Slot, Bytes)) return false;
        TArray<uint8> Check;
        return IO.Read(Slot, Check) && Check == Bytes;
    }
    bool RemoveVerified(IPlayerProgressSlotIO& IO, const FString& Slot)
    {
        if (!IO.Exists(Slot)) return true;
        IO.Remove(Slot);
        return !IO.Exists(Slot);
    }
    void CleanBackups(IPlayerProgressSlotIO& IO, const FString& Base)
    {
        RemoveVerified(IO, BaseBackup(Base));
        RemoveVerified(IO, ProfileBackup(Base));
    }
    void Describe(TSharedRef<FJsonObject> Object, const TCHAR* Prefix, const FBefore& Before)
    {
        const FString P(Prefix);
        Object->SetBoolField(P + TEXT("Exists"), Before.Exists);
        Object->SetNumberField(P + TEXT("Size"), Before.Bytes.Num());
        Object->SetNumberField(P + TEXT("CRC"), FCrc::MemCrc32(Before.Bytes.GetData(), Before.Bytes.Num()));
    }
    bool ReadBefore(IPlayerProgressSlotIO& IO, const TSharedPtr<FJsonObject>& Object,
        const TCHAR* Prefix, const FString& Backup, FBefore& Before)
    {
        const FString P(Prefix);
        double Size = -1, CRC = -1;
        if (!Object->TryGetBoolField(P + TEXT("Exists"), Before.Exists) ||
            !Object->TryGetNumberField(P + TEXT("Size"), Size) ||
            !Object->TryGetNumberField(P + TEXT("CRC"), CRC) ||
            Size < 0 || Size > MAX_int32 || Size != FMath::FloorToDouble(Size) ||
            CRC < 0 || CRC > MAX_uint32 || CRC != FMath::FloorToDouble(CRC)) return false;
        if (!Before.Exists) return Size == 0 && CRC == FCrc::MemCrc32(nullptr, 0);
        return Size > 0 && IO.Read(Backup, Before.Bytes) && Before.Bytes.Num() == Size &&
            FCrc::MemCrc32(Before.Bytes.GetData(), Before.Bytes.Num()) == static_cast<uint32>(CRC);
    }
    bool Restore(IPlayerProgressSlotIO& IO, const FString& Slot, const FBefore& Before)
    {
        return Before.Exists ? WriteVerified(IO, Slot, Before.Bytes) : RemoveVerified(IO, Slot);
    }
}

bool PlayerProgressSaveTransaction::IsMutableProfile(const FString& Base, const FString& Profile)
{
    if (!ValidBase(Base)) return false;
    const FString Prefix = Base + TEXT("_Profile_");
    if (!Profile.StartsWith(Prefix, ESearchCase::CaseSensitive)) return false;
    FGuid Guid;
    return FGuid::ParseExact(Profile.Mid(Prefix.Len()), EGuidFormats::Digits, Guid);
}
FString PlayerProgressSaveTransaction::JournalSlot(const FString& Base)
{
    return Base + TEXT("__ProgressTxn_Journal");
}
bool PlayerProgressSaveTransaction::Recover(IPlayerProgressSlotIO& IO, const FString& Base)
{
    const FString Marker = JournalSlot(Base);
    if (!IO.Exists(Marker)) return true;
    if (!ValidBase(Base)) return false;
    TArray<uint8> Data;
    if (!IO.Read(Marker, Data) || Data.IsEmpty() || Data.Num() > 65536) return false;
    // UTF-8 conversion has an explicit byte count; malformed/truncated JSON fails closed.
    FUTF8ToTCHAR Text(reinterpret_cast<const ANSICHAR*>(Data.GetData()), Data.Num());
    TSharedPtr<FJsonObject> Object;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(FString(Text.Length(), Text.Get())), Object) || !Object) return false;
    FString RecordedBase, Profile;
    double Version = 0;
    if (!Object->TryGetNumberField(TEXT("Version"), Version) || Version != JournalVersion ||
        !Object->TryGetStringField(TEXT("Base"), RecordedBase) || RecordedBase != Base ||
        !Object->TryGetStringField(TEXT("Profile"), Profile) || !IsMutableProfile(Base, Profile)) return false;
    FBefore OldBase, OldProfile;
    // Validate both backups before touching either target.
    if (!ReadBefore(IO, Object, TEXT("Base"), BaseBackup(Base), OldBase) ||
        !ReadBefore(IO, Object, TEXT("Profile"), ProfileBackup(Base), OldProfile)) return false;
    const bool BaseOK = Restore(IO, Base, OldBase);
    const bool ProfileOK = Restore(IO, Profile, OldProfile);
    if (!BaseOK || !ProfileOK || !RemoveVerified(IO, Marker)) return false;
    CleanBackups(IO, Base);
    return true;
}
bool PlayerProgressSaveTransaction::Save(IPlayerProgressSlotIO& IO, const FString& Base,
    const FString& Profile, const TArray<uint8>& Candidate)
{
    if (!IsMutableProfile(Base, Profile) || Candidate.IsEmpty() || !Recover(IO, Base)) return false;
    FBefore OldBase, OldProfile;
    if (!Snapshot(IO, Base, OldBase) || !Snapshot(IO, Profile, OldProfile)) return false;
    if ((OldBase.Exists && !WriteVerified(IO, BaseBackup(Base), OldBase.Bytes)) ||
        (OldProfile.Exists && !WriteVerified(IO, ProfileBackup(Base), OldProfile.Bytes)))
    {
        CleanBackups(IO, Base);
        return false;
    }
    auto Object = MakeShared<FJsonObject>();
    Object->SetNumberField(TEXT("Version"), JournalVersion);
    Object->SetStringField(TEXT("Base"), Base);
    Object->SetStringField(TEXT("Profile"), Profile);
    Describe(Object, TEXT("Base"), OldBase);
    Describe(Object, TEXT("Profile"), OldProfile);
    FString Text;
    FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Text));
    FTCHARToUTF8 UTF8(*Text);
    TArray<uint8> MarkerBytes;
    MarkerBytes.Append(reinterpret_cast<const uint8*>(UTF8.Get()), UTF8.Length());
    const FString Marker = JournalSlot(Base);
    if (!WriteVerified(IO, Marker, MarkerBytes))
    {
        // Never touch targets after a failed journal write; retain any marker for recovery.
        if (!IO.Exists(Marker)) CleanBackups(IO, Base);
        return false;
    }
    if (!WriteVerified(IO, Base, Candidate) || !WriteVerified(IO, Profile, Candidate) || !RemoveVerified(IO, Marker))
    {
        Recover(IO, Base);
        return false;
    }
    CleanBackups(IO, Base);
    return true;
}
