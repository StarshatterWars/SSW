#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "TimerSubsystem.generated.h"

class UCampaignSave;
// =======================================================
// Time / Pub-Sub Delegates
// =======================================================
// 
// 
// -------- Universe Delegates --------
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUniverseSecond, uint64 /*UniverseSeconds*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUniverseMinute, uint64 /*UniverseSeconds*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUniverseHour, uint64 /*UniverseSeconds*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUniverseDay, uint64 /*UniverseSeconds*/);

// -------- Mission/Cutscene Timeline Delegates --------
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMissionSecond, int32 /*MissionSeconds*/);

// Fired when campaign T+ changes (UI display)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCampaignTPlusChanged, uint64 /*UniverseSeconds*/, uint64 /*TPlusSeconds*/);

UENUM(BlueprintType)
enum class EMissionClockState : uint8
{
	Stopped,
	Running,
	Paused
};

UCLASS()
class STARSHATTERWARS_API UTimerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ---- Universe Events ----
	FOnUniverseSecond OnUniverseSecond;
	FOnUniverseMinute OnUniverseMinute;
	FOnUniverseHour   OnUniverseHour;
	FOnUniverseDay    OnUniverseDay;

	// ---- Mission/Cutscene Events ----
	FOnMissionSecond OnMissionSecond;

	// ---- Campaign Timer Events ----
	FOnCampaignTPlusChanged OnCampaignTPlusChanged;

	// ---- Universe Config ----
	/** 1.0 = real time, 60 = 1 min/sec, 3600 = 1 hr/sec */
	UPROPERTY()
	double TimeScale = 1.0;

	/** Fixed-step cadence (in real seconds) for advancing universe time */
	UPROPERTY()
	double TimeStepSeconds = 1.0;

	// ---- Mission Config ----
	/** 1.0 = real seconds; can speed up/slow down cutscene scheduling if needed */
	UPROPERTY()
	double MissionTimeScale = 1.0;

	// Player time
	UPROPERTY()
	int64 PlayerPlaytimeSeconds = 0;

	UPROPERTY()
	bool bCountPlaytimeWhilePaused = false;

	// ---- Universe State ----
	uint64 UniverseTimeSeconds = 0;
	int64  UniverseBaseUnixSeconds = 0;

	// ---- Universe API ----
	void StartClock();
	void StopClock();

	void OnClockTick();
	
	UFUNCTION()
	uint64 GetUniverseTimeSeconds() const { return UniverseTimeSeconds; }

	UFUNCTION()
	void SetUniverseTimeSeconds(uint64 InSeconds) { UniverseTimeSeconds = InSeconds; }

	UFUNCTION()
	void SetUniverseBaseUnixSeconds(int64 InBaseUnixSeconds) { UniverseBaseUnixSeconds = InBaseUnixSeconds; }

	UFUNCTION()
	FDateTime GetUniverseDateTime() const;

	UFUNCTION()
	FString GetUniverseDateTimeString() const;

	// ---- Mission/Cutscene Timeline API ----
	UFUNCTION()
	void StartMissionRun(bool bResetToZero = true);

	UFUNCTION()
	void StopMissionRun();

	UFUNCTION()
	void PauseMissionClock();

	UFUNCTION()
	void ResumeMissionClock();

	UFUNCTION()
	void ResetMissionClock();

	UFUNCTION()
	EMissionClockState GetMissionClockState() const { return MissionClockState; }

	UFUNCTION()
	double GetMissionTimeSeconds() const { return MissionTimelineSeconds; }

	UFUNCTION()
	int32 GetMissionTimeSecondsInt() const { return (int32)FMath::FloorToInt((float)MissionTimelineSeconds); }

	UFUNCTION()
	FText GetMissionTimerTextMMSS() const;

	UFUNCTION()
	void SetTimeScale(double NewTimeScale);

	UFUNCTION()
	void UpdateUniverseTime(float DeltaSeconds);

	UFUNCTION()
	void UpdatePlayerPlaytime(float DeltaSeconds);

	UFUNCTION()
	double GetTimeScale() const { return TimeScale; }

	// ---- Campaign (T+) API ----
	UFUNCTION()
	void SetCampaignSave(UCampaignSave* InCampaignSave);

	UFUNCTION()
	void ClearCampaignSave();

	UFUNCTION()
	bool HasCampaignSave() const { return CampaignSave.IsValid(); }

	UFUNCTION()
	uint64 GetCampaignTPlusSeconds() const { return CachedCampaignTPlusSeconds; }

	UFUNCTION()
	UCampaignSave* GetCampaignSave() const { return CampaignSave.Get(); }

	// Explicitly restart (reset) campaign timeline anchor so T+ becomes 0.
	// Does notxaffect Universe time.
	UFUNCTION(BlueprintCallable, Category = "Time|Campaign")
	void RestartCampaignClock(bool bSaveImmediately = true);

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	// World-independent ticker (survives travel, menus, etc.)
	FTSTicker::FDelegateHandle TickHandle;
	double AccumRealSeconds = 0.0;

	uint64 LastMinute = MAX_uint64;
	uint64 LastHour = MAX_uint64;
	uint64 LastDay = MAX_uint64;

	// Timer Pub/Sub
	uint64 LastBroadcastSecond = 0;
	uint64 LastBroadcastMinute = 0;

	// ---- Mission/Cutscene State ----
	double MissionTimelineSeconds = 0.0;
	EMissionClockState MissionClockState = EMissionClockState::Stopped;
	int32 LastMissionSecondBroadcast = TNumericLimits<int32>::Min();

	bool Tick(float DeltaSeconds);


	// ----Campaign(T + ) State----
	TWeakObjectPtr<UCampaignSave> CampaignSave;
	uint64 LastBroadcastTPlus = MAX_uint64;
	uint64 CachedCampaignTPlusSeconds = 0;
};
