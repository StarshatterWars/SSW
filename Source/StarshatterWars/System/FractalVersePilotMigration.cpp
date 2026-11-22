#include "FractalVersePilotMigration.h"
#include "LocalGameStateSave.h"
#include "LocalGameStateSubsystem.h"
#include "PlayerSaveGame.h"
#include "JsonObjectConverter.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"

namespace
{
bool HexId(const FString& Value)
{ if (Value.Len()!=32) return false; for (TCHAR C:Value) if (!FChar::IsHexDigit(C)) return false; return true; }
bool SafeSlot(const FString& Value)
{
    if (Value.IsEmpty() || Value.Len()>200) return false;
    for (TCHAR C:Value) if (!((C>=TEXT('a')&&C<=TEXT('z')) || (C>=TEXT('A')&&C<=TEXT('Z')) ||
                            (C>=TEXT('0')&&C<=TEXT('9')) || C==TEXT('_'))) return false;
    return true;
}
class FReadOnlyPilotFiles final:public IPlayerProgressSlotIO
{
public:
    explicit FReadOnlyPilotFiles(const FString& InDirectory):Directory(FPaths::ConvertRelativePathToFull(InDirectory)) {}
    bool Exists(const FString& Slot) override
    { return SafeSlot(Slot) && FPlatformFileManager::Get().GetPlatformPhysical().FileExists(*(Directory/(Slot+TEXT(".sav")))); }
    bool Read(const FString& Slot,TArray<uint8>& Out) override
    {
        Out.Reset(); if (!SafeSlot(Slot)) return false;
        // Legacy sources are physical save files, not cooked-content sandbox paths.
        TUniquePtr<IFileHandle> File(FPlatformFileManager::Get().GetPlatformPhysical().OpenRead(*(Directory/(Slot+TEXT(".sav")))));
        if (!File) return false;
        const int64 Size=File->Size();
        if (Size<16 || Size>64*1024*1024) return false;
        Out.SetNumUninitialized(static_cast<int32>(Size)); return File->Read(Out.GetData(),Out.Num());
    }
    bool Write(const FString&,const TArray<uint8>&) override { return false; }
    bool Remove(const FString&) override { return false; }
private:
    FString Directory;
};
FString PilotMigrationDigest(const TArray<uint8>& Bytes)
{ FSHAHash Hash; FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Hash.Hash); return Hash.ToString(); }
USaveGame* ReadObject(IPlayerProgressSlotIO& IO,const FString& Slot,TMap<FString,TArray<uint8>>& Sources)
{
    if (!SafeSlot(Slot)) return nullptr;
    TArray<uint8> Bytes;
    if (!IO.Read(Slot,Bytes) || Bytes.Num()<16 || Bytes.Num()>64*1024*1024 ||
        Bytes[0]!='G' || Bytes[1]!='V' || Bytes[2]!='A' || Bytes[3]!='S') return nullptr;
    USaveGame* Object=UGameplayStatics::LoadGameFromMemory(Bytes);
    if (Object) Sources.Add(Slot,MoveTemp(Bytes));
    return Object;
}
bool BackupFile(const FString& Path,const TArray<uint8>& Bytes)
{
    auto& Files=IFileManager::Get(); TArray<uint8> Check;
    if (Files.FileExists(*Path)) return FFileHelper::LoadFileToArray(Check,*Path) && Check==Bytes;
    const FString Pending=Path+TEXT(".pending-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    const bool Written=FFileHelper::SaveArrayToFile(Bytes,*Pending) &&
        FFileHelper::LoadFileToArray(Check,*Pending) && Check==Bytes && Files.Move(*Path,*Pending,false,false);
    if (!Written) Files.Delete(*Pending);
    return Written;
}
}

FString FFractalVersePilotMigration::EpicAccountSlot(const FString& EpicAccountId,const FString& ProductUserId)
{
    if (!HexId(EpicAccountId) || !HexId(ProductUserId)) return {};
    // This is FUniqueNetIdEOS::ToString() and the existing sign-in slot convention.
    const FString Key=TEXT("EOS:")+EpicAccountId.ToLower()+TEXT("|")+ProductUserId.ToLower();
    return TEXT("PlayerSave_")+FMD5::HashAnsiString(*Key);
}

