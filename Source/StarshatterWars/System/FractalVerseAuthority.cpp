#include "FractalVerseAuthority.h"
#include "FractalVerseShipProvisioning.h"
#include "FractalVersePilotPromotion.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "UObject/UObjectGlobals.h"

namespace
{
FString Digest(const TArray<uint8>& Data)
{
    FSHAHash Hash;
    FSHA1::HashBuffer(Data.GetData(), Data.Num(), Hash.Hash);
    return Hash.ToString();
}
bool ReadGeneration(const FString& Directory, const TCHAR* Head, FFractalVerseWorld& State, int32& Slot)
{
    if (!UObjectInitialized() || !GetTransientPackage()) return false;
    FString Record;
    if (!FFileHelper::LoadFileToString(Record, *(Directory / Head))) return false;
    TArray<FString> Parts;
    Record.ParseIntoArray(Parts, TEXT("\n"), true);
    if (Parts.Num() != 2 || (Parts[0] != TEXT("0") && Parts[0] != TEXT("1"))) return false;
    const int32 CandidateSlot = Parts[0] == TEXT("0") ? 0 : 1;
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *(Directory / FString::Printf(TEXT("world-%d.sav"), CandidateSlot))) ||
        Bytes.IsEmpty() || Digest(Bytes) != Parts[1]) return false;
    auto* Save = Cast<UFractalVerseCheckpoint>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!Save || Save->State.Schema != 1 || !Save->State.Universe.IsValid() || Save->State.Revision < 0) return false;
    State = Save->State;
    Slot = CandidateSlot;
    return true;
}
}

FFractalVerseStatus FFractalVerseAuthority::Status(bool bReady) const
{
    FFractalVerseStatus Result;
    Result.Universe = World.Universe;
    Result.Revision = World.Revision;
    Result.UniverseBaseUtc=World.UniverseBaseUtc; Result.UniverseSeconds=World.UniverseSeconds; Result.ServerUtc=World.LastServerUtc;
    Result.EconomySeconds = World.Economy.EconomyClockSeconds;
    Result.Inventories = World.Economy.Inventories.Num();
    Result.Journeys = World.Traffic.Journeys.Num();
    Result.Jobs = World.Economy.Jobs.Num();
    Result.bPersistenceReady = bReady;
    return Result;
}

bool FFractalVerseAuthority::SynchronizeClock(int64 ServerUtc)
{
    if (ServerUtc<=0 || ServerUtc>FDateTime(9799,12,31).ToUnixTimestamp()) return false;
    if (World.LastServerUtc==0)
    {
        const FDateTime Gmt=FDateTime::FromUnixTimestamp(ServerUtc);
        // Preserve the 2228 universe epoch and the established GMT calendar policy.
        const int32 Year=FMath::Max(2228,Gmt.GetYear()+200);
        const int32 Day=FMath::Min(Gmt.GetDay(),FDateTime::DaysInMonth(Year,Gmt.GetMonth()));
        World.UniverseBaseUtc=FDateTime(2228,1,1).ToUnixTimestamp();
        World.UniverseSeconds=FDateTime(Year,Gmt.GetMonth(),Day,Gmt.GetHour(),Gmt.GetMinute(),Gmt.GetSecond()).ToUnixTimestamp()-World.UniverseBaseUtc;
        World.LastServerUtc=ServerUtc;
        return true;
    }
    if (ServerUtc<=World.LastServerUtc) return false;
    World.UniverseSeconds+=ServerUtc-World.LastServerUtc;
    World.LastServerUtc=ServerUtc;
    return true;
}

