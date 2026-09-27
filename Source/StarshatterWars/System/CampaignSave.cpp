#include "CampaignSave.h"

FString UCampaignSave::MakeSlotNameFromCampaignIndex(int32 InCampaignIndex)
{
	// Enforce 1-based campaign index
	const int32 SafeIndex = FMath::Max(1, InCampaignIndex);

	// Campaign 1 -> Slot 0
	const int32 SlotNumber = SafeIndex - 1;

	return FString::Printf(TEXT("CampaignSave_%d"), SlotNumber);
}

void UCampaignSave::InitializeCampaignClock(uint64 UniverseTimeSecondsNow)
{
	// One-shot: never allow re-anchoring
	if (bInitialized && CampaignStartUniverseSeconds > 0)
	{
		return;
	}

	CampaignStartUniverseSeconds = UniverseTimeSecondsNow;
	bInitialized = true;
}

uint64 UCampaignSave::GetTPlusSeconds(uint64 UniverseTimeSecondsNow) const
{
	if (CampaignStartUniverseSeconds == 0)
	{
		return 0ULL;
	}

	if (UniverseTimeSecondsNow <= CampaignStartUniverseSeconds)
	{
		return 0ULL;
	}

	return UniverseTimeSecondsNow - CampaignStartUniverseSeconds;
}

FString UCampaignSave::FormatTPlus_DD_HHMMSS(uint64 TPlusSeconds)
{
	const uint64 Days0 = TPlusSeconds / 86400ULL;
	const uint64 RemD = TPlusSeconds % 86400ULL;

	const uint64 Hours = RemD / 3600ULL;
	const uint64 RemH = RemD % 3600ULL;

	const uint64 Mins = RemH / 60ULL;
	const uint64 Secs = RemH % 60ULL;

	// Starshatter-like 1-based day display:
	// T+0 => 01/00:00:00
	const uint64 DayDisplay = Days0 + 1ULL;

	return FString::Printf(TEXT("%02llu/%02llu:%02llu:%02llu"),
		DayDisplay, Hours, Mins, Secs);
}

FString UCampaignSave::GetTPlusDisplay(uint64 UniverseTimeSecondsNow) const
{
	return FormatTPlus_DD_HHMMSS(GetTPlusSeconds(UniverseTimeSecondsNow));
}

FString UCampaignSave::MakeSlotNameFromRowName(FName RowName)
{
	checkf(!RowName.IsNone(), TEXT("MakeSlotNameFromRowName called with None"));

	return FString::Printf(
		TEXT("CampaignSave_%s"),
		*RowName.ToString()
	);
}
