#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CampaignSave.generated.h"

/**
 * Minimal Campaign SaveGame:
 * - Campaign identity (index + row name + display name)
 * - Campaign clock anchored to UniverseTimeSeconds
 *
 * Helper methods:
 * - Slot naming (Campaign 1 -> Slot 0)
 * - T+ derivation from UniverseTimeSeconds
 * - Starshatter-style formatting: "DD/HH:MM:SS" with 1-based day (01 at T+0)
 */
UCLASS()
class STARSHATTERWARS_API UCampaignSave : public USaveGame
{
	GENERATED_BODY()

public:
	// ----------------------
// Identity
// ----------------------

/** 1-based campaign index (Campaign 1, Campaign 2...). */
	UPROPERTY(SaveGame)
	int32 CampaignIndex = 1;

	// NEW – preferred, stable slot naming
	static FString MakeSlotNameFromRowName(FName RowName);

	/** Stable identifier for DT_Campaign lookup/validation. */
	UPROPERTY(SaveGame)
	FName CampaignRowName = NAME_None;

	/** Optional display name for menus/Operations header. */
	UPROPERTY(SaveGame)
	FString CampaignDisplayName;

	// ----------------------
	// Campaign Clock
	// ----------------------

	/** UniverseTimeSeconds when campaign began (anchor for T+). */
	UPROPERTY(SaveGame)
	uint64 CampaignStartUniverseSeconds = 0;

	/** Set true after InitializeClock is called at campaign start. */
	UPROPERTY(SaveGame)
	bool bInitialized = false;

public:
	// ----------------------
	// Helper methods
	// ----------------------

	/**
	 * Campaign 1 -> "CampaignSave_0"
	 * Campaign 2 -> "CampaignSave_1"
	 */
	static FString MakeSlotNameFromCampaignIndex(int32 InCampaignIndex);

	/** Initialize campaign clock anchor from current UniverseTimeSeconds. */
	void InitializeCampaignClock(uint64 UniverseTimeSecondsNow);

	/** Derive T+ seconds from current UniverseTimeSeconds. */
	uint64 GetTPlusSeconds(uint64 UniverseTimeSecondsNow) const;

	/** Format "DD/HH:MM:SS" (1-based day: T+0 -> 01/00:00:00). */
	static FString FormatTPlus_DD_HHMMSS(uint64 TPlusSeconds);

	/** Convenience: derive + format. */
	FString GetTPlusDisplay(uint64 UniverseTimeSecondsNow) const;
};
