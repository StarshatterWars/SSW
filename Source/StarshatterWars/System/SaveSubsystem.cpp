#include "SaveSubsystem.h"

#include "TimerSubsystem.h"
#include "CampaignSave.h"
#include "UniverseSaveGame.h"
#include "PlayerSaveGame.h"         // FS_PlayerInfo
#include "Kismet/GameplayStatics.h"

void USaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	BindToTimer();
}

void USaveSubsystem::Deinitialize()
{
	UnbindFromTimer();
	Super::Deinitialize();
}

// ------------------------
// Timer binding
// ------------------------

void USaveSubsystem::BindToTimer()
{
	if (UTimerSubsystem* Timer = GetGameInstance()->GetSubsystem<UTimerSubsystem>())
	{
		Timer->OnUniverseMinute.AddUObject(this, &USaveSubsystem::HandleUniverseMinute);
	}
}

void USaveSubsystem::UnbindFromTimer()
{
	if (UTimerSubsystem* Timer = GetGameInstance()->GetSubsystem<UTimerSubsystem>())
	{
		Timer->OnUniverseMinute.RemoveAll(this);
	}
}

int32 USaveSubsystem::GetAutosaveIntervalMinutes() const
{
	switch (AutosavePolicy)
	{
	case EAutosavePolicy::EveryMinute:   return 1;
	case EAutosavePolicy::Every5Minutes: return 5;
	case EAutosavePolicy::Every10Minutes:return 10;
	default:                             return 0;
	}
}

void USaveSubsystem::HandleUniverseMinute(uint64 UniverseSecondsNow)
{
	if (!bAutosaveEnabled)
		return;

	const int32 Interval = GetAutosaveIntervalMinutes();
	if (Interval <= 0)
		return;

	const uint64 CurMinute = UniverseSecondsNow / 60ULL;

	// first tick or throttle
	if (LastAutosaveMinute != MAX_uint64)
	{
		const uint64 Delta = CurMinute - LastAutosaveMinute;
		if ((int32)Delta < Interval)
			return;
	}

	LastAutosaveMinute = CurMinute;

	// mirror time
	UniverseTimeSeconds = UniverseSecondsNow;

	// Save Universe + Campaign always (if configured/loaded)
	SaveUniverse(false);
	SaveCampaign(false);

	// Only save player if we have a valid slot name already
	if (!PlayerSlotName.IsEmpty())
	{
		SavePlayer(PlayerSlotName, false);
	}
}

void USaveSubsystem::SetAutosaveEnabled(bool bEnabled)
{
	bAutosaveEnabled = bEnabled;
}

// ------------------------
// Player save (REAL using UPlayerSaveGame)
// ------------------------

bool USaveSubsystem::DoesPlayerSaveExist(const FString& SlotName) const
{
	return !SlotName.IsEmpty() && UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}

bool USaveSubsystem::LoadPlayer(const FString& SlotName)
{
	PlayerSlotName = SlotName;

	if (SlotName.IsEmpty())
		return false;

	if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex))
	{
		if (UPlayerSaveGame* PS = Cast<UPlayerSaveGame>(Loaded))
		{
			PlayerInfo = PS->PlayerInfo;
			return true;
		}
	}

	return false;
}

bool USaveSubsystem::SavePlayer(const FString& SlotName, bool /*bForce*/)
{
	PlayerSlotName = SlotName;

	if (SlotName.IsEmpty())
		return false;

	UPlayerSaveGame* SaveObj = Cast<UPlayerSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UPlayerSaveGame::StaticClass())
	);
	if (!SaveObj)
		return false;

	SaveObj->PlayerInfo = PlayerInfo;
	return UGameplayStatics::SaveGameToSlot(SaveObj, SlotName, UserIndex);
}

// ------------------------
// Universe save
// ------------------------

FString USaveSubsystem::GetUniverseSlotName() const
{
	// Keep consistent and stable per universe id
	return FString::Printf(TEXT("Universe_%s"), *UniverseId);
}

bool USaveSubsystem::LoadOrCreateUniverse(const FString& InUniverseId, int64 BaseUnixSecondsIfNew)
{
	UniverseId = InUniverseId;

	if (UniverseId.IsEmpty())
		return false;

	const FString Slot = GetUniverseSlotName();

	// Load if exists
	if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(Slot, UserIndex))
	{
		if (UUniverseSaveGame* U = Cast<UUniverseSaveGame>(Loaded))
		{
			UniverseBaseUnixSeconds = U->UniverseBaseUnixSeconds;
			UniverseTimeSeconds = U->UniverseTimeSeconds;

			// Push into timer (runtime authority)
			if (UTimerSubsystem* Timer = GetGameInstance()->GetSubsystem<UTimerSubsystem>())
			{
				Timer->SetUniverseBaseUnixSeconds(UniverseBaseUnixSeconds);
				Timer->SetUniverseTimeSeconds(UniverseTimeSeconds);
			}

			return true;
		}
	}

	// Create new
	UUniverseSaveGame* NewObj = Cast<UUniverseSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UUniverseSaveGame::StaticClass())
	);
	if (!NewObj)
		return false;

	NewObj->UniverseId = UniverseId;
	NewObj->UniverseBaseUnixSeconds = BaseUnixSecondsIfNew;
	NewObj->UniverseTimeSeconds = 0;

	UniverseBaseUnixSeconds = BaseUnixSecondsIfNew;
	UniverseTimeSeconds = 0;

	// Push into timer
	if (UTimerSubsystem* Timer = GetGameInstance()->GetSubsystem<UTimerSubsystem>())
	{
		Timer->SetUniverseBaseUnixSeconds(UniverseBaseUnixSeconds);
		Timer->SetUniverseTimeSeconds(UniverseTimeSeconds);
	}

	return UGameplayStatics::SaveGameToSlot(NewObj, Slot, UserIndex);
}