bool FFractalVerseAuthority::EnsurePilot(FName VerifiedPrincipal, FString& Reason)
{
    Reason.Empty();
    if (VerifiedPrincipal.IsNone()) { Reason=TEXT("Verified account required."); return false; }
    if (World.Pilots.Contains(VerifiedPrincipal)) return true;
    if (World.Revision == MAX_int64 || World.Economy.Wallets.Contains(VerifiedPrincipal))
    { Reason=TEXT("Pilot creation needs account migration."); return false; }
    World.Pilots.Add(VerifiedPrincipal, FFractalVersePilot());
    World.Economy.PlayerAccounts.Add(VerifiedPrincipal);
    World.Economy.Wallets.Add(VerifiedPrincipal, 50000);
    FEconomyTransaction Grant;
    Grant.Id=FGuid::NewGuid(); Grant.To=VerifiedPrincipal; Grant.Credits=50000;
    Grant.TimestampUtc=World.LastServerUtc; Grant.Category=EWalletTransactionCategory::OpeningBalance;
    Grant.Reason=TEXT("Career starting allowance"); Grant.bHasWalletDeltas=true;
    Grant.WalletDeltas.Add(VerifiedPrincipal,50000); Grant.WalletBalancesAfter.Add(VerifiedPrincipal,50000);
    World.Economy.Journal.Add(Grant);
    World.Economy.CommittedTransactions.Add(Grant.Id);
    ++World.Revision;
    return true;
}

bool FFractalVerseAuthority::ImportPilot(FName Principal,const FFractalVersePilotCandidate& Source,FString& Reason)
{
    FFractalVerseWorld Candidate;
    if (!FFractalVersePilotPromotion::Build(World,Principal,Source,Candidate,Reason)) return false;
    World=MoveTemp(Candidate); return true;
}

bool FFractalVerseAuthority::PilotView(FName VerifiedPrincipal, FFractalVersePilotView& Out) const
{
    Out=FFractalVersePilotView();
    const auto* Pilot=World.Pilots.Find(VerifiedPrincipal);
    if (VerifiedPrincipal.IsNone() || !Pilot) return false;
    Out.bAuthenticated=true; Out.Callsign=Pilot->Callsign;
    Out.HomeSystem=Pilot->HomeSystem; Out.HomeRegion=Pilot->HomeRegion;
    Out.Credits=World.Economy.Wallets.FindRef(VerifiedPrincipal); Out.Revision=World.Revision;
    return true;
}

bool FFractalVerseAuthority::Restore(const FFractalVerseWorld& State, FString& Reason)
{
    if (State.Schema != 1 || !State.Universe.IsValid() || State.Revision < 0 || State.UniverseSeconds<0 || State.LastServerUtc<0 ||
        !FMath::IsFinite(State.Economy.EconomyClockSeconds) || State.Economy.EconomyClockSeconds < 0)
    { Reason = TEXT("Unsupported or invalid universe checkpoint."); return false; }
    for(const auto& Pair:State.PlayerShips)
        if(!State.Pilots.Contains(Pair.Key) || !FFractalVerseShipProvisioning::Validate(Pair.Value))
        {Reason=TEXT("Invalid authoritative player ship checkpoint.");return false;}
    World = State;
    for (const auto& Pilot:World.Pilots) World.Economy.PlayerAccounts.Add(Pilot.Key);
    Reason.Empty();
    return true;
}

bool FFractalVerseAuthority::Advance(double Seconds, TFunctionRef<void(FFractalVerseWorld&, double)> Simulation)
{
    if (!FMath::IsFinite(Seconds) || Seconds <= 0 || Seconds > 60 || World.Revision == MAX_int64) return false;
    Simulation(World, Seconds);
    ++World.Revision;
    return true;
}