bool FFractalVersePilotMigration::Read(IPlayerProgressSlotIO& IO,const FString& Base,FFractalVersePilotCandidate& Out,FString& Reason)
{
    Out=FFractalVersePilotCandidate(); Reason.Empty();
    if ((Base!=TEXT("PlayerSave") && (!Base.StartsWith(TEXT("PlayerSave_")) || !HexId(Base.Mid(11)))) || !SafeSlot(Base))
    { Reason=TEXT("Invalid server-selected account slot."); return false; }
    if (IO.Exists(PlayerProgressSaveTransaction::JournalSlot(Base)))
    { Reason=TEXT("An interrupted local save needs recovery before migration."); return false; }
    FFractalVersePilotCandidate Candidate; Candidate.BaseSlot=Base; Candidate.ProfileSlot=Base;
    TStrongObjectPtr<UPlayerSaveGame> Save(Cast<UPlayerSaveGame>(ReadObject(IO,Base,Candidate.Sources)));
    if (!Save || Save->SaveVersion<1 || Save->SaveVersion>2)
    { Reason=TEXT("Pilot save is missing, damaged or from an unsupported version."); return false; }
    Candidate.SourceSaveVersion=Save->SaveVersion; Candidate.Pilot=Save->PlayerInfo;
    const FString IndexSlot=Base+TEXT("_Profiles");
    if (IO.Exists(IndexSlot))
    {
        TStrongObjectPtr<UPlayerSaveIndexGame> Index(Cast<UPlayerSaveIndexGame>(ReadObject(IO,IndexSlot,Candidate.Sources)));
        if (!Index || Index->Version!=1) { Reason=TEXT("Pilot profile index is damaged."); return false; }
        const FString Active=Index->ActiveSlot;
        if (!Active.IsEmpty() && Active!=Base)
        {
            const bool Listed=Index->Players.ContainsByPredicate([&](const auto& Entry){return Entry.Slot==Active;});
            if (!Listed || !PlayerProgressSaveTransaction::IsMutableProfile(Base,Active))
            { Reason=TEXT("Select and save a mutable pilot profile before migration."); return false; }
            TStrongObjectPtr<UPlayerSaveGame> ActiveSave(Cast<UPlayerSaveGame>(ReadObject(IO,Active,Candidate.Sources)));
            if (!ActiveSave || ActiveSave->SaveVersion<1 || ActiveSave->SaveVersion>2)
            { Reason=TEXT("Active pilot profile is missing or damaged."); return false; }
            if (ActiveSave->PlayerInfo.LocalStateRevision>Candidate.Pilot.LocalStateRevision)
            { Candidate.Pilot=ActiveSave->PlayerInfo; Candidate.SourceSaveVersion=ActiveSave->SaveVersion; }
            Candidate.ProfileSlot=Active;
        }
    }
    const FString Manifest=TEXT("State_")+FMD5::HashAnsiString(*Candidate.ProfileSlot)+TEXT("_manifest");
    bool HasManifest=false, Complete=false;
    for (const FString Slot:{Manifest,Manifest+TEXT("_previous")})
    {
        if (!IO.Exists(Slot)) continue;
        HasManifest=true;
        TStrongObjectPtr<ULocalStateManifest> M(Cast<ULocalStateManifest>(ReadObject(IO,Slot,Candidate.Sources)));
        if (!M || M->Version!=1 || M->ProfileSlot!=Candidate.ProfileSlot || !M->Generation.IsValid() || M->Slots.Num()!=4) continue;
        const FString Prefix=TEXT("State_")+FMD5::HashAnsiString(*M->ProfileSlot)+TEXT("_")+M->Generation.ToString(EGuidFormats::Digits)+TEXT("_");
        static const TCHAR* Names[]={TEXT("pilot"),TEXT("economy"),TEXT("skills"),TEXT("universe")};
        bool NamesValid=true;
        for (int32 I=0;I<4;++I) if (M->Slots[I]!=Prefix+Names[I]) NamesValid=false;
        if (!NamesValid) continue; // Never read filenames supplied by an invalid manifest.
        TArray<TStrongObjectPtr<ULocalStateArea>> Held; TArray<ULocalStateArea*> Areas;
        for (const FString& Area:M->Slots)
        { Held.Emplace(Cast<ULocalStateArea>(ReadObject(IO,Area,Candidate.Sources))); Areas.Add(Held.Last().Get()); }
        if (!ULocalGameStateSubsystem::ValidateGeneration(*M,Areas)) continue;
        if (M->PlayerRevision>=Candidate.Pilot.LocalStateRevision)
        {
            Candidate.Pilot=CastChecked<ULocalPilotState>(Areas[0])->Pilot;
            Candidate.Pilot.Economy=CastChecked<ULocalEconomyState>(Areas[1])->Economy;
            Candidate.Pilot.Skills=CastChecked<ULocalSkillState>(Areas[2])->Skills;
            Candidate.Pilot.SkillQueue=CastChecked<ULocalSkillState>(Areas[2])->Queue;
            Candidate.World.Reset(CastChecked<ULocalUniverseState>(Areas[3]));
        }
        Candidate.bRecoveredPrevious=Slot!=Manifest; Complete=true; break;
    }
    if (HasManifest && !Complete) { Reason=TEXT("No complete local checkpoint generation can be migrated."); return false; }
    const auto& Economy=Candidate.Pilot.Economy;
    Candidate.Credits=Economy.Wallets.FindRef(FEconomyLedger::Pilot());
    if (Candidate.Credits<0 || (!Economy.bLegacyWalletMigrated && (Candidate.Pilot.InsuranceCredits<0 ||
        Candidate.Pilot.InsuranceCredits>MAX_int64-Candidate.Credits)))
    { Reason=TEXT("Legacy pilot balance is invalid."); return false; }
    if (!Economy.bLegacyWalletMigrated) Candidate.Credits+=Candidate.Pilot.InsuranceCredits;
    FJsonObjectConverter::CustomExportCallback ExactIntegers;
    ExactIntegers.BindLambda([](FProperty* Property,const void* Value)->TSharedPtr<FJsonValue>
    {
        if (const auto* Signed=CastField<FInt64Property>(Property)) return MakeShared<FJsonValueString>(LexToString(Signed->GetPropertyValue(Value)));
        if (const auto* Unsigned=CastField<FUInt64Property>(Property)) return MakeShared<FJsonValueString>(LexToString(Unsigned->GetPropertyValue(Value)));
        return nullptr;
    });
    auto Career=FJsonObjectConverter::UStructToJsonObject(Candidate.Pilot,0,0,&ExactIntegers);
    if (!Career) { Reason=TEXT("Pilot progression could not be converted."); return false; }
    Career->RemoveField(TEXT("economy")); Career->RemoveField(TEXT("traffic")); Career->RemoveField(TEXT("insuranceCredits"));
    auto Writer=TJsonWriterFactory<>::Create(&Candidate.CareerJson);
    if (!FJsonSerializer::Serialize(Career.ToSharedRef(),Writer)) { Reason=TEXT("Pilot progression could not be serialized."); return false; }
    TArray<FString> Keys; Candidate.Sources.GetKeys(Keys); Keys.Sort(); FString Summary;
    for (const FString& Key:Keys)
    {
        TArray<uint8> Check;
        if (!IO.Read(Key,Check) || Check!=Candidate.Sources[Key])
        { Reason=TEXT("Local saves changed during migration preparation. Close the game first."); return false; }
        Summary+=Key+TEXT(":")+PilotMigrationDigest(Check)+TEXT("\n");
    }
    if (IO.Exists(PlayerProgressSaveTransaction::JournalSlot(Base)))
    { Reason=TEXT("A local save started while preparing migration. Close the game first."); return false; }
    FTCHARToUTF8 Utf8(*Summary); TArray<uint8> SummaryBytes;
    SummaryBytes.Append(reinterpret_cast<const uint8*>(Utf8.Get()),Utf8.Length());
    Candidate.BundleDigest=PilotMigrationDigest(SummaryBytes); Out=MoveTemp(Candidate); return true;
}

