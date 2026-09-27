#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "TimerSubsystem.generated.h"

class UCampaignSave;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnUniverseSecond, uint64);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUniverseMinute, uint64);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUniverseHour, uint64);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUniverseDay, uint64);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMissionSecond, int32);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCampaignTPlusChanged, uint64, uint64);

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
	FOnUniverseSecond OnUniverseSecond;
	FOnUniverseMinute OnUniverseMinute;
	FOnUniverseHour   OnUniverseHour;
	FOnUniverseDay    OnUniverseDay;

	FOnMissionSecond OnMissionSecond;

	FOnCampaignTPlusChanged OnCampaignTPlusChanged;

	UPROPERTY()
	double TimeScale = 1.0;

	UPROPERTY()
	double TimeStepSeconds = 1.0;

	UPROPERTY()
	double MissionTimeScale = 1.0;

	UPROPERTY()
	int64 PlayerPlaytimeSeconds = 0;

	UPROPERTY()
	bool bCountPlaytimeWhilePaused = false;

	uint64 UniverseTimeSeconds = 0;
	int64 UniverseBaseUnixSeconds = 0;

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
	void ManualMissionTick(float DeltaSeconds);

	UFUNCTION()
	int32 GetMissionTimeMS() const
	{
		return FMath::RoundToInt(MissionTimelineSeconds * 1000.0);
	}

	UFUNCTION()
	EMissionClockState GetMissionClockState() const { return MissionClockState; }

	UFUNCTION()
	double GetMissionTimeSeconds() const { return MissionTimelineSeconds; }

	UFUNCTION()
	int32 GetMissionTimeSecondsInt() const
	{
		return FMath::FloorToInt(MissionTimelineSeconds);
	}

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

	UFUNCTION(BlueprintCallable, Category = "Time|Campaign")
	void RestartCampaignClock(bool bSaveImmediately = true);

	static UTimerSubsystem* Get();

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	bool HandleTicker(float DeltaSeconds);
	void AdvanceMissionClock(float DeltaSeconds);

	FTSTicker::FDelegateHandle TickHandle;
	double AccumRealSeconds = 0.0;

	uint64 LastMinute = MAX_uint64;
	uint64 LastHour = MAX_uint64;
	uint64 LastDay = MAX_uint64;

	uint64 LastBroadcastSecond = 0;
	uint64 LastBroadcastMinute = 0;

	double MissionTimelineSeconds = 0.0;
	EMissionClockState MissionClockState = EMissionClockState::Stopped;
	int32 LastMissionSecondBroadcast = TNumericLimits<int32>::Min();

	TWeakObjectPtr<UCampaignSave> CampaignSave;
	uint64 LastBroadcastTPlus = MAX_uint64;
	uint64 CachedCampaignTPlusSeconds = 0;
};