bool FFractalVerseAuthority::Transfer(const FFractalVerseAccess& Access, const FFractalVerseTransfer& Command, FString& Reason)
{
    Reason.Empty();
    if (Access.Principal.IsNone() || !Command.Request.IsValid() || !FMath::IsFinite(Command.Quantity) || Command.Quantity <= 0 ||
        Command.Source.IsNone() || Command.Destination.IsNone() || Command.Item.IsNone())
    { Reason = TEXT("Invalid cargo request."); return false; }
    // Length-delimited names avoid delimiter aliases. Revision is part of the original request.
    FString Fingerprint = FString::Printf(TEXT("%lld/%.17g/"), Command.ExpectedRevision, Command.Quantity);
    for (FName Name : {Command.Source, Command.Destination, Command.Item})
    { const FString Text = Name.ToString(); Fingerprint += FString::Printf(TEXT("%d:%s"), Text.Len(), *Text); }
    if (const auto* Prior = World.Receipts.Find(Command.Request))
    {
        if (Prior->Principal == Access.Principal && Prior->Fingerprint == Fingerprint) return true;
        Reason = TEXT("Request identifier already belongs to another operation."); return false;
    }
    if (!Access.bDocked || !Access.AccessibleInventories.Contains(Command.Source) || !Access.AccessibleInventories.Contains(Command.Destination))
    { Reason = TEXT("Dock at the cargo service before transferring these holds."); return false; }
    if (Command.ExpectedRevision != World.Revision)
    { Reason = TEXT("Universe changed; refresh cargo and try again."); return false; }
    if (World.Receipts.Num() >= 100000 || World.Revision == MAX_int64)
    { Reason = TEXT("Cargo service is temporarily unavailable."); return false; }
    FEconomyState Candidate = World.Economy;
    if (!FEconomyLedger::Transfer(Candidate, Access.Principal, Command.Source, Command.Destination, Command.Item, Command.Quantity))
    { Reason = TEXT("Cargo ownership, available quantity or capacity check failed."); return false; }
    World.Economy = MoveTemp(Candidate);
    ++World.Revision;
    FFractalVerseReceipt Receipt;
    Receipt.Principal = Access.Principal; Receipt.Fingerprint = MoveTemp(Fingerprint); Receipt.Revision = World.Revision;
    World.Receipts.Add(Command.Request, MoveTemp(Receipt));
    return true;
}

bool FFractalVerseCheckpointStore::Load(const FString& Directory, FFractalVerseWorld& State, FString& Reason)
{
    int32 Slot = 0;
    if (ReadGeneration(Directory, TEXT("head"), State, Slot) || ReadGeneration(Directory, TEXT("previous"), State, Slot))
    { Reason.Empty(); return true; }
    Reason = TEXT("No valid FractalVerse checkpoint generation."); return false;
}

bool FFractalVerseCheckpointStore::Save(const FString& Directory, const FFractalVerseWorld& State, FString& Reason)
{
    // GameInstance teardown may run after UObject shutdown on a failed map load.
    // The engine pre-exit callback saves while serialization is still available.
    if (!UObjectInitialized() || !GetTransientPackage())
    { Reason = TEXT("Checkpoint serialization is unavailable after UObject shutdown."); return false; }
    auto& Files = IFileManager::Get();
    if (!Files.MakeDirectory(*Directory, true)) { Reason = TEXT("Cannot create universe save directory."); return false; }
    FFractalVerseWorld Previous;
    int32 OldSlot = 1;
    const bool HasCurrent = ReadGeneration(Directory, TEXT("head"), Previous, OldSlot);
    const bool HasPrevious = !HasCurrent && ReadGeneration(Directory, TEXT("previous"), Previous, OldSlot);
    const int32 Slot = 1 - OldSlot;
    auto* Save = NewObject<UFractalVerseCheckpoint>(); Save->State = State;
    TArray<uint8> Bytes;
    const FString Blob = Directory / FString::Printf(TEXT("world-%d.sav"), Slot);
    if (!UGameplayStatics::SaveGameToMemory(Save, Bytes) || !FFileHelper::SaveArrayToFile(Bytes, *(Blob + TEXT(".pending"))) ||
        !Files.Move(*Blob, *(Blob + TEXT(".pending")), true, true))
    { Reason = TEXT("Cannot write universe checkpoint payload."); return false; }
    const FString Head = FString::Printf(TEXT("%d\n%s"), Slot, *Digest(Bytes));
    if (!FFileHelper::SaveStringToFile(Head, *(Directory / TEXT("head.pending")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    { Reason = TEXT("Cannot write universe checkpoint index."); return false; }
    if (HasCurrent && Files.Copy(*(Directory / TEXT("previous")), *(Directory / TEXT("head")), true, true) != COPY_OK)
    { Reason = TEXT("Cannot retain previous universe checkpoint."); return false; }
    // A recovered previous head remains intact until the new head commits.
    (void)HasPrevious;
    if (!Files.Move(*(Directory / TEXT("head")), *(Directory / TEXT("head.pending")), true, true))
    { Reason = TEXT("Cannot commit universe checkpoint index."); return false; }
    Reason.Empty(); return true;
}
