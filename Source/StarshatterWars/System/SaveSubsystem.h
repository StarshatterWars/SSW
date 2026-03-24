#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameStructs.h" 
#include "SaveSubsystem.generated.h"

class UTimerSubsystem;
class UCampaignSave;
class UUniverseSaveGame;
class UPlayerSaveGame;

UENUM(BlueprintType)
enum class EAutosavePolicy : uint8
{
	Disabled,
	EveryMinute,
	Every5Minutes,
	Every10Minutes
};

UCLASS()
class STARSHATTERWARS_API USaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ------------------------
	// Lifecycle
	// ------------------------
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ------------------------
	// Config
	// ------------------------
	UPROPERTY()
	EAutosavePolicy AutosavePolicy = EAutosavePolicy::EveryMinute;

	UPROPERTY()
	int32 UserIndex = 0;

	// ------------------------
	// Player Save API (REAL)
	// ------------------------
	
	UPROPERTY()
	FS_PlayerGameInfo PlayerInfo;

	UFUNCTION()
	bool LoadPlayer(const FString& SlotName);

	UFUNCTION()
	bool SavePlayer(const FString& SlotName, bool bForce = false);

	UFUNCTION()
	bool DoesPlayerSaveExist(const FString& SlotName) const;

	// Cached player info used by the game
	const FS_PlayerGameInfo& GetPlayerInfo() const { return PlayerInfo; }
	FS_PlayerGameInfo& GetPlayerInfoMutable() { return PlayerInfo; }
	void SetPlayerInfo(const FS_PlayerGameInfo& InInfo) { PlayerInfo = InInfo; }

	// ------------------------
	// Universe Save API
	// ------------------------
	UFUNCTION()
	bool LoadOrCreateUniverse(const FString& InUniverseId, int64 BaseUnixSecondsIfNew);

	UFUNCTION()
	bool SaveUniverse(bool bForce = false);

	UFUNCTION()
	uint64 GetUniverseTimeSeconds() const { return UniverseTimeSeconds; }

	UFUNCTION()
	int64 GetUniverseBaseUnixSeconds() const { return UniverseBaseUnixSeconds; }

	// ------------------------
	// Campaign Save API
	// ------------------------
	UFUNCTION()
	UCampaignSave* LoadOrCreateCampaignSave(int32 CampaignIndex, FName RowName, const FString& DisplayName);

	UFUNCTION()
	UCampaignSave* CreateNewCampaignSave(int32 CampaignIndex, FName RowName, const FString& DisplayName);

	UFUNCTION()
	bool SaveCampaign(bool bForce = false);

	UFUNCTION()
	bool DoesCampaignSaveExist(FName RowName) const;

	UFUNCTION()
	UCampaignSave* GetCampaignSave() const { return CampaignSave; }

	// ------------------------
	// Autosave
	// ------------------------
	UFUNCTION()
	void SetAutosaveEnabled(bool bEnabled);

private:
	// ------------------------
	// Timer binding (autosave trigger)
	// ------------------------
	void BindToTimer();
	void UnbindFromTimer();
	void HandleUniverseMinute(uint64 UniverseSecondsNow);
	int32 GetAutosaveIntervalMinutes() const;

	// ------------------------
	// Slot naming
	// ------------------------
	FString GetUniverseSlotName() const;

private:
	// Cached slot context
	UPROPERTY() FString PlayerSlotName;
	UPROPERTY() FString UniverseId;

	// Universe state mirrored for saving
	uint64 UniverseTimeSeconds = 0;
	int64  UniverseBaseUnixSeconds = 0;

	// Active campaign save
	UPROPERTY() UCampaignSave* CampaignSave = nullptr;

	// Autosave
	bool bAutosaveEnabled = true;
	uint64 LastAutosaveMinute = MAX_uint64;
};