bool FFractalVersePilotMigration::ReadDirectory(const FString& Directory,const FString& Base,FFractalVersePilotCandidate& Out,FString& Reason)
{
    if (Directory.IsEmpty()) { Out=FFractalVersePilotCandidate(); Reason=TEXT("A trusted legacy save directory is required."); return false; }
    FReadOnlyPilotFiles Source(Directory);
    return Read(Source,Base,Out,Reason);
}

bool FFractalVersePilotMigration::Revalidate(const FFractalVersePilotCandidate& Requested,FFractalVersePilotCandidate& Canonical,FString& Reason)
{
    class FSnapshotSource final:public IPlayerProgressSlotIO
    {
    public:
        explicit FSnapshotSource(const TMap<FString,TArray<uint8>>& In):Files(In) {}
        bool Exists(const FString& Slot) override { return Files.Contains(Slot); }
        bool Read(const FString& Slot,TArray<uint8>& Out) override { if (const auto* Data=Files.Find(Slot)) { Out=*Data;return true; } return false; }
        bool Write(const FString&,const TArray<uint8>&) override { return false; }
        bool Remove(const FString&) override { return false; }
    private:
        const TMap<FString,TArray<uint8>>& Files;
    } Source(Requested.Sources);
    if (!Read(Source,Requested.BaseSlot,Canonical,Reason)) return false;
    if (Requested.BundleDigest!=Canonical.BundleDigest || Requested.CareerJson!=Canonical.CareerJson ||
        Requested.Credits!=Canonical.Credits || Requested.ProfileSlot!=Canonical.ProfileSlot ||
        Requested.SourceSaveVersion!=Canonical.SourceSaveVersion)
    { Canonical=FFractalVersePilotCandidate(); Reason=TEXT("Prepared pilot no longer matches its source snapshot.");return false; }
    return true;
}

