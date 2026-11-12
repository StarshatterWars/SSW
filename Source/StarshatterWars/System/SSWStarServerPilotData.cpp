#include "SSWSessionLobbyGameMode.h"

#include "GameFramework/PlayerState.h"
#include "Game.h"
#include "HAL/FileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Online/CoreOnline.h"
#include "Ship.h"
#include "SimEvent.h"

namespace
{
    const TCHAR* const Fields[] = { TEXT("GunShots"), TEXT("GunHits"), TEXT("MissileShots"),
        TEXT("MissileHits"), TEXT("GunKills"), TEXT("MissileKills"), TEXT("Deaths"),
        TEXT("Collisions"), TEXT("Points"), TEXT("CommandPoints") };
    constexpr const TCHAR* PilotDataSection = TEXT("Pilot");

    TArray<int64> Snapshot(const FString& Name)
    {
        ShipStats* Stats = ShipStats::Find(TCHAR_TO_ANSI(*Name));
        if (!Stats)
        {
            TArray<int64> Empty;
            Empty.SetNumZeroed(UE_ARRAY_COUNT(Fields));
            return Empty;
        }
        Stats->Summarize();
        return {Stats->GetGunShots(), Stats->GetGunHits(), Stats->GetMissileShots(),
            Stats->GetMissileHits(), Stats->GetGunKills(), Stats->GetMissileKills(),
            Stats->GetDeaths(), Stats->GetColls(), Stats->GetPoints(), Stats->GetCommandPoints()};
    }

    void Accumulate(const FString& Name, TArray<int64>& Baseline, TArray<int64>& Totals)
    {
        if (Name.IsEmpty()) return;
        auto Current = Snapshot(Name);
        for (int32 Index = 0; Index < Current.Num(); ++Index)
            Totals[Index] += Current[Index] - Baseline[Index];
        Baseline = MoveTemp(Current);
    }

    void Increment(FConfigFile& File, const TCHAR* Key, int64 Delta)
    {
        FString Previous;
        File.GetString(PilotDataSection, Key, Previous);
        File.SetString(PilotDataSection, Key, *LexToString(FCString::Atoi64(*Previous) + Delta));
    }
}

void ASSWSessionLobbyGameMode::TrackServerPilot(ASSWSessionLobbyController& Pilot, const Ship& Fighter)
{
    if (GetNetMode() != NM_DedicatedServer) return;
    auto& Record = ServerPilots.FindOrAdd(&Pilot);
    const FString ShipName = ANSI_TO_TCHAR(Fighter.GetName());
    if (Record.ShipName == ShipName) return;
    if (Record.Totals.IsEmpty()) Record.Totals.SetNumZeroed(UE_ARRAY_COUNT(Fields));
    Accumulate(Record.ShipName, Record.Baseline, Record.Totals);
    Record.ShipName = ShipName;
    Record.Baseline = Snapshot(ShipName);
    if (!Record.bFlew) Record.FlightStartMs = Game::GetGameTime();
    Record.bFlew = true;
}

void ASSWSessionLobbyGameMode::SaveServerPilot(ASSWSessionLobbyController& Pilot, bool bFinish)
{
    if (GetNetMode() != NM_DedicatedServer) return;
    FServerPilotRecord Record;
    if (bFinish && !ServerPilots.RemoveAndCopyValue(&Pilot, Record)) return;
    const auto* Player = Pilot.GetPlayerState<APlayerState>();
    if (!Player || !Player->GetUniqueId().IsValid()) return;
    const auto& Id = Player->GetUniqueId();
    const FString AccountKey = Id.IsV1() ? Id.GetV1()->ToString() : UE::Online::ToString(Id.GetV2());
    const FString NetworkKey = Id.GetType().ToString() + TEXT(":") + AccountKey;
    const FString Directory = FPaths::ProjectSavedDir() / TEXT("StarServer/Pilots");
    auto& Files = IFileManager::Get();
    if (!Files.MakeDirectory(*Directory, true)) return;
    const FString Path = Directory / (FMD5::HashAnsiString(*NetworkKey) + TEXT(".ini"));
    const FString Temp = Path + TEXT(".tmp");
    FConfigFile File;
    if (Files.FileExists(*Path))
    {
        File.Read(Path);
        if (File.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("[StarServer] Could not read pilot record %s"), *Path);
            return;
        }
    }
    File.SetString(PilotDataSection, TEXT("DisplayName"), *Player->GetPlayerName().Left(64));
    File.SetString(PilotDataSection, TEXT("LastSeenUtc"), *FDateTime::UtcNow().ToIso8601());
    if (!bFinish)
    {
        ServerPilots.FindOrAdd(&Pilot);
        Increment(File, TEXT("Connections"), 1);
    }
    else if (Record.bFlew && LobbyMission)
    {
        Accumulate(Record.ShipName, Record.Baseline, Record.Totals);
        Increment(File, TEXT("Sorties"), 1);
        Increment(File, TEXT("FlightSeconds"), uint32(Game::GetGameTime() - Record.FlightStartMs) / 1000);
        for (int32 Index = 0; Index < Record.Totals.Num(); ++Index)
            Increment(File, Fields[Index], Record.Totals[Index]);
    }
    if (!File.Write(Temp) || !Files.Move(*Path, *Temp, true))
        UE_LOG(LogTemp, Warning, TEXT("[StarServer] Could not save pilot record %s"), *Path);
}
