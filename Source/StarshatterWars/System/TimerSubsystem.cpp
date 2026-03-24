#include "TimerSubsystem.h"
#include "CampaignSave.h"
#include "Kismet/GameplayStatics.h"
#include "Containers/Ticker.h"

void UTimerSubsystem::RestartCampaignClock(bool bSaveImmediately)
{
	UCampaignSave* CS = CampaignSave.Get();
	if (!CS)
	{
		UE_LOG(LogTemp, Error, TEXT("TimerSubsystem::RestartCampaignClock: No CampaignSave set"));
		return;
	}

	// Re-anchor to current universe time; Universe time itself continues.
	const uint64 Now = UniverseTimeSeconds;

	// Reset anchor (this is the restart)
	CS->CampaignStartUniverseSeconds = Now;
	CS->bInitialized = true;

	// Force immediate UI refresh
	LastBroadcastTPlus = MAX_uint64;
	CachedCampaignTPlusSeconds = 0;

	// Optional persistence: overwrite the per-campaign slot
	if (bSaveImmediately)
	{
		if (CS->CampaignRowName.IsNone())
		{
			UE_LOG(LogTemp, Error, TEXT("RestartCampaignClock: CampaignRowName is None; cannotxsave"));
		}
		else
		{
			const FString Slot = UCampaignSave::MakeSlotNameFromRowName(CS->CampaignRowName);
			constexpr int32 UserIndex = 0;

			const bool bOK = UGameplayStatics::SaveGameToSlot(CS, Slot, UserIndex);
			UE_LOG(LogTemp, Warning, TEXT("RestartCampaignClock: Saved slot=%s ok=%d"), *Slot, bOK ? 1 : 0);
		}
	}

	// Broadcast immediately so any screen snaps to T+ 00...
	OnCampaignTPlusChanged.Broadcast(UniverseTimeSeconds, 0ULL);
}

void UTimerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Universe time should never stop:
	StartClock();
}

void UTimerSubsystem::Deinitialize()
{
	StopClock();
	Super::Deinitialize();
}

void UTimerSubsystem::StartClock()
{
	// Prevent duplicates
	if (TickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		TickHandle.Reset();
	}

	if (TimeStepSeconds <= 0.0)
	{
		TimeStepSeconds = 1.0;
	}

	AccumRealSeconds = 0.0;

	TickHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UTimerSubsystem::Tick)
	);

	UE_LOG(LogTemp, Log, TEXT("UTimerSubsystem::StartClock (Ticker) Step=%.3f UniverseScale=%.2f MissionScale=%.2f"),
		TimeStepSeconds, TimeScale, MissionTimeScale);
}

void UTimerSubsystem::StopClock()
{
	if (TickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		TickHandle.Reset();
	}

	UE_LOG(LogTemp, Log, TEXT("UTimerSubsystem::StopClock (Ticker)"));
}

bool UTimerSubsystem::Tick(float DeltaSeconds)
{
	AccumRealSeconds += (double)DeltaSeconds;

	// Fixed-step advancement (preserves your original TimerManager cadence semantics)
	while (AccumRealSeconds >= TimeStepSeconds)
	{
		AccumRealSeconds -= TimeStepSeconds;
		OnClockTick();
	}

	return true; // keep ticking
}

// -------------------- Mission API --------------------

void UTimerSubsystem::StartMissionRun(bool bResetToZero)
{
	// IMPORTANT:
	// Do notxcall StartClock() here. Universe clock is always running.

	if (bResetToZero)
	{
		ResetMissionClock();
	}

	MissionClockState = EMissionClockState::Running;
}

void UTimerSubsystem::StopMissionRun()
{
	MissionClockState = EMissionClockState::Stopped;
}

void UTimerSubsystem::PauseMissionClock()
{
	if (MissionClockState == EMissionClockState::Running)
	{
		MissionClockState = EMissionClockState::Paused;
	}
}

void UTimerSubsystem::ResumeMissionClock()
{
	if (MissionClockState == EMissionClockState::Paused)
	{
		MissionClockState = EMissionClockState::Running;
	}
}

void UTimerSubsystem::ResetMissionClock()
{
	MissionTimelineSeconds = 0.0;
	LastMissionSecondBroadcast = TNumericLimits<int32>::Min();

	// Immediately broadcast 0 so UI/cutscene systems snap to 00:00 on restart
	OnMissionSecond.Broadcast(0);
}

FText UTimerSubsystem::GetMissionTimerTextMMSS() const
{
	const int32 TotalSeconds = GetMissionTimeSecondsInt();
	const int32 Minutes = TotalSeconds / 60;
	const int32 Seconds = TotalSeconds % 60;
	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds));
}

// -------------------- Clock Tick --------------------