bool FFractalVersePilotMigration::Stage(const FString& Directory,FName Principal,const FFractalVersePilotCandidate& Requested,FString& BundlePath,FString& Reason)
{
    BundlePath.Empty(); Reason.Empty();
    FFractalVersePilotCandidate Candidate;
    if (!Revalidate(Requested,Candidate,Reason)) return false;
    if (Directory.IsEmpty() || Principal.IsNone() || Candidate.BundleDigest.Len()!=40 || Candidate.Sources.IsEmpty() || Candidate.CareerJson.IsEmpty())
    { Reason=TEXT("Verified identity and a validated pilot bundle are required."); return false; }
    for (TCHAR C:Candidate.BundleDigest) if (!FChar::IsHexDigit(C))
    { Reason=TEXT("Invalid migration bundle digest."); return false; }
    TArray<FString> Keys; Candidate.Sources.GetKeys(Keys); Keys.Sort(); FString Summary;
    for (const FString& Key:Keys) Summary+=Key+TEXT(":")+PilotMigrationDigest(Candidate.Sources[Key])+TEXT("\n");
    FTCHARToUTF8 SummaryUtf8(*Summary); TArray<uint8> SummaryBytes;
    SummaryBytes.Append(reinterpret_cast<const uint8*>(SummaryUtf8.Get()),SummaryUtf8.Length());
    if (PilotMigrationDigest(SummaryBytes)!=Candidate.BundleDigest)
    { Reason=TEXT("Migration source bundle changed after validation."); return false; }
    const FString Target=FPaths::ConvertRelativePathToFull(Directory)/FMD5::HashAnsiString(*Principal.ToString())/Candidate.BundleDigest;
    if (!IFileManager::Get().MakeDirectory(*Target,true)) { Reason=TEXT("Could not create the pilot migration backup."); return false; }
    auto Manifest=MakeShared<FJsonObject>(); auto Files=MakeShared<FJsonObject>();
    for (const auto& Pair:Candidate.Sources)
    {
        if (!SafeSlot(Pair.Key) || !BackupFile(Target/(Pair.Key+TEXT(".sav")),Pair.Value))
        { Reason=TEXT("Pilot backup failed verification; no server pilot was changed."); return false; }
        Files->SetStringField(Pair.Key,PilotMigrationDigest(Pair.Value));
    }
    Manifest->SetNumberField(TEXT("schema"),1); Manifest->SetStringField(TEXT("sourceDigest"),Candidate.BundleDigest);
    Manifest->SetStringField(TEXT("baseSlot"),Candidate.BaseSlot); Manifest->SetStringField(TEXT("profileSlot"),Candidate.ProfileSlot);
    Manifest->SetBoolField(TEXT("recoveredPrevious"),Candidate.bRecoveredPrevious);
    // Int64 credits are strings to retain exactness in JSON tools.
    Manifest->SetStringField(TEXT("credits"),LexToString(Candidate.Credits)); Manifest->SetObjectField(TEXT("files"),Files);
    Manifest->SetStringField(TEXT("career"),Candidate.CareerJson);
    FString Json; auto Writer=TJsonWriterFactory<>::Create(&Json); if (!FJsonSerializer::Serialize(Manifest,Writer)) { Reason=TEXT("Could not serialize migration metadata."); return false; }
    FTCHARToUTF8 Utf8(*Json); TArray<uint8> Bytes; Bytes.Append(reinterpret_cast<const uint8*>(Utf8.Get()),Utf8.Length());
    if (!BackupFile(Target/TEXT("prepared.json"),Bytes)) { Reason=TEXT("Could not commit the verified migration backup."); return false; }
    BundlePath=Target; return true;
}
