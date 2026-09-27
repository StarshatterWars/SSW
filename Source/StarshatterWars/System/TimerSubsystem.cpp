#include "TimerSubsystem.h"
#include "CampaignSave.h"
#include "Kismet/GameplayStatics.h"
#include "Containers/Ticker.h"

static TWeakObjectPtr<UTimerSubsystem> ActiveTimer;

void UTimerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ActiveTimer = this;

	UE_LOG(LogTemp, Warning,
		TEXT("[TimerSubsystem] Initialize This=%p"),
		this);

	StartClock();
}

void UTimerSubsystem::Deinitialize()
{
	StopClock();

	ActiveTimer = nullptr;

	Super::Deinitialize();
}

UTimerSubsystem* UTimerSubsystem::Get()
{
	return ActiveTimer.Get();
}

void UTimerSubsystem::StartClock()
{
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
		FTickerDelegate::CreateUObject(this, &UTimerSubsystem::HandleTicker),
		0.0f
	);

	UE_LOG(LogTemp, Warning,
		TEXT("[TimerSubsystem] StartClock TickHandleValid=%d Step=%.3f UniverseScale=%.2f MissionScale=%.2f"),
		TickHandle.IsValid() ? 1 : 0,
		TimeStepSeconds,
		TimeScale,
		MissionTimeScale);
}

void UTimerSubsystem::StopClock()
{
	if (TickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		TickHandle.Reset();
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[TimerSubsystem] StopClock"));
}

bool UTimerSubsystem::HandleTicker(float DeltaSeconds)
{
	AdvanceMissionClock(DeltaSeconds);

	AccumRealSeconds += (double)DeltaSeconds;

	while (AccumRealSeconds >= TimeStepSeconds)
	{
		AccumRealSeconds -= TimeStepSeconds;
		OnClockTick();
	}

	return true;
}

void UTimerSubsystem::AdvanceMissionClock(float DeltaSeconds)
{
	if (MissionClockState != EMissionClockState::Running)
	{
		return;
	}

	MissionTimelineSeconds += (double)DeltaSeconds * MissionTimeScale;

	const int32 CurMissionSec = GetMissionTimeSecondsInt();

	if (CurMissionSec != LastMissionSecondBroadcast)
	{
		LastMissionSecondBroadcast = CurMissionSec;
		OnMissionSecond.Broadcast(CurMissionSec);

		UE_LOG(LogTemp, Warning,
			TEXT("[TimerSubsystem] MissionSecond=%d Time=%.3f MS=%d"),
			CurMissionSec,
			MissionTimelineSeconds,
			GetMissionTimeMS());
	}
}

void UTimerSubsystem::StartMissionRun(bool bResetToZero)
{
	if (bResetToZero)
	{
		ResetMissionClock();
	}

	MissionClockState = EMissionClockState::Running;

	UE_LOG(LogTemp, Warning,
		TEXT("[TimerSubsystem] StartMissionRun Reset=%d State=%d Time=%.3f This=%p Active=%p"),
		bResetToZero ? 1 : 0,
		(int32)MissionClockState,
		MissionTimelineSeconds,
		this,
		UTimerSubsystem::Get());
}

void UTimerSubsystem::StopMissionRun()
{
	MissionClockState = EMissionClockState::Stopped;

	UE_LOG(LogTemp, Warning,
		TEXT("[TimerSubsystem] StopMissionRun Time=%.3f"),
		MissionTimelineSeconds);
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

	OnMissionSecond.Broadcast(0);

	UE_LOG(LogTemp, Warning,
		TEXT("[TimerSubsystem] ResetMissionClock"));
}

FText UTimerSubsystem::GetMissionTimerTextMMSS() const
{
	const int32 TotalSeconds = GetMissionTimeSecondsInt();
	const int32 Minutes = TotalSeconds / 60;
	const int32 Seconds = TotalSeconds % 60;

	return FText::FromString(
		FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds));
}

void UTimerSubsystem::OnClockTick()
{
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

FDateTime UTimerSubsystem::GetUniverseDateTime() const
{
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

	UE_LOG(LogTemp, Warning,
		TEXT("[TimerSubsystem] TimeScale set to %.2f"),
		TimeScale);
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

	LastBroadcastTPlus = MAX_uint64;
	CachedCampaignTPlusSeconds = 0;

	UE_LOG(LogTemp, Warning,
		TEXT("[TimerSubsystem] SetCampaignSave ObjName=%s Row=%s Index=%d Start=%llu Init=%d"),
		*GetNameSafe(InCampaignSave),
		InCampaignSave ? *InCampaignSave->CampaignRowName.ToString() : TEXT("None"),
		InCampaignSave ? InCampaignSave->CampaignIndex : -1,
		(unsigned long long)(InCampaignSave ? InCampaignSave->CampaignStartUniverseSeconds : 0ULL),
		InCampaignSave ? (InCampaignSave->bInitialized ? 1 : 0) : 0);
}

void UTimerSubsystem::ClearCampaignSave()
{
	CampaignSave = nullptr;
	LastBroadcastTPlus = MAX_uint64;
	CachedCampaignTPlusSeconds = 0;

	UE_LOG(LogTemp, Log,
		TEXT("[TimerSubsystem] CampaignSave cleared"));
}

void UTimerSubsystem::RestartCampaignClock(bool bSaveImmediately)
{
	UCampaignSave* CS = CampaignSave.Get();
	if (!CS)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[TimerSubsystem] RestartCampaignClock: No CampaignSave set"));
		return;
	}

	const uint64 Now = UniverseTimeSeconds;

	CS->CampaignStartUniverseSeconds = Now;
	CS->bInitialized = true;

	LastBroadcastTPlus = MAX_uint64;
	CachedCampaignTPlusSeconds = 0;

	if (bSaveImmediately)
	{
		if (CS->CampaignRowName.IsNone())
		{
			UE_LOG(LogTemp, Error,
				TEXT("[TimerSubsystem] RestartCampaignClock: CampaignRowName is None; cannot save"));
		}
		else
		{
			const FString Slot = UCampaignSave::MakeSlotNameFromRowName(CS->CampaignRowName);
			constexpr int32 UserIndex = 0;

			const bool bOK = UGameplayStatics::SaveGameToSlot(CS, Slot, UserIndex);

			UE_LOG(LogTemp, Warning,
				TEXT("[TimerSubsystem] RestartCampaignClock Saved slot=%s ok=%d"),
				*Slot,
				bOK ? 1 : 0);
		}
	}

	OnCampaignTPlusChanged.Broadcast(UniverseTimeSeconds, 0ULL);
}

void UTimerSubsystem::ManualMissionTick(float DeltaSeconds)
{
	if (MissionClockState != EMissionClockState::Running)
	{
		return;
	}

	MissionTimelineSeconds += (double)DeltaSeconds * MissionTimeScale;

	const int32 CurMissionSec = GetMissionTimeSecondsInt();

	if (CurMissionSec != LastMissionSecondBroadcast)
	{
		LastMissionSecondBroadcast = CurMissionSec;
		OnMissionSecond.Broadcast(CurMissionSec);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[TimerSubsystem] ManualMissionTick Time=%.3f MS=%d State=%d"),
		MissionTimelineSeconds,
		GetMissionTimeMS(),
		(int32)MissionClockState);
}