void UTimerSubsystem::OnClockTick()
{
	// ---- Universe time ----
	const double DeltaUniverse = TimeStepSeconds * TimeScale;
	const uint64 AddSeconds = (uint64)FMath::Max(1.0, FMath::RoundToDouble(DeltaUniverse));
	UniverseTimeSeconds += AddSeconds;

	OnUniverseSecond.Broadcast(UniverseTimeSeconds);

	const uint64 CurMinute = UniverseTimeSeconds / 60ULL;
	if (CurMinute != LastMinute)
	{
		LastMinute = CurMinute;
		OnUniverseMinute.Broadcast(UniverseTimeSeconds);
	}

	const uint64 CurHour = UniverseTimeSeconds / 3600ULL;
	if (CurHour != LastHour)
	{
		LastHour = CurHour;
		OnUniverseHour.Broadcast(UniverseTimeSeconds);
	}

	const uint64 CurDay = UniverseTimeSeconds / 86400ULL;
	if (CurDay != LastDay)
	{
		LastDay = CurDay;
		OnUniverseDay.Broadcast(UniverseTimeSeconds);
	}

	// ---- Mission/Cutscene timeline (shared) ----
	if (MissionClockState == EMissionClockState::Running)
	{
		// Mission timeline advances in real seconds by default
		MissionTimelineSeconds += (TimeStepSeconds * MissionTimeScale);

		const int32 CurMissionSec = GetMissionTimeSecondsInt();
		if (CurMissionSec != LastMissionSecondBroadcast)
		{
			LastMissionSecondBroadcast = CurMissionSec;
			OnMissionSecond.Broadcast(CurMissionSec);
		}
	}

	// ---- Campaign (T+) ----
	if (CampaignSave.IsValid())
	{
		const uint64 NewTPlus = CampaignSave->GetTPlusSeconds(UniverseTimeSeconds);
		CachedCampaignTPlusSeconds = NewTPlus;

		if (NewTPlus != LastBroadcastTPlus)
		{
			LastBroadcastTPlus = NewTPlus;
			OnCampaignTPlusChanged.Broadcast(UniverseTimeSeconds, NewTPlus);
		}
	}
}

// -------------------- Date/Time helpers --------------------

FDateTime UTimerSubsystem::GetUniverseDateTime() const
{
	// If you want a specific base date, set UniverseBaseUnixSeconds externally (e.g., 2228-01-01)
	if (UniverseBaseUnixSeconds <= 0)
	{
		return FDateTime::FromUnixTimestamp((int64)UniverseTimeSeconds);
	}

	const int64 Unix = UniverseBaseUnixSeconds + (int64)UniverseTimeSeconds;
	return FDateTime::FromUnixTimestamp(Unix);
}

FString UTimerSubsystem::GetUniverseDateTimeString() const
{
	return GetUniverseDateTime().ToString(TEXT("%Y-%m-%d %H:%M:%S"));
}

void UTimerSubsystem::SetTimeScale(double NewTimeScale)
{
	TimeScale = FMath::Clamp(NewTimeScale, 0.0, 1.0e7);
	UE_LOG(LogTemp, Warning, TEXT("TimeScale set to %.2f"), TimeScale);
}

void UTimerSubsystem::UpdateUniverseTime(float DeltaSeconds)
{
	UniverseTimeSeconds += (int64)FMath::RoundToInt(DeltaSeconds * TimeScale);
}

void UTimerSubsystem::UpdatePlayerPlaytime(float DeltaSeconds)
{
	PlayerPlaytimeSeconds += (int64)FMath::RoundToInt(DeltaSeconds);
}

void UTimerSubsystem::SetCampaignSave(UCampaignSave* InCampaignSave)
{
	CampaignSave = InCampaignSave;

	// Force next tick to broadcast immediately so UI updates right away:
	LastBroadcastTPlus = MAX_uint64;
	CachedCampaignTPlusSeconds = 0;

	UE_LOG(LogTemp, Warning,
		TEXT("TimerSubsystem: SetCampaignSave ObjName=%s Row=%s Index=%d Start=%llu Init=%d"),
		*GetNameSafe(InCampaignSave),
		InCampaignSave ? *InCampaignSave->CampaignRowName.ToString() : TEXT("None"),
		InCampaignSave ? InCampaignSave->CampaignIndex : -1,
		(unsigned long long)(InCampaignSave ? InCampaignSave->CampaignStartUniverseSeconds : 0ULL),
		InCampaignSave ? (InCampaignSave->bInitialized ? 1 : 0) : 0
	);
}



void UTimerSubsystem::ClearCampaignSave()
{
	CampaignSave = nullptr;
	LastBroadcastTPlus = MAX_uint64;
	CachedCampaignTPlusSeconds = 0;

	UE_LOG(LogTemp, Log, TEXT("TimerSubsystem: CampaignSave cleared"));
}
