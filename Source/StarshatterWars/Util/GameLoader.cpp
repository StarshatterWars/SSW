/*  Project Starshatter Wars
	Fractal Dev Games
	Copyright (C) 2024. All Rights Reserved.

	SUBSYSTEM:    Space
	FILE:         GameLoader.cpp
	AUTHOR:       Carlos Bott
*/

#include "GameLoader.h"
#include "SimUniverse.h"
#include "Sim.h"
#include "Campaign.h"
#include "DataLoader.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Guid.h"
#include "HAL/PlatformTime.h"
#include "UniverseSaveGame.h"
#include "MusicController.h"

void AGameLoader::BeginPlay()
{
	
	InitializeGame();
	
	LoadOrCreateUniverse();
	LoadMainMenu();
	//LoadGalaxy();
}

void AGameLoader::Tick(float DeltaTime)
{
}

AGameLoader::AGameLoader()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SetActorHiddenInGame(true);
	SetCanBeDamaged(false);
}

USSWGameInstance* AGameLoader::GetSSWGameInstance()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	return SSWInstance;
}

void AGameLoader::LoadGalaxy()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->SpawnGalaxy();
}

void AGameLoader::InitializeGame()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	
	//UUniverse* Universe;
	//Universe = NewObject<UUniverse>();

	//USim* Sim;
	//Sim = NewObject<USim>();

	//Campaign* Campaign;
	//Campaign::Initialize();
}

void AGameLoader::LoadMainMenu()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->ShowMainMenuScreen();
}

void AGameLoader::LoadOrCreateUniverse()
{
	// When campaign starts
	const FString Slot = GetUniverseSlotName();
	constexpr int32 UserIndex = 0;

	UUniverseSaveGame* LoadedSave = nullptr;

	// -------------------- LOAD IF EXISTS --------------------
	if (UGameplayStatics::DoesSaveGameExist(Slot, UserIndex))
	{
		if (USaveGame* Raw = UGameplayStatics::LoadGameFromSlot(Slot, UserIndex))
		{
			LoadedSave = Cast<UUniverseSaveGame>(Raw);
		}
	}

	UniverseSave = LoadedSave;

	// -------------------- CREATE IF MISSING/CORRUPT --------------------
	if (!UniverseSave)
	{
		UniverseSave = Cast<UUniverseSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UUniverseSaveGame::StaticClass())
		);

		if (!UniverseSave)
		{
			UE_LOG(LogTemp, Error, TEXT("LoadOrCreateUniverse: Failed to create UUniverseSaveGame"));
			return;
		}

		UniverseSave->UniverseId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
		UniverseSave->UniverseSeed = FPlatformTime::Cycles64();

		// Base date set ONCE on create
		const FDateTime BaseDate(2228, 1, 1);
		UniverseSave->UniverseBaseUnixSeconds = BaseDate.ToUnixTimestamp();

		// Start of campaign
		UniverseSave->UniverseTimeSeconds = 0;

		UGameplayStatics::SaveGameToSlot(UniverseSave, Slot, UserIndex);
	}
	else
	{
		// -------------------- REPAIR OLDER SAVES --------------------
		// If you added UniverseBaseUnixSeconds later, older saves may have 0.
		if (UniverseSave->UniverseBaseUnixSeconds <= 0)
		{
			const FDateTime BaseDate(2228, 1, 1);
			UniverseSave->UniverseBaseUnixSeconds = BaseDate.ToUnixTimestamp();

			UGameplayStatics::SaveGameToSlot(UniverseSave, Slot, UserIndex);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Universe loaded: Id=%s Seed=%llu Base=%lld Time=%llu"),
		*UniverseSave->UniverseId,
		(unsigned long long)UniverseSave->UniverseSeed,
		(long long)UniverseSave->UniverseBaseUnixSeconds,
		(unsigned long long)UniverseSave->UniverseTimeSeconds);


	// -------------------- PUSH INTO GAME INSTANCE --------------------
	UTimerSubsystem* Timer = GetGameInstance()->GetSubsystem<UTimerSubsystem>();
	USSWGameInstance* GI = GetSSWGameInstance();

	//if (USSWGameInstance* GI = GetSSWGameInstance())
	{
		GI->UniverseId = UniverseSave->UniverseId;
		GI->UniverseSeed = UniverseSave->UniverseSeed;
		Timer->UniverseBaseUnixSeconds = UniverseSave->UniverseBaseUnixSeconds;
		Timer->UniverseTimeSeconds = UniverseSave->UniverseTimeSeconds;

		GI->SetUniverseSaveContext(Slot, UserIndex, UniverseSave);
	}
}
