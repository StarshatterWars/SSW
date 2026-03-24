// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameStructs.h"
#include "UniverseSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UUniverseSaveGame : public USaveGame
{
	GENERATED_BODY()
	
	
public:
	UPROPERTY() FString UniverseId;
	UPROPERTY() uint64 UniverseSeed = 0;
	UPROPERTY() double UniverseTimeSeconds = 0.0;

	// Mutable layer:
	UPROPERTY() TMap<FName, FSystemState> SystemStates;

	UPROPERTY(SaveGame)
	int64 UniverseBaseUnixSeconds = 0;  // base “Jan 1, 2228 00:00:00” in Unix seconds
	
};