bool USaveSubsystem::SaveUniverse(bool /*bForce*/)
{
	if (UniverseId.IsEmpty())
		return false;

	// Pull latest runtime time from timer
	if (UTimerSubsystem* Timer = GetGameInstance()->GetSubsystem<UTimerSubsystem>())
	{
		UniverseTimeSeconds = Timer->GetUniverseTimeSeconds();

		// DO notxrecompute base time using GetUniverseDateTime().
		// Base unix seconds is a persistent anchor and must remain stable.
		// Use the cached value we loaded/created (UniverseBaseUnixSeconds).
	}

	UUniverseSaveGame* SaveObj = Cast<UUniverseSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UUniverseSaveGame::StaticClass())
	);
	if (!SaveObj)
		return false;

	SaveObj->UniverseId = UniverseId;
	SaveObj->UniverseBaseUnixSeconds = UniverseBaseUnixSeconds;
	SaveObj->UniverseTimeSeconds = UniverseTimeSeconds;

	return UGameplayStatics::SaveGameToSlot(SaveObj, GetUniverseSlotName(), UserIndex);
}

// ------------------------
// Campaign save
// ------------------------

bool USaveSubsystem::DoesCampaignSaveExist(FName RowName) const
{
	if (RowName.IsNone())
		return false;

	const FString Slot = UCampaignSave::MakeSlotNameFromRowName(RowName);
	return UGameplayStatics::DoesSaveGameExist(Slot, UserIndex);
}

UCampaignSave* USaveSubsystem::LoadOrCreateCampaignSave(int32 CampaignIndex, FName RowName, const FString& DisplayName)
{
	CampaignIndex = FMath::Max(1, CampaignIndex);
	if (RowName.IsNone())
		return nullptr;

	const FString Slot = UCampaignSave::MakeSlotNameFromRowName(RowName);

	// Load if exists
	if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(Slot, UserIndex))
	{
		if (UCampaignSave* LoadedSave = Cast<UCampaignSave>(Loaded))
		{
			// Repair identity
			LoadedSave->CampaignIndex = CampaignIndex;
			LoadedSave->CampaignRowName = RowName;
			if (!DisplayName.IsEmpty())
			{
				LoadedSave->CampaignDisplayName = DisplayName;
			}

			CampaignSave = LoadedSave;

			// Inject into timer for T+
			if (UTimerSubsystem* Timer = GetGameInstance()->GetSubsystem<UTimerSubsystem>())
			{
				Timer->SetCampaignSave(CampaignSave);
			}

			return CampaignSave;
		}
	}

	// Create new if missing
	return CreateNewCampaignSave(CampaignIndex, RowName, DisplayName);
}

UCampaignSave* USaveSubsystem::CreateNewCampaignSave(int32 CampaignIndex, FName RowName, const FString& DisplayName)
{
	CampaignIndex = FMath::Max(1, CampaignIndex);
	if (RowName.IsNone())
		return nullptr;

	const FString Slot = UCampaignSave::MakeSlotNameFromRowName(RowName);

	UCampaignSave* NewSave = Cast<UCampaignSave>(
		UGameplayStatics::CreateSaveGameObject(UCampaignSave::StaticClass())
	);
	if (!NewSave)
		return nullptr;

	NewSave->CampaignIndex = CampaignIndex;
	NewSave->CampaignRowName = RowName;
	NewSave->CampaignDisplayName = DisplayName;

	// Anchor to current universe seconds
	uint64 NowUniverse = 0ULL;
	if (UTimerSubsystem* Timer = GetGameInstance()->GetSubsystem<UTimerSubsystem>())
	{
		NowUniverse = Timer->GetUniverseTimeSeconds();
	}
	NewSave->InitializeCampaignClock(NowUniverse);

	// Persist
	const bool bOK = UGameplayStatics::SaveGameToSlot(NewSave, Slot, UserIndex);
	if (!bOK)
		return nullptr;

	CampaignSave = NewSave;

	// Inject into timer
	if (UTimerSubsystem* Timer = GetGameInstance()->GetSubsystem<UTimerSubsystem>())
	{
		Timer->SetCampaignSave(CampaignSave);
	}

	return CampaignSave;
}

bool USaveSubsystem::SaveCampaign(bool /*bForce*/)
{
	if (!CampaignSave || CampaignSave->CampaignRowName.IsNone())
		return false;

	const FString Slot = UCampaignSave::MakeSlotNameFromRowName(CampaignSave->CampaignRowName);
	return UGameplayStatics::SaveGameToSlot(CampaignSave, Slot, UserIndex);
}
